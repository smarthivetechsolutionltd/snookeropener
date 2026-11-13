import { NextRequest, NextResponse } from "next/server";
import { getServerSession } from "next-auth";
import { authOptions } from "@/lib/auth";
import { connectDB } from "@/lib/mongodb";
import { RackEvent } from "@/models";
import { ALL_ZONES, findZone, getOwnerZones } from "@/lib/zones";

export const dynamic = "force-dynamic";

export async function GET(req: NextRequest) {
  try {
    await connectDB();
    const session = await getServerSession(authOptions);
    const ownerId = (session?.user as unknown as { id?: string })?.id;

    if (!ownerId) {
      return NextResponse.json({ ok: false, error: "Unauthorized" }, { status: 401 });
    }

    const { searchParams } = new URL(req.url);
    const page = Math.max(1, parseInt(searchParams.get("page") || "1", 10) || 1);
    const limit = Math.min(100, Math.max(1, parseInt(searchParams.get("limit") || "15", 10) || 15));
    const requestedZone = searchParams.get("zone") || ALL_ZONES;
    const deviceId = searchParams.get("table") || "all";
    const trigger = searchParams.get("trigger") || "all";
    const eventFilter = searchParams.get("event") || "all";
    const search = (searchParams.get("search") || "").trim();

    const query: Record<string, unknown> = { ownerId };

    if (requestedZone !== ALL_ZONES) {
      const zones = await getOwnerZones(ownerId);
      const matchedZone = findZone(zones, requestedZone);
      if (matchedZone) {
        query.venueName = matchedZone;
      }
    }

    if (deviceId !== "all") {
      query.deviceId = deviceId;
    }

    if (trigger !== "all") {
      query.trigger = trigger;
    }

    if (eventFilter === "billable") {
      query.event = { $in: ["button_press", "remote_unlock"] };
    } else if (eventFilter === "alerts") {
      query.event = { $in: ["unauthorized_card_attempt", "blocked_card_attempt"] };
    } else if (eventFilter === "rfid_program") {
      query.event = "card_written_success";
    }

    if (search) {
      const regex = new RegExp(search.replace(/[.*+?^${}()|[\]\\]/g, "\\$&"), "i");
      query.$or = [
        { staffName: regex },
        { cardUid: regex },
        { tableName: regex },
        { deviceId: regex },
      ];
    }

    const skip = (page - 1) * limit;
    const [events, total] = await Promise.all([
      RackEvent.find(query).sort({ createdAt: -1 }).skip(skip).limit(limit).lean(),
      RackEvent.countDocuments(query),
    ]);

    return NextResponse.json({
      ok: true,
      events,
      total,
      page,
      limit,
      totalPages: Math.max(1, Math.ceil(total / limit)),
    });
  } catch (error: unknown) {
    const message = error instanceof Error ? error.message : "Failed to load events";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

