import { NextResponse } from "next/server";
import { requireAdmin } from "@/lib/adminAuth";
import { connectDB } from "@/lib/mongodb";
import {
  User,
  TableDevice,
  AuthorizedCard,
  RackEvent,
  FirmwareRelease,
  ErrorLog,
} from "@/models";

export async function GET() {
  const auth = await requireAdmin();
  if (!auth.authorized || auth.errorResponse) {
    return auth.errorResponse!;
  }

  try {
    await connectDB();

    const now = new Date();
    const onlineThreshold = new Date(now.getTime() - 90 * 1000); // Online if heartbeat within 90s
    const sevenDaysAgo = new Date(now.getTime() - 7 * 24 * 3600 * 1000);

    // Parallel aggregate queries for speed
    const [
      totalUsers,
      usersByRole,
      devices,
      rackEvents7d,
      totalCards,
      recentErrors,
      unresolvedErrorCount,
      firmwareReleases,
    ] = await Promise.all([
      User.countDocuments({}),
      User.aggregate([
        { $group: { _id: "$role", count: { $sum: 1 } } },
      ]),
      TableDevice.find({}).lean(),
      RackEvent.find({ createdAt: { $gte: sevenDaysAgo } }).lean(),
      AuthorizedCard.countDocuments({}),
      ErrorLog.find({}).sort({ createdAt: -1 }).limit(10).lean(),
      ErrorLog.countDocuments({ resolved: false }),
      FirmwareRelease.find({}).lean(),
    ]);

    // Role breakdown
    const roleMap: Record<string, number> = {
      owner: 0,
      manager: 0,
      attendant: 0,
      admin: 0,
      superadmin: 0,
    };
    usersByRole.forEach((r) => {
      if (r._id) roleMap[r._id] = r.count;
    });

    // Device telemetry stats
    let onlineCount = 0;
    let offlineCount = 0;
    let maintenanceCount = 0;
    let unclaimedCount = 0;
    let totalLifetimeRacks = 0;
    const rssiHistogram = { good: 0, fair: 0, weak: 0 };
    const firmwareDistribution: Record<string, number> = {};

    devices.forEach((dev) => {
      const isOnline = Boolean(dev.lastHeartbeat && new Date(dev.lastHeartbeat) >= onlineThreshold);
      if (isOnline) onlineCount++;
      else offlineCount++;

      if (dev.maintenanceLocked) maintenanceCount++;
      if (!dev.isClaimed) unclaimedCount++;

      totalLifetimeRacks += dev.totalRacksLifetime || 0;

      // RSSI grouping
      const rssi = dev.rssi || -70;
      if (rssi >= -65) rssiHistogram.good++;
      else if (rssi >= -78) rssiHistogram.fair++;
      else rssiHistogram.weak++;

      // Firmware grouping
      const fw = dev.firmwareVersion || "v1.0.0-legacy";
      firmwareDistribution[fw] = (firmwareDistribution[fw] || 0) + 1;
    });

    // 7-day usage velocity & revenue
    const dailyMap: Record<string, { date: string; games: number; revenue: number }> = {};
    for (let i = 6; i >= 0; i--) {
      const d = new Date(now.getTime() - i * 24 * 3600 * 1000);
      const key = d.toISOString().split("T")[0];
      dailyMap[key] = { date: key, games: 0, revenue: 0 };
    }

    // Trigger breakdown (RFID vs Button vs Cloud Remote)
    const triggerDistribution: Record<string, number> = {
      rfid: 0,
      button: 0,
      cloud: 0,
      system: 0,
    };

    // 24-hour peak gaming activity histogram
    const hourlyDistribution = Array.from({ length: 24 }, (_, hour) => ({
      hour,
      hourLabel: `${hour.toString().padStart(2, "0")}:00`,
      games: 0,
    }));

    let totalRevenue7d = 0;
    let totalGames7d = 0;

    rackEvents7d.forEach((evt) => {
      const dKey = new Date(evt.createdAt).toISOString().split("T")[0];
      const isUnlock =
        evt.event === "button_press" ||
        evt.event === "remote_unlock" ||
        evt.event === "card_written_success";

      if (isUnlock) {
        totalGames7d++;
        const price = evt.priceCharged || 0;
        totalRevenue7d += price;

        if (dailyMap[dKey]) {
          dailyMap[dKey].games++;
          dailyMap[dKey].revenue += price;
        }

        const hour = new Date(evt.createdAt).getHours();
        if (hourlyDistribution[hour]) {
          hourlyDistribution[hour].games++;
        }
      }

      const trig = evt.trigger || "rfid";
      triggerDistribution[trig] = (triggerDistribution[trig] || 0) + 1;
    });

    return NextResponse.json({
      ok: true,
      data: {
        caller: auth.user,
        kpi: {
          totalUsers,
          ownersCount: roleMap.owner || 0,
          managersCount: roleMap.manager || 0,
          attendantsCount: roleMap.attendant || 0,
          adminsCount: (roleMap.admin || 0) + (roleMap.superadmin || 0),
          totalDevices: devices.length,
          onlineDevices: onlineCount,
          offlineDevices: offlineCount,
          maintenanceLocked: maintenanceCount,
          unclaimedDevices: unclaimedCount,
          totalCards,
          totalLifetimeRacks,
          totalGames7d,
          totalRevenue7d,
          unresolvedErrors: unresolvedErrorCount,
        },
        trends: {
          daily: Object.values(dailyMap),
          hourly: hourlyDistribution,
          triggers: triggerDistribution,
          rssi: rssiHistogram,
          firmwares: firmwareDistribution,
        },
        firmwareReleasesCount: firmwareReleases.length,
        recentErrors,
      },
    });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error querying admin overview";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

