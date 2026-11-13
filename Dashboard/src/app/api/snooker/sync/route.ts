import { NextRequest, NextResponse } from "next/server";
import { connectDB } from "@/lib/mongodb";
import { TableDevice, AuthorizedCard, RackEvent } from "@/models";
import { computeClaimPin, parseSerialToChipId } from "@/lib/hardware";

export const dynamic = "force-dynamic";

async function buildHardwareSyncResponse(deviceId: string) {
  const device = await TableDevice.findOne({ deviceId }).lean();

  // Auto-expire armed write command if older than 60 seconds
  let rfidWriteCmd = {
    arm: false,
    staffName: "",
    staffId: "",
  };

  if (device?.rfidWriteCmd?.arm) {
    const armedAt = device.rfidWriteCmd.armedAt
      ? new Date(device.rfidWriteCmd.armedAt).getTime()
      : Date.now();
    if (Date.now() - armedAt < 65000) {
      rfidWriteCmd = {
        arm: true,
        staffName: device.rfidWriteCmd.staffName || "Staff",
        staffId: device.rfidWriteCmd.staffId || "EMP01",
      };
    } else {
      await TableDevice.updateOne(
        { deviceId },
        {
          $set: {
            "rfidWriteCmd.arm": false,
            "rfidWriteCmd.staffName": "",
            "rfidWriteCmd.staffId": "",
            "rfidWriteCmd.armedAt": null,
          },
        }
      );
    }
  }

  const cardQuery: Record<string, unknown> = {
    status: "active",
    $or: [
      { assignedTables: "*" },
      { assignedTables: "ALL" },
      { assignedTables: deviceId },
    ],
  };
  if (device?.ownerId) {
    cardQuery.ownerId = device.ownerId;
  }

  const activeCards = await AuthorizedCard.find(cardQuery)
    .sort({ updatedAt: -1 })
    .limit(50)
    .lean();

  const authorizedCards = activeCards.map((c) => ({
    cardUid: c.cardUid.toUpperCase().trim(),
    staffName: c.staffName,
    staffId: c.staffId,
    role: c.role || "staff",
  }));

  const otaCmd = {
    pending: Boolean(device?.otaCmd?.pending),
    targetVersion: device?.otaCmd?.targetVersion || "",
    firmwareUrl: device?.otaCmd?.firmwareUrl || "",
  };

  let unlockCmd = {
    pending: false,
    staffName: "",
  };

  if (device?.unlockCmd?.pending) {
    const requestedAt = device.unlockCmd.requestedAt
      ? new Date(device.unlockCmd.requestedAt).getTime()
      : Date.now();
    // Valid within 60 seconds
    if (Date.now() - requestedAt < 60000) {
      unlockCmd = {
        pending: true,
        staffName: device.unlockCmd.staffName || "Cloud Remote",
      };
    }
    // Always consume the one-shot unlock command so it doesn't repeat
    await TableDevice.updateOne(
      { deviceId },
      {
        $set: {
          "unlockCmd.pending": false,
          "unlockCmd.requestedAt": null,
        },
      }
    );
  }

  return {
    ok: true,
    deviceId,
    tableName: device?.name || "Table",
    pricePerGame: device?.pricePerGame || 500,
    currency: device?.currency || "NGN",
    maintenanceLocked: Boolean(device?.maintenanceLocked),
    serverTime: new Date().toISOString(),
    serverEpoch: Math.floor(Date.now() / 1000),
    authorizedCards,
    rfidWriteCmd,
    otaCmd,
    unlockCmd,
  };
}

