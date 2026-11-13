import { NextRequest, NextResponse } from "next/server";
import fs from "fs";
import path from "path";
import { requireAdmin } from "@/lib/adminAuth";
import { connectDB } from "@/lib/mongodb";
import { FirmwareRelease, TableDevice } from "@/models";

export async function GET() {
  const auth = await requireAdmin();
  if (!auth.authorized || auth.errorResponse) {
    return auth.errorResponse!;
  }

  try {
    await connectDB();
    const releases = await FirmwareRelease.find({}).sort({ createdAt: -1 }).lean();
    const devices = await TableDevice.find({}).select("firmwareVersion").lean();

    // Count how many devices are running each version
    const countByVersion: Record<string, number> = {};
    devices.forEach((d) => {
      const v = d.firmwareVersion || "v1.0.0-legacy";
      countByVersion[v] = (countByVersion[v] || 0) + 1;
    });

    const enriched = releases.map((r) => ({
      ...r,
      _id: r._id.toString(),
      activeDeviceCount: countByVersion[r.version] || 0,
    }));

    return NextResponse.json({
      ok: true,
      releases: enriched,
      totalDevices: devices.length,
      currentFleetVersionCounts: countByVersion,
    });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error listing firmwares";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

export async function POST(req: NextRequest) {
  const auth = await requireAdmin();
  if (!auth.authorized || auth.errorResponse) {
    return auth.errorResponse!;
  }

  try {
    await connectDB();
    const contentType = req.headers.get("content-type") || "";

    if (contentType.includes("multipart/form-data")) {
      const formData = await req.formData();
      const file = formData.get("file") as File | null;
      const version = ((formData.get("version") as string) || "").trim();
      const releaseNotes = ((formData.get("releaseNotes") as string) || "").trim();
      const isDefault = formData.get("isDefault") === "true";
      const targetHardware = ((formData.get("targetHardware") as string) || "esp32_devkit_v1").trim();

      if (!file || !version) {
        return NextResponse.json(
          { ok: false, error: "Both 'file' and 'version' (e.g. v2.1.0-esp32) are required." },
          { status: 400 }
        );
      }

      const buffer = Buffer.from(await file.arrayBuffer());
      const safeVersion = version.replace(/[^a-zA-Z0-9._-]/g, "_");
      const fileName = `${safeVersion}.bin`;

      const uploadDir = path.join(process.cwd(), "public", "firmware");
      if (!fs.existsSync(uploadDir)) {
        fs.mkdirSync(uploadDir, { recursive: true });
      }

      const filePath = path.join(uploadDir, fileName);
      fs.writeFileSync(filePath, buffer);

      const downloadUrl = `/firmware/${fileName}`;

      if (isDefault) {
        await FirmwareRelease.updateMany({}, { isDefault: false });
      }

      const release = await FirmwareRelease.findOneAndUpdate(
        { version: safeVersion },
        {
          version: safeVersion,
          targetHardware,
          fileName,
          downloadUrl,
          fileSizeBytes: buffer.length,
          releaseNotes: releaseNotes || `ESP32 binary release ${safeVersion}`,
          isDefault,
          isActive: true,
          uploadedBy: auth.user?.email || "admin",
        },
        { upsert: true, new: true }
      );

      return NextResponse.json({
        ok: true,
        message: `Firmware ${safeVersion} (${(buffer.length / 1024).toFixed(1)} KB) registered.`,
        release,
      });
    }

    // JSON payload to register an external URL release
    const body = await req.json();
    const { version, downloadUrl, releaseNotes, isDefault, targetHardware } = body;

    if (!version || !downloadUrl) {
      return NextResponse.json(
        { ok: false, error: "version and downloadUrl are required" },
        { status: 400 }
      );
    }

    if (isDefault) {
      await FirmwareRelease.updateMany({}, { isDefault: false });
    }

    const release = await FirmwareRelease.findOneAndUpdate(
      { version },
      {
        version,
        targetHardware: targetHardware || "esp32_devkit_v1",
        fileName: `${version}.bin`,
        downloadUrl,
        fileSizeBytes: 0,
        releaseNotes: releaseNotes || `Firmware release ${version}`,
        isDefault: Boolean(isDefault),
        isActive: true,
        uploadedBy: auth.user?.email || "admin",
      },
      { upsert: true, new: true }
    );

    return NextResponse.json({
      ok: true,
      message: `Firmware ${version} registered successfully.`,
      release,
    });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error registering firmware";
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
    const { action, releaseId, version, targetDeviceId } = body;

    if (action === "set_default") {
      if (!releaseId && !version) {
        return NextResponse.json({ ok: false, error: "releaseId or version required" }, { status: 400 });
      }
      await FirmwareRelease.updateMany({}, { isDefault: false });
      const query = releaseId ? { _id: releaseId } : { version };
      await FirmwareRelease.findOneAndUpdate(query, { isDefault: true });

      return NextResponse.json({
        ok: true,
        message: `Default firmware set to ${version || releaseId}.`,
      });
    }

    // Trigger fleet-wide or selective OTA update
    if (action === "fleet_rollout") {
      const release = await FirmwareRelease.findOne(releaseId ? { _id: releaseId } : { version });
      if (!release) {
        return NextResponse.json({ ok: false, error: "Firmware release not found" }, { status: 404 });
      }

      // Convert relative URL to full public URL if needed
      let fullUrl = release.downloadUrl;
      if (fullUrl.startsWith("/")) {
        const origin = req.nextUrl.origin || "http://localhost:3000";
        fullUrl = `${origin}${fullUrl}`;
      }

      const filter = targetDeviceId && targetDeviceId !== "*" ? { deviceId: targetDeviceId } : {};

      const result = await TableDevice.updateMany(filter, {
        $set: {
          otaCmd: {
            pending: true,
            targetVersion: release.version,
            firmwareUrl: fullUrl,
            triggeredAt: new Date(),
            status: "pending",
            errorMessage: "",
          },
        },
      });

      return NextResponse.json({
        ok: true,
        message: `Queued OTA update to ${release.version} for ${result.modifiedCount} devices.`,
        modifiedCount: result.modifiedCount,
      });
    }

    return NextResponse.json({ ok: false, error: "Unknown action" }, { status: 400 });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error updating firmware";
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
    const releaseId = searchParams.get("releaseId");

    if (!releaseId) {
      return NextResponse.json({ ok: false, error: "releaseId is required" }, { status: 400 });
    }

    const release = await FirmwareRelease.findByIdAndDelete(releaseId);
    if (!release) {
      return NextResponse.json({ ok: false, error: "Firmware not found" }, { status: 404 });
    }

    // Attempt to delete local binary if it exists
    if (release.fileName) {
      const filePath = path.join(process.cwd(), "public", "firmware", release.fileName);
      if (fs.existsSync(filePath)) {
        try {
          fs.unlinkSync(filePath);
        } catch {
          // ignore file deletion errors
        }
      }
    }

    return NextResponse.json({
      ok: true,
      message: `Firmware release ${release.version} deleted.`,
    });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error deleting firmware";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

