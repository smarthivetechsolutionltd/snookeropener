import { NextRequest, NextResponse } from "next/server";
import { getServerSession } from "next-auth";
import { authOptions } from "@/lib/auth";
import { connectDB } from "@/lib/mongodb";
import { TableDevice, User } from "@/models";
import { computeClaimPin, parseSerialToChipId } from "@/lib/hardware";

export async function POST(req: NextRequest) {
  try {
    const session = await getServerSession(authOptions);
    const ownerId = (session?.user as unknown as { id?: string })?.id;
    const ownerEmail = session?.user?.email;

    if (!ownerId || !ownerEmail) {
      return NextResponse.json(
        { ok: false, error: "Unauthorized. Please sign in to claim a hardware device." },
        { status: 401 }
      );
    }

    await connectDB();
    const { serialNumber, claimCode, tableName, venueName, pricePerGame } = await req.json();

    if (!serialNumber || !claimCode) {
      return NextResponse.json(
        { ok: false, error: "Serial number and Claim PIN are required" },
        { status: 400 }
      );
    }

    const cleanSerial = serialNumber.trim().toUpperCase();
    const cleanPin = claimCode.trim();

    // Find device in factory inventory or existing pool
    let device = await TableDevice.findOne({ serialNumber: cleanSerial });

    if (!device) {
      // Validate serial format and cryptographic claim PIN
      const chipId = parseSerialToChipId(cleanSerial);
      if (chipId !== null) {
        const expectedPin = computeClaimPin(chipId);
        if (expectedPin !== cleanPin) {
          return NextResponse.json(
            {
              ok: false,
              error:
                "Invalid 6-digit Claim PIN. Please check the label on the bottom of the enclosure or the controller setup portal.",
            },
            { status: 400 }
          );
        }
      }

      // Look up owner's preferred/existing zone
      const userDoc = await User.findById(ownerId).select("venues activeVenue").lean();
      const targetZone = venueName?.trim() || userDoc?.activeVenue || userDoc?.venues?.[0] || "Main Floor";

      // Provision new hardware record
      const hexChip = chipId !== null ? chipId.toString(16).padStart(6, "0") : cleanSerial.toLowerCase().replace(/[^a-z0-9]/g, "_");
      const generatedDeviceId = `snooker_${hexChip}`;
      device = await TableDevice.create({
        deviceId: generatedDeviceId,
        serialNumber: cleanSerial,
        claimCode: cleanPin,
        isClaimed: true,
        ownerId,
        ownerEmail,
        name: tableName?.trim() || `Table (${cleanSerial})`,
        venueName: targetZone,
        location: targetZone,
        pricePerGame: Number(pricePerGame) || 500,
        currency: "NGN",
        firmwareVersion: "Smarthive Pool V1.0",
        lastHeartbeat: null,
        rssi: 0,
        localIp: "0.0.0.0",
        chipId: hexChip.toUpperCase(),
        totalRacksLifetime: 0,
      });

      return NextResponse.json({
        ok: true,
        message: `Device ${cleanSerial} successfully paired to your account!`,
        device,
      });
    }

    // Check if device is already claimed
    if (device.isClaimed && device.ownerId && device.ownerId !== ownerId) {
      return NextResponse.json(
        { ok: false, error: "This device has already been claimed by another account. Contact Smarthivetech support if you are the legitimate owner." },
        { status: 403 }
      );
    }

    // Verify claim PIN
    if (device.claimCode !== cleanPin) {
      return NextResponse.json(
        { ok: false, error: "Invalid 6-digit Claim PIN. Please check the label on the bottom of the enclosure." },
        { status: 400 }
      );
    }

    device.isClaimed = true;
    device.ownerId = ownerId;
    device.ownerEmail = ownerEmail;
    if (tableName) device.name = tableName.trim();
    const userDocExisting = await User.findById(ownerId).select("venues activeVenue").lean();
    const assignedZone = venueName?.trim() || userDocExisting?.activeVenue || userDocExisting?.venues?.[0] || "Main Floor";
    device.venueName = assignedZone;
    device.location = assignedZone;
    if (pricePerGame) device.pricePerGame = Number(pricePerGame);
    await device.save();

    return NextResponse.json({
      ok: true,
      message: `Device ${cleanSerial} successfully linked and activated!`,
      device,
    });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Pairing error";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

