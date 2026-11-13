import { NextRequest, NextResponse } from "next/server";
import { getServerSession } from "next-auth";
import { authOptions } from "@/lib/auth";
import { connectDB } from "@/lib/mongodb";
import {
  TableDevice,
  AuthorizedCard,
  RackEvent,
  CashReconciliation,
  User,
} from "@/models";
import { ALL_ZONES, findZone, getOwnerZones } from "@/lib/zones";

export const dynamic = "force-dynamic";

export async function GET(req: NextRequest) {
  try {
    await connectDB();
    const session = await getServerSession(authOptions);
    const ownerId = (session?.user as unknown as { id?: string })?.id;

    if (!ownerId) {
      return NextResponse.json(
        { ok: false, error: "Unauthorized. Please sign in to access your venue dashboard." },
        { status: 401 }
      );
    }

    const now = new Date();
    const startOfToday = new Date(
      now.getFullYear(),
      now.getMonth(),
      now.getDate()
    );
    const sevenDaysAgo = new Date(
      startOfToday.getTime() - 6 * 24 * 3600 * 1000
    );

    // Strict multi-tenant query filter, optionally scoped to a single zone
    const zones = await getOwnerZones(ownerId);
    const requestedZone = new URL(req.url).searchParams.get("zone");
    let zone = ALL_ZONES;
    if (requestedZone === null) {
      const saved = (await User.findById(ownerId).select("dashboardZone").lean())?.dashboardZone;
      if (saved && saved !== ALL_ZONES && findZone(zones, saved)) zone = findZone(zones, saved)!;
    } else if (requestedZone !== ALL_ZONES) {
      zone = findZone(zones, requestedZone) || ALL_ZONES;
    }
    const zoneFilter = zone === ALL_ZONES ? {} : { venueName: zone };

    const deviceFilter = { ownerId, ...zoneFilter };
    const cardFilter = { ownerId };
    const eventFilter = { ownerId, ...zoneFilter };
    const recFilter = { ownerId, ...zoneFilter };

    const [tables, cards, recentEvents, last7DaysEvents, reconciliations, currentUser] =
      await Promise.all([
        TableDevice.find(deviceFilter).sort({ deviceId: 1 }).lean(),
        AuthorizedCard.find(cardFilter).sort({ status: 1, updatedAt: -1 }).lean(),
        RackEvent.find(eventFilter).sort({ createdAt: -1 }).limit(100).lean(),
        RackEvent.find({
          ...eventFilter,
          createdAt: { $gte: sevenDaysAgo },
        })
          .sort({ createdAt: 1 })
          .lean(),
        CashReconciliation.find(recFilter).sort({ createdAt: -1 }).limit(20).lean(),
        User.findById(ownerId).lean(),
      ]);

    if (!currentUser) {
      return NextResponse.json(
        { ok: false, error: "Venue account profile not found. Please log in again." },
        { status: 401 }
      );
    }

    const todayEvents = last7DaysEvents.filter(
      (e) => new Date(e.createdAt) >= startOfToday
    );

    const todayBillable = todayEvents.filter(
      (e) => e.event === "button_press" || e.event === "remote_unlock"
    );
    const todayRevenue = todayBillable.reduce(
      (acc, e) => acc + (e.priceCharged || 0),
      0
    );
    const todayRacks = todayBillable.length;
    const todayRfidRacks = todayBillable.filter(
      (e) => e.trigger === "rfid"
    ).length;
    const todayButtonRacks = todayBillable.filter(
      (e) => e.trigger === "button"
    ).length;
    const todayRfidRevenue = todayBillable
      .filter((e) => e.trigger === "rfid")
      .reduce((acc, e) => acc + (e.priceCharged || 0), 0);
    const todayCoinBoxRevenue = todayBillable
      .filter((e) => e.trigger === "button")
      .reduce((acc, e) => acc + (e.priceCharged || 0), 0);

    const securityAlertsToday = todayEvents.filter(
      (e) =>
        e.event === "unauthorized_card_attempt" ||
        e.event === "blocked_card_attempt"
    ).length;

    const sevenDayRevenue = last7DaysEvents
      .filter((e) => e.event === "button_press" || e.event === "remote_unlock")
      .reduce((acc, e) => acc + (e.priceCharged || 0), 0);

    const sevenDayRacks = last7DaysEvents.filter(
      (e) => e.event === "button_press" || e.event === "remote_unlock"
    ).length;

    const dailyChart: {
      date: string;
      label: string;
      racks: number;
      rfidRacks: number;
      buttonRacks: number;
      revenue: number;
      alerts: number;
    }[] = [];

    const dayNames = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
    for (let i = 6; i >= 0; i--) {
      const d = new Date(startOfToday.getTime() - i * 24 * 3600 * 1000);
      const nextD = new Date(d.getTime() + 24 * 3600 * 1000);
      const dateStr = d.toISOString().slice(0, 10);
      const dayEvs = last7DaysEvents.filter((e) => {
        const t = new Date(e.createdAt);
        return t >= d && t < nextD;
      });
      const billable = dayEvs.filter(
        (e) => e.event === "button_press" || e.event === "remote_unlock"
      );
      dailyChart.push({
        date: dateStr,
        label: i === 0 ? "Today" : dayNames[d.getDay()],
        racks: billable.length,
        rfidRacks: billable.filter((e) => e.trigger === "rfid").length,
        buttonRacks: billable.filter((e) => e.trigger === "button").length,
        revenue: billable.reduce((s, e) => s + (e.priceCharged || 0), 0),
        alerts: dayEvs.filter(
          (e) =>
            e.event === "unauthorized_card_attempt" ||
            e.event === "blocked_card_attempt"
        ).length,
      });
    }

    const enrichedTables = tables.map((t) => {
      const lastHb = t.lastHeartbeat ? new Date(t.lastHeartbeat).getTime() : 0;
      const secondsAgo = lastHb
        ? Math.floor((now.getTime() - lastHb) / 1000)
        : null;
      const isOnline = secondsAgo !== null && secondsAgo <= 90;

      const tableTodayBillable = todayBillable.filter(
        (e) => e.deviceId === t.deviceId
      );
      const tableTodayRevenue = tableTodayBillable.reduce(
        (s, e) => s + (e.priceCharged || 0),
        0
      );

      let isArmed = false;
      let armedRemainingSec = 0;
      if (t.rfidWriteCmd?.arm && t.rfidWriteCmd?.armedAt) {
        const elapsed = Math.floor(
          (now.getTime() - new Date(t.rfidWriteCmd.armedAt).getTime()) / 1000
        );
        if (elapsed < 65) {
          isArmed = true;
          armedRemainingSec = 65 - elapsed;
        }
      }

      const defaultUserZone = currentUser.activeVenue || zones[0] || "Main Floor";
      const cleanLocation =
        t.location && t.location !== "Unassigned"
          ? t.location
          : (t.venueName && t.venueName !== "Unassigned" ? t.venueName : defaultUserZone);

      if (t.location === "Unassigned" || !t.location || t.venueName === "Unassigned") {
        TableDevice.updateOne(
          { _id: t._id },
          { $set: { location: cleanLocation, venueName: cleanLocation } }
        ).catch(() => {});
      }

      return {
        ...t,
        location: cleanLocation,
        venueName: cleanLocation,
        isOnline,
        secondsSinceHeartbeat: secondsAgo,
        todayRacks: tableTodayBillable.length,
        todayRevenue: tableTodayRevenue,
        isArmed,
        armedRemainingSec,
      };
    });

    return NextResponse.json({
      ok: true,
      user: {
        id: currentUser._id,
        name: currentUser.name,
        email: currentUser.email,
        companyName: currentUser.companyName,
        venues: zones,
        activeVenue: currentUser.activeVenue || zones[0],
        subscriptionPlan: currentUser.subscriptionPlan,
        subscriptionStatus: currentUser.subscriptionStatus,
        trialEndsAt: currentUser.trialEndsAt,
      },
      zone,
      zones,
      summary: {
        todayRevenue,
        todayRacks,
        todayRfidRacks,
        todayButtonRacks,
        todayRfidRevenue,
        todayCoinBoxRevenue,
        securityAlertsToday,
        sevenDayRevenue,
        sevenDayRacks,
        onlineTablesCount: enrichedTables.filter((t) => t.isOnline).length,
        totalTablesCount: enrichedTables.length,
        activeCardsCount: cards.filter((c) => c.status === "active").length,
        blockedCardsCount: cards.filter((c) => c.status === "blocked").length,
      },
      tables: enrichedTables,
      cards,
      recentEvents,
      dailyChart,
      reconciliations,
    });
  } catch (error: unknown) {
    const message =
      error instanceof Error ? error.message : "Failed to load dashboard data";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

export async function POST(req: NextRequest) {
  try {
    await connectDB();
    const session = await getServerSession(authOptions);
    const ownerId = (session?.user as unknown as { id?: string })?.id;

    if (!ownerId) {
      return NextResponse.json(
        { ok: false, error: "Unauthorized access. Please log in." },
        { status: 401 }
      );
    }

    const body = await req.json();
    const { action } = body;

    if (action === "arm_card_writer") {
      const { deviceId, staffName, staffId, role } = body;
      if (!deviceId || !staffName || !staffId) {
        return NextResponse.json(
          { ok: false, error: "Device, Staff Name, and Staff ID are required." },
          { status: 400 }
        );
      }

      const table = await TableDevice.findOne({ deviceId, ownerId });
      if (!table) {
        return NextResponse.json(
          { ok: false, error: "Table node not found or not owned by your account." },
          { status: 404 }
        );
      }

      await TableDevice.updateOne(
        { deviceId, ownerId },
        {
          $set: {
            "rfidWriteCmd.arm": true,
            "rfidWriteCmd.staffName": staffName.trim(),
            "rfidWriteCmd.staffId": staffId.trim().toUpperCase(),
            "rfidWriteCmd.role": role || "staff",
            "rfidWriteCmd.armedAt": new Date(),
          },
        }
      );

      return NextResponse.json({
        ok: true,
        message: `RFID Reader on ${table.name} is now armed for ${staffName}. Tap blank card on table pod within 60 seconds.`,
      });
    }

    if (action === "cancel_card_writer") {
      const { deviceId } = body;
      await TableDevice.updateOne(
        { deviceId, ownerId },
        {
          $set: {
            "rfidWriteCmd.arm": false,
            "rfidWriteCmd.staffName": "",
            "rfidWriteCmd.staffId": "",
            "rfidWriteCmd.armedAt": null,
          },
        }
      );
      return NextResponse.json({
        ok: true,
        message: "Remote card writer disarmed.",
      });
    }

    if (action === "upsert_card") {
      const {
        cardUid,
        staffName,
        staffId,
        role,
        status,
        dailyLimit,
        notes,
        assignedTables,
      } = body;
      if (!cardUid || !staffName || !staffId) {
        return NextResponse.json(
          { ok: false, error: "Card UID, Staff Name, and Staff ID required." },
          { status: 400 }
        );
      }
      const normalizedUid = cardUid.toUpperCase().trim();
      const card = await AuthorizedCard.findOneAndUpdate(
        { cardUid: normalizedUid, ownerId },
        {
          $set: {
            cardUid: normalizedUid,
            staffName: staffName.trim(),
            staffId: staffId.trim().toUpperCase(),
            role: role || "staff",
            status: status || "active",
            dailyLimit: Number(dailyLimit) || 200,
            notes: notes || "",
            assignedTables: assignedTables || ["*"],
            ownerId,
          },
        },
        { upsert: true, new: true }
      );
      return NextResponse.json({ ok: true, card });
    }

    if (action === "toggle_card_status") {
      const { cardUid, status } = body;
      const updated = await AuthorizedCard.findOneAndUpdate(
        { cardUid, ownerId },
        { $set: { status } },
        { new: true }
      );
      return NextResponse.json({ ok: true, card: updated });
    }

    if (action === "delete_card") {
      const { cardUid } = body;
      await AuthorizedCard.deleteOne({ cardUid, ownerId });
      return NextResponse.json({ ok: true });
    }

    if (action === "delete_table") {
      const { deviceId } = body;
      if (!deviceId) {
        return NextResponse.json(
          { ok: false, error: "Device ID is required to remove table." },
          { status: 400 }
        );
      }

      const table = await TableDevice.findOne({ deviceId, ownerId });
      if (!table) {
        return NextResponse.json(
          { ok: false, error: "Table node not found or not owned by your account." },
          { status: 404 }
        );
      }

      // If it's a registered hardware unit with a serial number or chipId, unclaim and release it
      if (table.serialNumber || table.chipId) {
        await TableDevice.updateOne(
          { deviceId, ownerId },
          {
            $set: {
              isClaimed: false,
              ownerId: null,
              venueName: "Unassigned",
              location: "Unassigned",
              name: `Table (${table.serialNumber || deviceId})`,
            },
          }
        );
      } else {
        // Purely manual/virtual table: delete permanently
        await TableDevice.deleteOne({ deviceId, ownerId });
      }

      return NextResponse.json({
        ok: true,
        message: `Table "${table.name || deviceId}" removed from your venue dashboard.`,
      });
    }

    if (action === "upsert_table") {
      const {
        deviceId,
        name,
        venueName,
        location,
        pricePerGame,
        currency,
        subscriptionTier,
        deviceToken,
      } = body;

      if (!deviceId || !name) {
        return NextResponse.json(
          { ok: false, error: "Device ID and Table Name are required." },
          { status: 400 }
        );
      }

      const userDoc = await User.findById(ownerId).select("venues activeVenue").lean();
      const fallbackZone = userDoc?.activeVenue || userDoc?.venues?.[0] || "Main Floor";
      const table = await TableDevice.findOneAndUpdate(
        { deviceId: deviceId.trim(), ownerId },
        {
          $set: {
            name: name.trim(),
            venueName: (venueName || fallbackZone).trim(),
            location: (location || fallbackZone).trim(),
            pricePerGame: Number(pricePerGame) || 500,
            currency: currency || "NGN",
            subscriptionTier: subscriptionTier || "growth",
            deviceToken: deviceToken || "adrenaline_snooker_secret_2026",
            ownerId,
          },
          $setOnInsert: {
            serialNumber: `ADR-SNK-${Math.floor(10000 + Math.random() * 90000)}`,
            claimCode: `${Math.floor(100000 + Math.random() * 900000)}`,
            isClaimed: true,
            firmwareVersion: "Smarthive Pool V1.0",
            lastHeartbeat: null,
            rssi: 0,
            localIp: "0.0.0.0",
            chipId: "",
          },
        },
        { upsert: true, new: true }
      );

      return NextResponse.json({ ok: true, table });
    }

    if (action === "record_reconciliation") {
      const {
        shiftDate,
        shiftLabel,
        managerName,
        expectedCashFromCoinBox,
        expectedRevenueFromStaffRfid,
        actualCashCounted,
        notes,
      } = body;

      const totalExpected =
        Number(expectedCashFromCoinBox || 0) +
        Number(expectedRevenueFromStaffRfid || 0);
      const actual = Number(actualCashCounted || 0);
      const variance = actual - totalExpected;
      const status =
        variance === 0 ? "balanced" : variance < 0 ? "shortage" : "overage";

      const rec = await CashReconciliation.create({
        ownerId,
        venueName: typeof body.venueName === "string" ? body.venueName.trim() : "",
        shiftDate: shiftDate || new Date().toISOString().slice(0, 10),
        shiftLabel: shiftLabel || "Daily Closeout",
        managerName: managerName || "Duty Manager",
        expectedCashFromCoinBox: Number(expectedCashFromCoinBox || 0),
        expectedRevenueFromStaffRfid: Number(expectedRevenueFromStaffRfid || 0),
        actualCashCounted: actual,
        variance,
        notes: notes || "",
        status,
      });

      return NextResponse.json({ ok: true, reconciliation: rec });
    }

    if (action === "remote_unlock") {
      const { deviceId } = body;
      if (!deviceId) {
        return NextResponse.json(
          { ok: false, error: "Device ID is required." },
          { status: 400 }
        );
      }

      const table = await TableDevice.findOne({ deviceId, ownerId });
      if (!table) {
        return NextResponse.json(
          { ok: false, error: "Table device node not found or not owned by your account." },
          { status: 404 }
        );
      }

      if (table.maintenanceLocked) {
        return NextResponse.json(
          { ok: false, error: `Cannot unlock: ${table.name} is currently locked for maintenance.` },
          { status: 400 }
        );
      }

      const userName = (session?.user?.name as string) || "Manager Cloud Remote";
      const unlockEvent = await RackEvent.create({
        ownerId,
        deviceId: table.deviceId,
        serialNumber: table.serialNumber || "",
        tableName: table.name,
        venueName: table.venueName,
        event: "remote_unlock",
        trigger: "cloud",
        staffName: userName,
        staffId: "CLOUD-REMOTE",
        priceCharged: table.pricePerGame || 500,
        currency: table.currency || "NGN",
        syncedFromOffline: false,
        rssi: table.rssi || 0,
        localIp: table.localIp || "",
      });

      await TableDevice.updateOne(
        { deviceId, ownerId },
        {
          $inc: { totalRacksLifetime: 1 },
          $set: {
            "unlockCmd.pending": true,
            "unlockCmd.requestedAt": new Date(),
            "unlockCmd.staffName": userName,
          },
        }
      );

      return NextResponse.json({
        ok: true,
        message: `Remote unlock dispatched for ${table.name}! Node will trigger on next heartbeat.`,
        event: unlockEvent,
      });
    }

    if (action === "toggle_maintenance") {
      const { deviceId, locked } = body;
      if (!deviceId) {
        return NextResponse.json(
          { ok: false, error: "Device ID is required." },
          { status: 400 }
        );
      }

      const updated = await TableDevice.findOneAndUpdate(
        { deviceId, ownerId },
        { $set: { maintenanceLocked: Boolean(locked) } },
        { new: true }
      );

      if (!updated) {
        return NextResponse.json(
          { ok: false, error: "Table node not found or not authorized." },
          { status: 404 }
        );
      }

      return NextResponse.json({
        ok: true,
        message: `Maintenance lock ${locked ? "ENABLED" : "DISABLED"} for ${updated?.name || deviceId}`,
        table: updated,
      });
    }

    return NextResponse.json(
      { ok: false, error: `Unknown action: ${action}` },
      { status: 400 }
    );
  } catch (error: unknown) {
    const message =
      error instanceof Error ? error.message : "Dashboard action error";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}
