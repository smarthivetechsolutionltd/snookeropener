import { NextRequest, NextResponse } from "next/server";
import crypto from "crypto";
import { requireAdmin } from "@/lib/adminAuth";
import { connectDB } from "@/lib/mongodb";
import { TableDevice } from "@/models";

export async function GET(req: NextRequest) {
  const auth = await requireAdmin();
  if (!auth.authorized || auth.errorResponse) {
    return auth.errorResponse!;
  }

  try {
    await connectDB();
    const { searchParams } = new URL(req.url);
    const query = searchParams.get("q")?.trim() || "";
    const statusFilter = searchParams.get("status") || "all";

    const filter: Record<string, unknown> = {};
    if (query) {
      filter.$or = [
        { deviceId: { $regex: query, $options: "i" } },
        { serialNumber: { $regex: query, $options: "i" } },
        { name: { $regex: query, $options: "i" } },
        { ownerEmail: { $regex: query, $options: "i" } },
        { venueName: { $regex: query, $options: "i" } },
      ];
    }

    if (statusFilter === "claimed") filter.isClaimed = true;
    if (statusFilter === "unclaimed") filter.isClaimed = false;
    if (statusFilter === "maintenance") filter.maintenanceLocked = true;

    const devices = await TableDevice.find(filter).sort({ updatedAt: -1 }).lean();

    const now = new Date();
    const onlineThreshold = new Date(now.getTime() - 90 * 1000);

    const enriched = devices.map((d) => {
      const isOnline = Boolean(d.lastHeartbeat && new Date(d.lastHeartbeat) >= onlineThreshold);
      const secondsSince = d.lastHeartbeat
        ? Math.floor((now.getTime() - new Date(d.lastHeartbeat).getTime()) / 1000)
        : null;

      return {
        ...d,
        _id: d._id.toString(),
        isOnline,
        secondsSinceHeartbeat: secondsSince,
      };
    });

    return NextResponse.json({ ok: true, devices: enriched, total: enriched.length });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error listing devices";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

/**
 * Pre-provisions a new hardware node for factory manufacturing or testing.
 * Generates official Serial Number & Claim PIN.
 */
export async function POST(req: NextRequest) {
  const auth = await requireAdmin();
  if (!auth.authorized || auth.errorResponse) {
    return auth.errorResponse!;
  }

  try {
    await connectDB();
    const body = await req.json().catch(() => ({}));
    const { venueName, location, name, pricePerGame } = body;

    const count = await TableDevice.countDocuments({});
    const randomHex = crypto.randomBytes(3).toString("hex").toUpperCase();
    const serialNumber = `SNK-ESP32-${randomHex}`;
    const claimCode = Math.floor(100000 + Math.random() * 900000).toString(); // 6-digit numeric PIN
    const deviceId = `snooker_node_${count + 1}`;

    const newDevice = await TableDevice.create({
      deviceId,
      serialNumber,
      claimCode,
      isClaimed: false,
      ownerId: null,
      ownerEmail: null,
      name: name || `Factory Table ${count + 1}`,
      venueName: venueName || "Unassigned Venue",
      location: location || "Bay 1",
      pricePerGame: Number(pricePerGame) || 500,
      firmwareVersion: "v2.0.0-esp32",
      rssi: -60,
      totalRacksLifetime: 0,
      maintenanceLocked: false,
    });

    return NextResponse.json({
      ok: true,
      message: `Provisioned hardware node ${serialNumber} with Claim PIN ${claimCode}`,
      device: newDevice,
    });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error provisioning device";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

export async function PATCH(req: NextRequest) {
  const auth = await requireAdmin();
  if (!auth.authorized || auth.errorResponse) {
    return auth.errorResponse!;
  }

  try {
    await connectDB();
    const body = await req.json();
    const { deviceId, action, ownerEmail, maintenanceLocked } = body;

    if (!deviceId) {
      return NextResponse.json({ ok: false, error: "deviceId is required" }, { status: 400 });
    }

    const device = await TableDevice.findOne({ deviceId });
    if (!device) {
      return NextResponse.json({ ok: false, error: "Device not found" }, { status: 404 });
    }

    if (action === "unpair") {
      device.isClaimed = false;
      device.ownerId = null;
      device.ownerEmail = null;
      await device.save();
      return NextResponse.json({
        ok: true,
        message: `Device ${deviceId} unlinked from owner and ready to be claimed again.`,
      });
    }

    if (action === "reset_pin") {
      const newPin = Math.floor(100000 + Math.random() * 900000).toString();
      device.claimCode = newPin;
      await device.save();
      return NextResponse.json({
        ok: true,
        message: `Generated new Claim PIN ${newPin} for ${deviceId}.`,
        newPin,
      });
    }

    if (action === "assign_owner") {
      if (!ownerEmail) {
        return NextResponse.json({ ok: false, error: "ownerEmail is required" }, { status: 400 });
      }
      device.ownerEmail = ownerEmail.toLowerCase().trim();
      device.isClaimed = true;
      await device.save();
      return NextResponse.json({
        ok: true,
        message: `Assigned device ${deviceId} to ${ownerEmail}.`,
      });
    }

    if (action === "toggle_maintenance") {
      device.maintenanceLocked = Boolean(maintenanceLocked);
      await device.save();
      return NextResponse.json({
        ok: true,
        message: `Table ${deviceId} maintenance lock set to ${device.maintenanceLocked}.`,
      });
    }

    return NextResponse.json({ ok: false, error: "Unknown action" }, { status: 400 });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error updating device";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

export async function DELETE(req: NextRequest) {
  const auth = await requireAdmin();
  if (!auth.authorized || auth.errorResponse) {
    return auth.errorResponse!;
  }

  try {
    await connectDB();
    const { searchParams } = new URL(req.url);
    const deviceId = searchParams.get("deviceId");

    if (!deviceId) {
      return NextResponse.json({ ok: false, error: "deviceId is required" }, { status: 400 });
    }

    const res = await TableDevice.findOneAndDelete({ deviceId });
    if (!res) {
      return NextResponse.json({ ok: false, error: "Device not found" }, { status: 404 });
    }

    return NextResponse.json({
      ok: true,
      message: `Device ${deviceId} permanently deleted from registry.`,
    });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error deleting device";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