export async function GET(req: NextRequest) {
  try {
    await connectDB();
    const { searchParams } = new URL(req.url);
    const deviceId = searchParams.get("device_id");
    if (!deviceId) {
      return NextResponse.json({ ok: false, error: "Missing device_id" }, { status: 400 });
    }
    const payload = await buildHardwareSyncResponse(deviceId);
    return NextResponse.json(payload);
  } catch (error: unknown) {
    const message = error instanceof Error ? error.message : "Sync GET error";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

export async function POST(req: NextRequest) {
  try {
    await connectDB();

    const defaultDeviceSecret =
      process.env.SNOOKER_DEVICE_SECRET || "adrenaline_snooker_secret_2026";
    const incomingToken = (req.headers.get("x-device-token") || "").trim();

    const body = await req.json();
    const deviceId: string = body.device_id;
    if (!deviceId) {
      return NextResponse.json({ ok: false, error: "Missing device_id" }, { status: 400 });
    }
    const rssi: number = typeof body.rssi === "number" ? body.rssi : -58;
    const localIp: string = body.ip || "0.0.0.0";
    const chipId: string = body.chip_id || "Hardware Node";

    // Ensure the table device exists in fleet and verify x-device-token
    let table = await TableDevice.findOne({ deviceId });
    if (table) {
      const expectedToken = table.deviceToken || defaultDeviceSecret;
      if (!incomingToken || (incomingToken !== expectedToken && incomingToken !== defaultDeviceSecret)) {
        return NextResponse.json({ ok: false, error: "Unauthorized device token" }, { status: 401 });
      }
    } else if (!incomingToken || incomingToken !== defaultDeviceSecret) {
      return NextResponse.json({ ok: false, error: "Unauthorized device token" }, { status: 401 });
    }

    if (!table) {
      const serialNumber: string =
        body.serial_number ||
        (deviceId.startsWith("snooker_")
          ? `SMP-${deviceId.replace(/^snooker_/, "").toUpperCase()}`
          : `SN-${deviceId.toUpperCase()}`);
      const chipNum = parseSerialToChipId(serialNumber);
      const claimCode: string =
        body.claim_pin ||
        (chipNum !== null
          ? computeClaimPin(chipNum)
          : Math.floor(100000 + Math.random() * 900000).toString());

      table = await TableDevice.create({
        deviceId,
        serialNumber,
        claimCode,
        isClaimed: false,
        name: `Table (${serialNumber})`,
        venueName: "Unassigned",
        location: "Unassigned",
        pricePerGame: 500,
        currency: "NGN",
        lastHeartbeat: new Date(),
        rssi,
        localIp,
        chipId,
      });
    } else {
      table.lastHeartbeat = new Date();
      table.rssi = rssi;
      if (localIp) table.localIp = localIp;
      if (chipId) table.chipId = chipId;
      if (typeof body.free_heap === "number") table.freeHeap = body.free_heap;
      if (typeof body.reset_reason === "string") table.resetReason = body.reset_reason.slice(0, 64);
      if (typeof body.rc522_ok === "boolean") table.rc522Ok = body.rc522_ok;
      if (typeof body.offline_queue_count === "number") table.offlineQueueCount = body.offline_queue_count;
      if (typeof body.boot_count === "number") table.bootCount = body.boot_count;
      if (body.firmware_version) {
        table.firmwareVersion = body.firmware_version;
        if (table.otaCmd?.pending && body.firmware_version === table.otaCmd.targetVersion) {
          table.otaCmd.pending = false;
          table.otaCmd.status = "success";
          table.otaCmd.errorMessage = "";
        }
      }
      if (body.event === "ota_failed") {
        if (table.otaCmd) {
          table.otaCmd.pending = false;
          table.otaCmd.status = "failed";
          table.otaCmd.errorMessage = body.card_uid || "Firmware download/flash failed";
        }
      }
      await table.save();
    }

    // Check if offline batch payload
    if (Array.isArray(body.batch)) {
      let racksInBatch = 0;
      for (const item of body.batch) {
        const clientEventId = typeof item.client_event_id === "string" ? item.client_event_id.trim() : "";
        if (clientEventId) {
          const existing = await RackEvent.findOne({ clientEventId }).select("_id").lean();
          if (existing) {
            continue;
          }
        }

        const evType = item.event || "button_press";
        const trigger =
          item.trigger || (item.card_uid ? "rfid" : "button");
        const cardUid = (item.card_uid || "").toUpperCase().trim();
        let staffName = item.staff_name || "";
        let staffId = item.staff_id || "";

        if (cardUid && (!staffName || !staffId)) {
          const cardDoc = await AuthorizedCard.findOne({ cardUid }).lean();
          if (cardDoc) {
            staffName = cardDoc.staffName;
            staffId = cardDoc.staffId;
          }
        }

        const isBillable = evType === "button_press";
        const priceCharged = isBillable ? table.pricePerGame : 0;
        const ageSeconds =
          typeof item.age_seconds === "number" && item.age_seconds > 0 && item.age_seconds < 86400 * 30
            ? Math.floor(item.age_seconds)
            : 0;
        const eventTimestamp = ageSeconds > 0 ? new Date(Date.now() - ageSeconds * 1000) : new Date();

        if (isBillable) {
          racksInBatch += 1;
          if (cardUid) {
            await AuthorizedCard.updateOne(
              { cardUid },
              {
                $inc: {
                  totalGamesUnlocked: 1,
                  totalRevenueGenerated: priceCharged,
                },
                $set: {
                  lastUsedAt: eventTimestamp,
                  lastUsedDevice: deviceId,
                },
              }
            );
          }
        }

        await RackEvent.create({
          ownerId: table.ownerId || null,
          deviceId,
          tableName: table.name,
          venueName: table.venueName,
          event: evType,
          trigger,
          cardUid,
          staffName:
            staffName || (trigger === "button" ? "Coin / Push Button" : evType.includes("attempt") ? "Unregistered Card" : "Unknown"),
          staffId: staffId || (trigger === "button" ? "COIN-BOX" : evType.includes("attempt") ? "SECURITY-ALERT" : "UNASSIGNED"),
          priceCharged,
          currency: table.currency,
          syncedFromOffline: true,
          ...(clientEventId ? { clientEventId } : {}),
          ageSeconds,
          hardwareDate: item.date || "",
          hardwareTime: item.time || "",
          rssi,
          localIp,
          createdAt: eventTimestamp,
        });
      }

      if (racksInBatch > 0) {
        await TableDevice.updateOne(
          { deviceId },
          { $inc: { totalRacksLifetime: racksInBatch } }
        );
      }
    } else {
      // Single real-time event payload
      const evType: string = body.event || "heartbeat";
      const trigger: "rfid" | "button" | "cloud" | "system" =
        body.trigger || (body.card_uid ? "rfid" : "button");
      const cardUid: string = (body.card_uid || "").toUpperCase().trim();
      let staffName: string = body.staff_name || "";
      let staffId: string = body.staff_id || "";

      if (evType === "card_written_success" && cardUid) {
        const pendingRole = table.rfidWriteCmd?.role || "staff";
        const finalStaffName =
          staffName || table.rfidWriteCmd?.staffName || "Pool Attendant";
        const finalStaffId =
          staffId || table.rfidWriteCmd?.staffId || "STF-01";

        await AuthorizedCard.findOneAndUpdate(
          { cardUid },
          {
            $set: {
              ownerId: table.ownerId || null,
              cardUid,
              staffName: finalStaffName,
              staffId: finalStaffId,
              role: pendingRole,
              status: "active",
              lastUsedDevice: deviceId,
            },
            $setOnInsert: {
              assignedTables: ["*"],
              dailyLimit: 200,
              totalGamesUnlocked: 0,
              totalRevenueGenerated: 0,
            },
          },
          { upsert: true, new: true }
        );

        // Disarm write command on the table
        await TableDevice.updateOne(
          { deviceId },
          {
            $set: {
              "rfidWriteCmd.arm": false,
              "rfidWriteCmd.staffName": "",
              "rfidWriteCmd.staffId": "",
              "rfidWriteCmd.armedAt": null,
            },
          }
        );

        await RackEvent.create({
          ownerId: table.ownerId || null,
          deviceId,
          tableName: table.name,
          venueName: table.venueName,
          event: "card_written_success",
          trigger: "rfid",
          cardUid,
          staffName: finalStaffName,
          staffId: finalStaffId,
          priceCharged: 0,
          currency: table.currency,
          syncedFromOffline: false,
          rssi,
          localIp,
        });
      } else if (evType !== "heartbeat") {
        const clientEventId = typeof body.client_event_id === "string" ? body.client_event_id.trim() : "";
        const existingEvent = clientEventId
          ? await RackEvent.findOne({ clientEventId }).select("_id").lean()
          : null;

        if (!existingEvent) {
          if (cardUid && (!staffName || !staffId)) {
            const cardDoc = await AuthorizedCard.findOne({ cardUid }).lean();
            if (cardDoc) {
              staffName = cardDoc.staffName;
              staffId = cardDoc.staffId;
            }
          }

          const isBillable =
            evType === "button_press" || evType === "remote_unlock";
          const priceCharged = isBillable ? table.pricePerGame : 0;

          if (isBillable) {
            await TableDevice.updateOne(
              { deviceId },
              { $inc: { totalRacksLifetime: 1 } }
            );

            if (cardUid) {
              await AuthorizedCard.updateOne(
                { cardUid },
                {
                  $inc: {
                    totalGamesUnlocked: 1,
                    totalRevenueGenerated: priceCharged,
                  },
                  $set: {
                    lastUsedAt: new Date(),
                    lastUsedDevice: deviceId,
                  },
                }
              );
            }
          }

          await RackEvent.create({
            ownerId: table.ownerId || null,
            deviceId,
            tableName: table.name,
            venueName: table.venueName,
            event: evType as
              | "button_press"
              | "unauthorized_card_attempt"
              | "blocked_card_attempt"
              | "remote_unlock",
            trigger,
            cardUid,
            staffName:
              staffName ||
              (trigger === "button"
                ? "Coin / Push Button"
                : evType.includes("attempt")
                ? "Unregistered Card"
                : "Staff Member"),
            staffId:
              staffId ||
              (trigger === "button"
                ? "COIN-BOX"
                : evType.includes("attempt")
                ? "SECURITY-ALERT"
                : "STF"),
            priceCharged,
            currency: table.currency,
            syncedFromOffline: false,
            ...(clientEventId ? { clientEventId } : {}),
            rssi,
            localIp,
          });
        }
      }
    }

    const syncResponse = await buildHardwareSyncResponse(deviceId);
    return NextResponse.json(syncResponse);
  } catch (error: unknown) {
    const message = error instanceof Error ? error.message : "Sync POST error";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

