import { NextRequest, NextResponse } from "next/server";
import { getServerSession } from "next-auth";
import { authOptions } from "@/lib/auth";
import { connectDB } from "@/lib/mongodb";
import { User } from "@/models";

/** Returns the profile fields used to prefill the onboarding modal. */
export async function GET() {
  const session = await getServerSession(authOptions);
  if (!session?.user?.email) {
    return NextResponse.json({ ok: false, error: "Unauthorized" }, { status: 401 });
  }

  await connectDB();
  const user = await User.findOne({ email: session.user.email.toLowerCase() });
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
      activeVenue: user.activeVenue || "",
      city: user.city || "",
      tableCount: user.tableCount || 0,
    },
  });
}

/** Saves the profile details collected from Google (OAuth) users and completes onboarding. */
export async function POST(req: NextRequest) {
  try {
    const session = await getServerSession(authOptions);
    if (!session?.user?.email) {
      return NextResponse.json({ ok: false, error: "Unauthorized" }, { status: 401 });
    }

    const body = await req.json();
    const name = String(body.name || "").trim();
    const phoneNumber = String(body.phoneNumber || "").trim();
    const companyName = String(body.companyName || "").trim();
    const activeVenue = String(body.activeVenue || "").trim();
    const city = String(body.city || "").trim();
    const tableCount = Math.max(0, Math.min(500, Number(body.tableCount) || 0));

    if (!name || !phoneNumber || !companyName || !activeVenue || !city) {
      return NextResponse.json(
        { ok: false, error: "Please fill in all required fields" },
        { status: 400 }
      );
    }

    if (!/^\+?[0-9\s-]{7,20}$/.test(phoneNumber)) {
      return NextResponse.json(
        { ok: false, error: "Please enter a valid phone number" },
        { status: 400 }
      );
    }

    await connectDB();
    const user = await User.findOneAndUpdate(
      { email: session.user.email.toLowerCase() },
      {
        $set: {
          name,
          phoneNumber,
          companyName,
          activeVenue,
          city,
          tableCount,
          onboardingCompleted: true,
        },
        $addToSet: { venues: activeVenue },
      },
      { new: true }
    );

    if (!user) {
      return NextResponse.json({ ok: false, error: "User not found" }, { status: 404 });
    }

    if (user.onboardingCompleted !== true) {
      return NextResponse.json(
        { ok: false, error: "Profile could not be saved. Please try again." },
        { status: 500 }
      );
    }

    return NextResponse.json({ ok: true, message: "Profile completed" });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Onboarding error";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

