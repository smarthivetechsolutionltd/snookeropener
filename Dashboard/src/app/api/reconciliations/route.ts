import { NextRequest, NextResponse } from "next/server";
import { getServerSession } from "next-auth";
import { authOptions } from "@/lib/auth";
import { connectDB } from "@/lib/mongodb";
import { CashReconciliation } from "@/models";
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
    const limit = Math.min(50, Math.max(1, parseInt(searchParams.get("limit") || "10", 10) || 10));
    const requestedZone = searchParams.get("zone") || ALL_ZONES;

    const query: Record<string, unknown> = { ownerId };
    if (requestedZone !== ALL_ZONES) {
      const zones = await getOwnerZones(ownerId);
      const matchedZone = findZone(zones, requestedZone);
      if (matchedZone) {
        query.venueName = matchedZone;
      }
    }

    const skip = (page - 1) * limit;
    const [reconciliations, total] = await Promise.all([
      CashReconciliation.find(query).sort({ createdAt: -1 }).skip(skip).limit(limit).lean(),
      CashReconciliation.countDocuments(query),
    ]);

    return NextResponse.json({
      ok: true,
      reconciliations,
      total,
      page,
      limit,
      totalPages: Math.max(1, Math.ceil(total / limit)),
    });
  } catch (error: unknown) {
    const message = error instanceof Error ? error.message : "Failed to load reconciliations";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

