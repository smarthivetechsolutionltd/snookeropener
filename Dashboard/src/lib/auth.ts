import dns from "node:dns";
import { NextAuthOptions } from "next-auth";
import CredentialsProvider from "next-auth/providers/credentials";
import GoogleProvider from "next-auth/providers/google";
import bcrypt from "bcryptjs";
import { connectDB } from "@/lib/mongodb";
import { User } from "@/models";

// Ensure Node.js uses Google and Cloudflare DNS to resolve MongoDB Atlas SRV/TXT records
try {
  dns.setServers(["8.8.8.8", "8.8.4.4", "1.1.1.1"]);
} catch {
  // Ignore in environments where setting DNS servers is prohibited
}

/**
 * OAuth (Google) accounts have no password and skip the registration form,
 * so they must complete their profile before accessing the dashboard.
 */
function needsOnboarding(dbUser: { password?: string; onboardingCompleted?: boolean }): boolean {
  return !dbUser.password && !dbUser.onboardingCompleted;
}

export const authOptions: NextAuthOptions = {
  session: {
    strategy: "jwt",
    maxAge: 30 * 24 * 60 * 60, // 30 days
  },
  providers: [
    ...(process.env.GOOGLE_CLIENT_ID && process.env.GOOGLE_CLIENT_SECRET
      ? [
          GoogleProvider({
            clientId: process.env.GOOGLE_CLIENT_ID,
            clientSecret: process.env.GOOGLE_CLIENT_SECRET,
          }),
        ]
      : []),
    CredentialsProvider({
      name: "Credentials",
      credentials: {
        email: { label: "Email", type: "email" },
        password: { label: "Password", type: "password" },
      },
      async authorize(credentials) {
        if (!credentials?.email || !credentials?.password) {
          throw new Error("Please enter both email and password");
        }

        await connectDB();
        const user = await User.findOne({
          email: credentials.email.toLowerCase().trim(),
        });

        if (!user || !user.password) {
          throw new Error("No account found with this email");
        }

        const isValid = await bcrypt.compare(credentials.password, user.password);
        if (!isValid) {
          throw new Error("Invalid password");
        }

        return {
          id: user._id.toString(),
          name: user.name,
          email: user.email,
          role: user.role,
          companyName: user.companyName,
          emailVerified: user.emailVerified,
          needsOnboarding: false,
        };
      },
    }),
  ],
  callbacks: {
    async signIn({ user, account }) {
      if (account?.provider === "google") {
        try {
          await connectDB();
          let dbUser = await User.findOne({ email: user.email?.toLowerCase() });
          if (!dbUser) {
            dbUser = await User.create({
              name: user.name || "Venue Owner",
              email: user.email?.toLowerCase(),
              emailVerified: true,
              role: "owner",
              companyName: "",
              venues: ["Main Hall"],
              activeVenue: "Main Hall",
              subscriptionPlan: "growth",
              subscriptionStatus: "active",
              image: user.image,
              onboardingCompleted: false,
            });
          }
          (user as unknown as { id: string }).id = dbUser._id.toString();
          (user as unknown as { role: string }).role = dbUser.role;
          (user as unknown as { companyName: string }).companyName = dbUser.companyName;
          (user as unknown as { needsOnboarding: boolean }).needsOnboarding = needsOnboarding(dbUser);
        } catch (dbErr) {
          console.error("Database connection error during Google sign-in:", dbErr);
          throw dbErr;
        }
      }
      return true;
    },
    async jwt({ token, user, trigger }) {
      if (user) {
        token.id = user.id;
        token.role = (user as unknown as { role?: string }).role || "owner";
        token.companyName = (user as unknown as { companyName?: string }).companyName || "Smarthive Lounge";
        token.emailVerified = (user as unknown as { emailVerified?: boolean }).emailVerified ?? true;
        token.needsOnboarding = (user as unknown as { needsOnboarding?: boolean }).needsOnboarding ?? false;
      }

      // Re-read the profile from the database on session refresh, update triggers,
      // or to verify current role privileges. Never trust stale client tokens.
      if (
        (trigger === "update" || token.needsOnboarding !== false || !token.role) &&
        token.email
      ) {
        try {
          await connectDB();
          const dbUser = await User.findOne({ email: (token.email as string).toLowerCase() });
          if (dbUser) {
            token.name = dbUser.name;
            token.companyName = dbUser.companyName;
            token.role = dbUser.role || "owner";
            token.needsOnboarding = needsOnboarding(dbUser);
          }
        } catch (dbErr) {
          console.error("Failed to refresh user profile in JWT:", dbErr);
        }
      }
      return token;
    },
    async session({ session, token }) {
      if (session.user) {
        (session.user as unknown as { id: string }).id = token.id as string;
        (session.user as unknown as { role: string }).role = token.role as string;
        (session.user as unknown as { companyName: string }).companyName = token.companyName as string;
        (session.user as unknown as { emailVerified: boolean }).emailVerified = token.emailVerified as boolean;
        (session.user as unknown as { needsOnboarding: boolean }).needsOnboarding = Boolean(token.needsOnboarding);
      }
      return session;
    },
  },
  pages: {
    signIn: "/auth/signin",
    error: "/auth/signin",
  },
  secret: process.env.NEXTAUTH_SECRET || "smarthive_pool_super_secret_enterprise_jwt_key_2026",
};

