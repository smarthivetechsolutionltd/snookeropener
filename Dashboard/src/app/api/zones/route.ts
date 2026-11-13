import { NextRequest, NextResponse } from "next/server";
import { getServerSession } from "next-auth";
import { authOptions } from "@/lib/auth";
import { connectDB } from "@/lib/mongodb";
import { CashReconciliation, RackEvent, TableDevice, User } from "@/models";
import { ALL_ZONES, cleanZoneName, findZone, getOwnerZones } from "@/lib/zones";

export const dynamic = "force-dynamic";

async function requireOwner() {
  const session = await getServerSession(authOptions);
  const ownerId = (session?.user as unknown as { id?: string })?.id;
  return ownerId || null;
}

const unauthorized = () =>
  NextResponse.json({ ok: false, error: "Unauthorized. Please sign in." }, { status: 401 });

const badRequest = (error: string, status = 400) =>
  NextResponse.json({ ok: false, error }, { status });

/** Builds the zone list with live per-zone stats for the switcher and settings page. */
async function buildZoneSummaries(ownerId: string) {
  const zones = await getOwnerZones(ownerId);
  const now = new Date();
  const startOfToday = new Date(now.getFullYear(), now.getMonth(), now.getDate());

  const [tables, todayEvents, user] = await Promise.all([
    TableDevice.find({ ownerId }).select("venueName lastHeartbeat").lean(),
    RackEvent.find({
      ownerId,
      createdAt: { $gte: startOfToday },
      event: { $in: ["button_press", "remote_unlock"] },
    })
      .select("venueName priceCharged")
      .lean(),
    User.findById(ownerId).select("dashboardZone activeVenue").lean(),
  ]);

  const summaries = zones.map((name) => {
    const key = name.toLowerCase();
    const zoneTables = tables.filter((t) => (t.venueName || "").toLowerCase() === key);
    const zoneEvents = todayEvents.filter((e) => (e.venueName || "").toLowerCase() === key);
    return {
      name,
      tableCount: zoneTables.length,
      onlineCount: zoneTables.filter(
        (t) => t.lastHeartbeat && now.getTime() - new Date(t.lastHeartbeat).getTime() <= 90_000
      ).length,
      todayRacks: zoneEvents.length,
      todayRevenue: zoneEvents.reduce((s, e) => s + (e.priceCharged || 0), 0),
    };
  });

  const selected = user?.dashboardZone || ALL_ZONES;
  return {
    zones: summaries,
    selectedZone: selected === ALL_ZONES || findZone(zones, selected) ? selected : ALL_ZONES,
    primaryZone: user?.activeVenue || zones[0],
  };
}

/** GET: list zones with stats. */
export async function GET() {
  try {
    const ownerId = await requireOwner();
    if (!ownerId) return unauthorized();
    await connectDB();
    return NextResponse.json({ ok: true, ...(await buildZoneSummaries(ownerId)) });
  } catch (err: unknown) {
    return badRequest(err instanceof Error ? err.message : "Failed to load zones", 500);
  }
}

/**
 * POST actions:
 * - create      { name }
 * - rename      { name, newName }
 * - delete      { name, moveTo }
 * - select      { name }        (zone switcher, "all" allowed)
 * - set_primary { name }
 */
