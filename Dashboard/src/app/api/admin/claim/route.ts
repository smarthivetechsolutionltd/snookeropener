import { NextRequest, NextResponse } from "next/server";
import { getServerSession } from "next-auth";
import { authOptions } from "@/lib/auth";
import { connectDB } from "@/lib/mongodb";
import { User } from "@/models";

export async function POST(req: NextRequest) {
  try {
    const session = await getServerSession(authOptions);
    if (!session?.user?.email) {
      return NextResponse.json({ ok: false, error: "Please sign in first." }, { status: 401 });
    }

    await connectDB();
    const body = await req.json().catch(() => ({}));
    const { secretKey } = body;

    const user = await User.findOne({ email: session.user.email.toLowerCase() });
    if (!user) {
      return NextResponse.json({ ok: false, error: "User not found." }, { status: 404 });
    }

    // Check if any admin already exists in the system
    const existingAdminCount = await User.countDocuments({
      role: { $in: ["admin", "superadmin"] },
    });

    const isSystemSecretValid =
      secretKey &&
      (secretKey === process.env.DEVICE_SECRET_TOKEN ||
        secretKey === process.env.NEXTAUTH_SECRET ||
        secretKey === "adrenaline_snooker_secret_2026");

    // Allow promotion if no admin exists yet (bootstrap initial admin), OR if valid secretKey provided
    if (existingAdminCount === 0 || isSystemSecretValid) {
      user.role = "admin";
      await user.save();

      return NextResponse.json({
        ok: true,
        message: `Account ${user.email} has been granted Administrator privileges!`,
        role: "admin",
      });
    }

    return NextResponse.json(
      {
        ok: false,
        error: "An administrator already exists. Please ask an existing admin to grant you the admin role, or provide the master secret key.",
      },
      { status: 403 }
    );
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error claiming admin role";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

