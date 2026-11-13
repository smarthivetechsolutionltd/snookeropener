import { NextRequest, NextResponse } from "next/server";
import { requireAdmin } from "@/lib/adminAuth";
import { connectDB } from "@/lib/mongodb";
import { User, TableDevice } from "@/models";

export async function GET(req: NextRequest) {
  const auth = await requireAdmin();
  if (!auth.authorized || auth.errorResponse) {
    return auth.errorResponse!;
  }

  try {
    await connectDB();
    const { searchParams } = new URL(req.url);
    const query = searchParams.get("q")?.trim() || "";
    const roleFilter = searchParams.get("role")?.trim() || "";

    const filter: Record<string, unknown> = {};
    if (roleFilter && roleFilter !== "all") {
      filter.role = roleFilter;
    }
    if (query) {
      filter.$or = [
        { name: { $regex: query, $options: "i" } },
        { email: { $regex: query, $options: "i" } },
        { companyName: { $regex: query, $options: "i" } },
      ];
    }

    const users = await User.find(filter)
      .select("-password -emailVerificationToken")
      .sort({ createdAt: -1 })
      .lean();

    // Map device counts per user
    const devices = await TableDevice.find({}).select("ownerId ownerEmail deviceId").lean();
    const deviceCountByOwner: Record<string, number> = {};

    devices.forEach((d) => {
      if (d.ownerEmail) {
        const email = d.ownerEmail.toLowerCase();
        deviceCountByOwner[email] = (deviceCountByOwner[email] || 0) + 1;
      }
      if (d.ownerId) {
        deviceCountByOwner[d.ownerId] = (deviceCountByOwner[d.ownerId] || 0) + 1;
      }
    });

    const enrichedUsers = users.map((u) => {
      const email = u.email?.toLowerCase();
      const id = u._id.toString();
      const tablesCount = (email ? deviceCountByOwner[email] : 0) || deviceCountByOwner[id] || 0;
      return {
        ...u,
        _id: id,
        tableCount: tablesCount,
      };
    });

    return NextResponse.json({
      ok: true,
      users: enrichedUsers,
      total: enrichedUsers.length,
    });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error listing users";
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
    const { userId, role, subscriptionPlan, subscriptionStatus, companyName } = body;

    if (!userId) {
      return NextResponse.json({ ok: false, error: "userId is required" }, { status: 400 });
    }

    const updateFields: Record<string, unknown> = {};
    if (role) {
      const allowedRoles = ["superadmin", "admin", "owner", "manager", "attendant"];
      if (!allowedRoles.includes(role)) {
        return NextResponse.json({ ok: false, error: "Invalid role specified" }, { status: 400 });
      }
      updateFields.role = role;
    }

    if (subscriptionPlan) {
      updateFields.subscriptionPlan = subscriptionPlan;
    }
    if (subscriptionStatus) {
      updateFields.subscriptionStatus = subscriptionStatus;
    }
    if (companyName) {
      updateFields.companyName = companyName.trim();
    }

    const updatedUser = await User.findByIdAndUpdate(userId, updateFields, {
      new: true,
    }).select("-password");

    if (!updatedUser) {
      return NextResponse.json({ ok: false, error: "User not found" }, { status: 404 });
    }

    return NextResponse.json({
      ok: true,
      message: `User updated successfully: ${updatedUser.name} (${updatedUser.role})`,
      user: updatedUser,
    });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error updating user";
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
    const userId = searchParams.get("userId");

    if (!userId) {
      return NextResponse.json({ ok: false, error: "userId parameter is required" }, { status: 400 });
    }

    if (userId === auth.user?.id) {
      return NextResponse.json(
        { ok: false, error: "Security restriction: You cannot delete your own admin account." },
        { status: 400 }
      );
    }

    const targetUser = await User.findById(userId);
    if (!targetUser) {
      return NextResponse.json({ ok: false, error: "User not found" }, { status: 404 });
    }

    // Unpair any devices belonging to this user
    await TableDevice.updateMany(
      { $or: [{ ownerId: userId }, { ownerEmail: targetUser.email }] },
      { $set: { isClaimed: false, ownerId: null, ownerEmail: null } }
    );

    await User.findByIdAndDelete(userId);

    return NextResponse.json({
      ok: true,
      message: `User ${targetUser.email} was removed and their tables unlinked.`,
    });
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : "Error deleting user";
    return NextResponse.json({ ok: false, error: message }, { status: 500 });
  }
}

