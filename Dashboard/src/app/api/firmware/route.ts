import { NextRequest, NextResponse } from "next/server";
import { getServerSession } from "next-auth/next";
import { authOptions } from "@/lib/auth";
import { connectDB } from "@/lib/mongodb";
import { TableDevice } from "@/models";
import fs from "fs";
import path from "path";

export const dynamic = "force-dynamic";

const FIRMWARE_DIR = path.join(process.cwd(), "public", "firmware");

function ensureFirmwareDir() {
  if (!fs.existsSync(FIRMWARE_DIR)) {
    fs.mkdirSync(FIRMWARE_DIR, { recursive: true });
  }
}

export async function GET(req: NextRequest) {
  try {
    const session = await getServerSession(authOptions);
    if (!session?.user?.email) {
      return NextResponse.json({ ok: false, error: "Unauthorized" }, { status: 401 });
    }

    ensureFirmwareDir();
    const files = fs.readdirSync(FIRMWARE_DIR).filter((f) => f.endsWith(".bin"));

    const firmwareList = files.map((fileName) => {
      const filePath = path.join(FIRMWARE_DIR, fileName);
      const stat = fs.statSync(filePath);
      return {
        fileName,
        sizeBytes: stat.size,
        sizeFormatted: `${(stat.size / 1024).toFixed(1)} KB`,
        uploadedAt: stat.mtime,
        downloadUrl: `/firmware/${encodeURIComponent(fileName)}`,
      };
    });

    return NextResponse.json({ ok: true, firmwareList });
  } catch (error: unknown) {
    const message = error instanceof Error ? error.message : "Error listing firmware";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

export async function POST(req: NextRequest) {
  try {
    const session = await getServerSession(authOptions);
    if (!session?.user?.email) {
      return NextResponse.json({ ok: false, error: "Unauthorized" }, { status: 401 });
    }

    await connectDB();
    const contentType = req.headers.get("content-type") || "";

    // Case 1: Binary .bin file upload
    if (contentType.includes("multipart/form-data")) {
      ensureFirmwareDir();
      const formData = await req.formData();
      const file = formData.get("file") as File | null;
      const customVersion = (formData.get("version") as string | null) || "";

      if (!file) {
        return NextResponse.json({ ok: false, error: "No file uploaded" }, { status: 400 });
      }

      if (!file.name.endsWith(".bin")) {
        return NextResponse.json(
          { ok: false, error: "Invalid file type. Firmware must be a compiled .bin file." },
          { status: 400 }
        );
      }

      const bytes = await file.arrayBuffer();
      const buffer = Buffer.from(bytes);

      const cleanVersion = customVersion.trim()
        ? customVersion.trim().replace(/[^a-zA-Z0-9._-]/g, "_")
        : "";
      const baseName = cleanVersion ? `firmware_${cleanVersion}.bin` : file.name.replace(/[^a-zA-Z0-9._-]/g, "_");
      const targetPath = path.join(FIRMWARE_DIR, baseName);

      fs.writeFileSync(targetPath, buffer);

      // Construct absolute or relative URL
      const host = req.headers.get("host") || "localhost:3000";
      const protocol = req.headers.get("x-forwarded-proto") || (host.includes("localhost") ? "http" : "https");
      const downloadUrl = `${protocol}://${host}/firmware/${encodeURIComponent(baseName)}`;

      return NextResponse.json({
        ok: true,
        message: `Firmware binary ${baseName} uploaded successfully (${(buffer.length / 1024).toFixed(1)} KB)`,
        fileName: baseName,
        downloadUrl,
        sizeBytes: buffer.length,
      });
    }

    // Case 2: JSON Action (Trigger / Cancel OTA)
    const body = await req.json();
    const { action, deviceId, targetVersion, firmwareUrl } = body;

    if (action === "trigger_ota") {
      if (!targetVersion || !firmwareUrl) {
        return NextResponse.json(
          { ok: false, error: "Missing targetVersion or firmwareUrl" },
          { status: 400 }
        );
      }

      const updateData = {
        $set: {
          "otaCmd.pending": true,
          "otaCmd.targetVersion": targetVersion.trim(),
          "otaCmd.firmwareUrl": firmwareUrl.trim(),
          "otaCmd.triggeredAt": new Date(),
          "otaCmd.status": "pending",
          "otaCmd.errorMessage": "",
        },
      };

      if (!deviceId || deviceId === "*" || deviceId === "all") {
        // Broadcast to all devices owned by this user
        const result = await TableDevice.updateMany(
          session.user.role === "superadmin" ? {} : { ownerId: session.user.id },
          updateData
        );
        return NextResponse.json({
          ok: true,
          message: `Cloud OTA update to ${targetVersion} dispatched to ${result.modifiedCount} table(s).`,
        });
      } else {
        // Specific table
        const device = await TableDevice.findOneAndUpdate(
          session.user.role === "superadmin"
            ? { deviceId }
            : { deviceId, ownerId: session.user.id },
          updateData,
          { new: true }
        );

        if (!device) {
          return NextResponse.json({ ok: false, error: "Device not found" }, { status: 404 });
        }

        return NextResponse.json({
          ok: true,
          message: `Cloud OTA update to ${targetVersion} queued for ${device.name}. Table will flash automatically on next heartbeat.`,
        });
      }
    }

    if (action === "cancel_ota") {
      const filter =
        !deviceId || deviceId === "*" || deviceId === "all"
          ? session.user.role === "superadmin"
            ? {}
            : { ownerId: session.user.id }
          : { deviceId };

      await TableDevice.updateMany(filter, {
        $set: {
          "otaCmd.pending": false,
          "otaCmd.status": "idle",
          "otaCmd.errorMessage": "Cancelled by operator",
        },
      });

      return NextResponse.json({ ok: true, message: "OTA command cancelled" });
    }

    return NextResponse.json({ ok: false, error: "Unknown action" }, { status: 400 });
  } catch (error: unknown) {
    const message = error instanceof Error ? error.message : "Firmware API error";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

