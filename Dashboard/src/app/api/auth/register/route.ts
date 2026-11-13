import { NextRequest, NextResponse } from "next/server";
import crypto from "crypto";
import bcrypt from "bcryptjs";
import { connectDB } from "@/lib/mongodb";
import { User } from "@/models";

export async function POST(req: NextRequest) {
  try {
    await connectDB();
    const { name, email, password, companyName, phoneNumber } = await req.json();

    if (!name || !email || !password) {
      return NextResponse.json(
        { ok: false, error: "Name, email, and password are required" },
        { status: 400 }
      );
    }

    const cleanEmail = email.toLowerCase().trim();
    const existing = await User.findOne({ email: cleanEmail });
    if (existing) {
      return NextResponse.json(
        { ok: false, error: "An account with this email already exists" },
        { status: 400 }
      );
    }

    const hashedPassword = await bcrypt.hash(password, 10);
    const verificationToken = crypto.randomBytes(24).toString("hex");
    const tokenExpires = new Date(Date.now() + 24 * 3600 * 1000); // 24 hours

    const user = await User.create({
      name: name.trim(),
      email: cleanEmail,
      password: hashedPassword,
      companyName: companyName ? companyName.trim() : "Smarthive Arena",
      phoneNumber: phoneNumber ? phoneNumber.trim() : "",
      emailVerified: true, // Auto-verified for instant onboarding with token saved
      emailVerificationToken: verificationToken,
      emailVerificationExpires: tokenExpires,
      role: "owner",
      venues: ["Main Lounge"],
      activeVenue: "Main Lounge",
      subscriptionPlan: "growth",
      subscriptionStatus: "active",
      trialEndsAt: new Date(Date.now() + 14 * 24 * 3600 * 1000), // 14-day pro trial
      onboardingCompleted: true, // Profile info collected on the registration form
    });

    return NextResponse.json({
      ok: true,
      message: "Account registered successfully!",
      user: {
        id: user._id,
        name: user.name,
        email: user.email,
        companyName: user.companyName,
      },
    });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Registration error";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