export async function POST(req: NextRequest) {
  try {
    const ownerId = await requireOwner();
    if (!ownerId) return unauthorized();
    await connectDB();

    const body = await req.json();
    const action = String(body.action || "");
    const zones = await getOwnerZones(ownerId);

    if (action === "create") {
      const name = cleanZoneName(body.name);
      if (!name) return badRequest("Zone name is required");
      if (name.toLowerCase() === ALL_ZONES) return badRequest("That zone name is reserved");
      if (findZone(zones, name)) return badRequest(`A zone named "${name}" already exists`);
      if (zones.length >= 50) return badRequest("You can have at most 50 zones");

      await User.updateOne({ _id: ownerId }, { $push: { venues: name } });
      return NextResponse.json({
        ok: true,
        message: `Zone "${name}" created`,
        ...(await buildZoneSummaries(ownerId)),
      });
    }

    if (action === "rename") {
      const current = findZone(zones, cleanZoneName(body.name));
      const newName = cleanZoneName(body.newName);
      if (!current) return badRequest("Zone not found", 404);
      if (!newName) return badRequest("New zone name is required");
      if (newName.toLowerCase() === ALL_ZONES) return badRequest("That zone name is reserved");
      const clash = findZone(zones, newName);
      if (clash && clash !== current) return badRequest(`A zone named "${newName}" already exists`);

      const user = await User.findById(ownerId).select("activeVenue dashboardZone").lean();
      await Promise.all([
        User.updateOne(
          { _id: ownerId },
          {
            $set: {
              venues: zones.map((z) => (z === current ? newName : z)),
              ...(user?.activeVenue?.toLowerCase() === current.toLowerCase()
                ? { activeVenue: newName }
                : {}),
              ...(user?.dashboardZone?.toLowerCase() === current.toLowerCase()
                ? { dashboardZone: newName }
                : {}),
            },
          }
        ),
        TableDevice.updateMany({ ownerId, venueName: current }, { $set: { venueName: newName } }),
        RackEvent.updateMany({ ownerId, venueName: current }, { $set: { venueName: newName } }),
        CashReconciliation.updateMany(
          { ownerId, venueName: current },
          { $set: { venueName: newName } }
        ),
      ]);

      return NextResponse.json({
        ok: true,
        message: `Zone renamed to "${newName}"`,
        ...(await buildZoneSummaries(ownerId)),
      });
    }

    if (action === "delete") {
      const target = findZone(zones, cleanZoneName(body.name));
      if (!target) return badRequest("Zone not found", 404);
      if (zones.length <= 1) return badRequest("You must keep at least one zone");

      const tableCount = await TableDevice.countDocuments({ ownerId, venueName: target });
      const moveTo = body.moveTo ? findZone(zones, cleanZoneName(body.moveTo)) : undefined;
      if (moveTo && moveTo === target) return badRequest("Choose a different zone to move tables to");
      if (tableCount > 0 && !moveTo) {
        return badRequest(`Move this zone's ${tableCount} table(s) to another zone before deleting it`);
      }

      const fallback = moveTo || zones.find((z) => z !== target)!;
      const user = await User.findById(ownerId).select("activeVenue dashboardZone").lean();

      await Promise.all([
        User.updateOne(
          { _id: ownerId },
          {
            $set: {
              venues: zones.filter((z) => z !== target),
              ...(user?.activeVenue === target ? { activeVenue: fallback } : {}),
              ...(user?.dashboardZone === target ? { dashboardZone: ALL_ZONES } : {}),
            },
          }
        ),
        // Merge the zone's tables and history into the destination zone
        ...(moveTo
          ? [
              TableDevice.updateMany({ ownerId, venueName: target }, { $set: { venueName: moveTo } }),
              RackEvent.updateMany({ ownerId, venueName: target }, { $set: { venueName: moveTo } }),
              CashReconciliation.updateMany(
                { ownerId, venueName: target },
                { $set: { venueName: moveTo } }
              ),
            ]
          : []),
      ]);

      return NextResponse.json({
        ok: true,
        message: moveTo
          ? `Zone "${target}" deleted. Tables and history moved to "${moveTo}".`
          : `Zone "${target}" deleted`,
        ...(await buildZoneSummaries(ownerId)),
      });
    }

    if (action === "select") {
      const raw = cleanZoneName(body.name);
      const name = raw.toLowerCase() === ALL_ZONES ? ALL_ZONES : findZone(zones, raw);
      if (!name) return badRequest("Zone not found", 404);
      await User.updateOne({ _id: ownerId }, { $set: { dashboardZone: name } });
      return NextResponse.json({ ok: true, selectedZone: name });
    }

    if (action === "set_primary") {
      const name = findZone(zones, cleanZoneName(body.name));
      if (!name) return badRequest("Zone not found", 404);
      await User.updateOne({ _id: ownerId }, { $set: { activeVenue: name } });
      return NextResponse.json({
        ok: true,
        message: `"${name}" is now your primary zone`,
        ...(await buildZoneSummaries(ownerId)),
      });
    }

    return badRequest(`Unknown action: ${action}`);
  } catch (err: unknown) {
    return badRequest(err instanceof Error ? err.message : "Zone action failed", 500);
  }
}

