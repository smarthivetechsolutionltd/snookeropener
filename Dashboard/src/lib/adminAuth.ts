import { NextResponse } from "next/server";
import { getServerSession } from "next-auth";
import { authOptions } from "@/lib/auth";
import { connectDB } from "@/lib/mongodb";
import { User } from "@/models";

export interface AdminAuthResult {
  authorized: boolean;
  user?: {
    id: string;
    email: string;
    name: string;
    role: string;
  };
  errorResponse?: NextResponse;
}

/**
 * Checks if the current authenticated caller has an admin or superadmin role.
 * Queries the database directly to prevent privilege elevation or stale JWT claims.
 */
export async function requireAdmin(): Promise<AdminAuthResult> {
  try {
    const session = await getServerSession(authOptions);

    if (!session?.user?.email) {
      return {
        authorized: false,
        errorResponse: NextResponse.json(
          { ok: false, error: "Authentication required. Please sign in." },
          { status: 401 }
        ),
      };
    }

    await connectDB();
    const dbUser = await User.findOne({
      email: session.user.email.toLowerCase().trim(),
    });

    if (!dbUser) {
      return {
        authorized: false,
        errorResponse: NextResponse.json(
          { ok: false, error: "Account not found." },
          { status: 404 }
        ),
      };
    }

    const role = dbUser.role;
    const isAdmin = role === "admin" || role === "superadmin";

    if (!isAdmin) {
      return {
        authorized: false,
        errorResponse: NextResponse.json(
          {
            ok: false,
            error: "Access denied. Only administrator accounts can access this console.",
          },
          { status: 403 }
        ),
      };
    }

    return {
      authorized: true,
      user: {
        id: dbUser._id.toString(),
        email: dbUser.email,
        name: dbUser.name,
        role: dbUser.role,
      },
    };
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Authorization verification failure";
    return {
      authorized: false,
      errorResponse: NextResponse.json({ ok: false, error: message }, { status: 500 }),
    };
  }
}

