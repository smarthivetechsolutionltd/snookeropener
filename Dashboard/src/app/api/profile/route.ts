import { NextRequest, NextResponse } from "next/server";
import { getServerSession } from "next-auth";
import { authOptions } from "@/lib/auth";
import { connectDB } from "@/lib/mongodb";
import { User } from "@/models";

export const dynamic = "force-dynamic";

async function getOwnerId() {
  const session = await getServerSession(authOptions);
  return (session?.user as unknown as { id?: string })?.id || null;
}

/** GET: current owner profile. */
export async function GET() {
  const ownerId = await getOwnerId();
  if (!ownerId) {
    return NextResponse.json({ ok: false, error: "Unauthorized" }, { status: 401 });
  }

  await connectDB();
  const user = await User.findById(ownerId).lean();
  if (!user) {
    return NextResponse.json({ ok: false, error: "User not found" }, { status: 404 });
  }

  return NextResponse.json({
    ok: true,
    profile: {
      name: user.name || "",
      email: user.email,
      image: user.image || "",
      phoneNumber: user.phoneNumber || "",
      companyName: user.companyName || "",
      city: user.city || "",
      role: user.role,
      subscriptionPlan: user.subscriptionPlan,
      subscriptionStatus: user.subscriptionStatus,
      hasPassword: Boolean(user.password),
      createdAt: user.createdAt,
    },
  });
}

/** PATCH: update editable profile fields. */
export async function PATCH(req: NextRequest) {
  try {
    const ownerId = await getOwnerId();
    if (!ownerId) {
      return NextResponse.json({ ok: false, error: "Unauthorized" }, { status: 401 });
    }

    const body = await req.json();
    const name = String(body.name ?? "").trim().slice(0, 80);
    const companyName = String(body.companyName ?? "").trim().slice(0, 80);
    const phoneNumber = String(body.phoneNumber ?? "").trim();
    const city = String(body.city ?? "").trim().slice(0, 60);

    if (!name || !companyName) {
      return NextResponse.json(
        { ok: false, error: "Full name and arena name are required" },
        { status: 400 }
      );
    }
    if (phoneNumber && !/^\+?[0-9\s-]{7,20}$/.test(phoneNumber)) {
      return NextResponse.json(
        { ok: false, error: "Please enter a valid phone number" },
        { status: 400 }
      );
    }

    await connectDB();
    await User.updateOne(
      { _id: ownerId },
      { $set: { name, companyName, phoneNumber, city } }
    );

    return NextResponse.json({ ok: true, message: "Profile updated" });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Profile update failed";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

