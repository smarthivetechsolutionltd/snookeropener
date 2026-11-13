import { NextRequest, NextResponse } from "next/server";
import { requireAdmin } from "@/lib/adminAuth";
import { connectDB } from "@/lib/mongodb";
import { ErrorLog, TableDevice, RackEvent } from "@/models";

export async function GET(req: NextRequest) {
  const auth = await requireAdmin();
  if (!auth.authorized || auth.errorResponse) {
    return auth.errorResponse!;
  }

  try {
    await connectDB();
    const { searchParams } = new URL(req.url);
    const severity = searchParams.get("severity");
    const type = searchParams.get("type");
    const resolved = searchParams.get("resolved");

    const filter: Record<string, unknown> = {};
    if (severity && severity !== "all") filter.severity = severity;
    if (type && type !== "all") filter.type = type;
    if (resolved !== null && resolved !== undefined && resolved !== "all") {
      filter.resolved = resolved === "true";
    }

    // Fetch logged errors
    const logs = await ErrorLog.find(filter).sort({ createdAt: -1 }).limit(100).lean();

    // Also check for active live issues across tables:
    // 1. Tables with failed OTA
    const failedOtaTables = await TableDevice.find({ "otaCmd.status": "failed" })
      .select("deviceId name venueName otaCmd updatedAt")
      .lean();

    // 2. Recent unauthorized card attempts (anti-tamper alert)
    const unauthorizedAttempts = await RackEvent.find({
      event: { $in: ["unauthorized_card_attempt", "blocked_card_attempt"] },
    })
      .sort({ createdAt: -1 })
      .limit(10)
      .lean();

    return NextResponse.json({
      ok: true,
      logs: logs.map((l) => ({ ...l, _id: l._id.toString() })),
      telemetryAlerts: {
        failedOtas: failedOtaTables,
        unauthorizedAttempts,
      },
    });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error listing diagnostics";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

export async function POST(req: NextRequest) {
  try {
    await connectDB();
    const body = await req.json();
    const { type, severity, message, details, deviceId, venueName, ownerEmail } = body;

    if (!message) {
      return NextResponse.json({ ok: false, error: "Message is required" }, { status: 400 });
    }

    const log = await ErrorLog.create({
      type: type || "client_exception",
      severity: severity || "warning",
      message: message.trim(),
      details: typeof details === "object" ? JSON.stringify(details, null, 2) : String(details || ""),
      deviceId,
      venueName,
      ownerEmail,
      resolved: false,
    });

    return NextResponse.json({ ok: true, log });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error logging diagnostic report";
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
    const { action, logId } = body;

    if (action === "toggle_resolved") {
      if (!logId) {
        return NextResponse.json({ ok: false, error: "logId is required" }, { status: 400 });
      }
      const log = await ErrorLog.findById(logId);
      if (!log) {
        return NextResponse.json({ ok: false, error: "Log not found" }, { status: 404 });
      }
      log.resolved = !log.resolved;
      log.resolvedAt = log.resolved ? new Date() : null;
      log.resolvedBy = log.resolved ? auth.user?.email : "";
      await log.save();

      return NextResponse.json({
        ok: true,
        message: `Error status updated to ${log.resolved ? "Resolved" : "Open"}.`,
        log,
      });
    }

    if (action === "resolve_all") {
      await ErrorLog.updateMany(
        { resolved: false },
        {
          $set: {
            resolved: true,
            resolvedAt: new Date(),
            resolvedBy: auth.user?.email || "admin",
          },
        }
      );
      return NextResponse.json({ ok: true, message: "All open errors marked as resolved." });
    }

    return NextResponse.json({ ok: false, error: "Unknown action" }, { status: 400 });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error updating diagnostics";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

