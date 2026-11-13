import { NextRequest, NextResponse } from "next/server";
import { getServerSession } from "next-auth";
import { authOptions } from "@/lib/auth";
import { connectDB } from "@/lib/mongodb";
import { TableDevice, RackEvent, AuthorizedCard } from "@/models";

export async function GET(req: NextRequest) {
  try {
    const session = await getServerSession(authOptions);
    const ownerId = (session?.user as unknown as { id?: string })?.id;

    if (!ownerId) {
      return NextResponse.json(
        { ok: false, error: "Unauthorized. Please sign in to export reports." },
        { status: 401 }
      );
    }

    await connectDB();
    const { searchParams } = new URL(req.url);
    const format = searchParams.get("format") || "csv";
    const days = parseInt(searchParams.get("days") || "7", 10);

    const sinceDate = new Date(Date.now() - days * 24 * 3600 * 1000);

    const query: Record<string, unknown> = {
      ownerId,
      createdAt: { $gte: sinceDate },
    };

    const zone = searchParams.get("zone");
    if (zone && zone !== "all") {
      query.venueName = zone;
    }

    const events = await RackEvent.find(query).sort({ createdAt: -1 }).lean();

    if (format === "csv") {
      const headers = [
        "Event ID",
        "Date Time (UTC)",
        "Zone",
        "Table Name",
        "Device ID",
        "Event Type",
        "Trigger Source",
        "Staff Name",
        "Staff / Card ID",
        "Price Charged (NGN)",
        "Offline Synced",
        "Wi-Fi RSSI (dBm)",
      ];

      const rows = events.map((ev) => [
        `"${ev._id}"`,
        `"${new Date(ev.createdAt).toISOString()}"`,
        `"${(ev.venueName || "").replace(/"/g, '""')}"`,
        `"${(ev.tableName || "").replace(/"/g, '""')}"`,
        `"${ev.deviceId}"`,
        `"${ev.event}"`,
        `"${ev.trigger}"`,
        `"${(ev.staffName || "").replace(/"/g, '""')}"`,
        `"${ev.cardUid || ev.staffId || ""}"`,
        ev.priceCharged || 0,
        ev.syncedFromOffline ? "YES" : "NO",
        ev.rssi || -60,
      ]);

      const csvContent = [headers.join(","), ...rows.map((r) => r.join(","))].join("\n");

      const zoneSlug =
        zone && zone !== "all" ? `_${zone.toLowerCase().replace(/[^a-z0-9]+/g, "_")}` : "";

      return new NextResponse(csvContent, {
        headers: {
          "Content-Type": "text/csv; charset=utf-8",
          "Content-Disposition": `attachment; filename="smarthive_pool_audit_report${zoneSlug}_${new Date().toISOString().slice(0, 10)}.csv"`,
        },
      });
    }

    return NextResponse.json({ ok: true, eventsCount: events.length, events });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Report error";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

