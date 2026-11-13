"use client";

import React, { useState } from "react";
import Image from "next/image";
import Link from "next/link";
import { signIn, useSession } from "next-auth/react";
import { useRouter } from "next/navigation";
import {
  Lock,
  Mail,
  ArrowRight,
  ShieldCheck,
  CheckCircle2,
  AlertTriangle,
  Radio,
  Server,
  Layers,
  Sparkles,
  Eye,
  EyeOff,
  Zap,
  Activity,
  Coins,
  Receipt,
} from "lucide-react";

export default function SignInPage() {
  const router = useRouter();
  const { data: session, status } = useSession();
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [showPassword, setShowPassword] = useState(false);
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [isGoogleSubmitting, setIsGoogleSubmitting] = useState(false);
  const [errorMsg, setErrorMsg] = useState<string | null>(null);

  React.useEffect(() => {
    if (status === "authenticated" && session) {
      router.push("/");
    }
  }, [status, session, router]);

  React.useEffect(() => {
    if (typeof window !== "undefined") {
      const params = new URLSearchParams(window.location.search);
      const err = params.get("error");
      if (err) {
        if (err.includes("ESERVFAIL") || err.includes("queryTxt")) {
          setErrorMsg("Database DNS error connecting to MongoDB Atlas. Public DNS has been configured; please click Continue with Google again.");
        } else if (err === "OAuthSignin" || err === "OAuthCallback") {
          setErrorMsg("Google sign-in was interrupted. Please try again.");
        } else {
          setErrorMsg(`Authentication error: ${err}`);
        }
      }
    }
  }, []);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setIsSubmitting(true);
    setErrorMsg(null);

    try {
      const res = await signIn("credentials", {
        redirect: false,
        email: email.trim().toLowerCase(),
        password,
      });

      if (res?.error) {
        setErrorMsg(res.error);
        setIsSubmitting(false);
      } else {
        router.push("/");
        router.refresh();
      }
    } catch (err: unknown) {
      const message =
        err instanceof Error ? err.message : "Authentication failed";
      setErrorMsg(message);
      setIsSubmitting(false);
    }
  };

  const handleGoogleSignIn = async () => {
    setIsGoogleSubmitting(true);
    try {
      await signIn("google", { callbackUrl: "/" });
    } catch {
      setIsGoogleSubmitting(false);
    }
  };

  return (
    <div className="min-h-screen bg-slate-950 flex items-center justify-center p-4 sm:p-6 lg:p-8 relative overflow-hidden">
      {/* Background radial glow */}
      <div className="absolute top-1/4 left-1/2 -translate-x-1/2 -translate-y-1/2 w-[600px] h-[600px] bg-emerald-500/10 rounded-full blur-[140px] pointer-events-none" />
      <div className="absolute bottom-10 right-10 w-96 h-96 bg-blue-500/5 rounded-full blur-3xl pointer-events-none" />

      <div className="w-full max-w-5xl grid grid-cols-1 lg:grid-cols-12 gap-8 items-center relative z-10">
        {/* Left Hero Showcase Banner (Desktop Only) */}
        <div className="hidden lg:flex lg:col-span-5 flex-col justify-between space-y-8 p-8 rounded-3xl bg-slate-900/60 border border-slate-800/80 backdrop-blur-xl shadow-2xl relative overflow-hidden">
          <div className="absolute -top-12 -right-12 w-48 h-48 bg-emerald-500/10 rounded-full blur-2xl" />

          <div className="space-y-4">
            <div className="flex items-center gap-3">
              <div className="w-12 h-12 rounded-2xl bg-slate-800 border border-emerald-500/40 p-2 shadow-lg shadow-emerald-950/60 flex items-center justify-center">
                <Image
                  src="/logo.png"
                  alt="Smarthive Pool"
                  width={40}
                  height={40}
                  className="object-contain"
                />
              </div>
              <div>
                <span className="font-extrabold text-base tracking-wider text-white">
                  SMARTHIVE POOL
                </span>
                <p className="text-[11px] text-emerald-400 font-semibold tracking-wide uppercase">
                  Hardware-SaaS Telematics
                </p>
              </div>
            </div>

            <div className="pt-2">
              <h1 className="text-2xl font-bold text-white tracking-tight leading-snug">
                Automated Solenoid Ball Opener Fleet Control
              </h1>
              <p className="text-xs text-slate-400 mt-2 leading-relaxed">
                Real-time device telemetry, anti-pilferage cash auditing, and over-the-air MIFARE RFID card programming for snooker arenas.
              </p>
            </div>
          </div>

          {/* Feature Highlights */}
          <div className="space-y-3.5 pt-2 border-t border-slate-800/80 text-xs">
            <div className="flex items-center gap-3 text-slate-300">
              <div className="p-2 rounded-xl bg-emerald-500/10 text-emerald-400 border border-emerald-500/20">
                <Radio className="w-4 h-4" />
              </div>
              <div>
                <div className="font-semibold text-white">Live Device Telemetry</div>
                <div className="text-[11px] text-slate-400">15s heartbeats & instant offline sync</div>
              </div>
            </div>

            <div className="flex items-center gap-3 text-slate-300">
              <div className="p-2 rounded-xl bg-blue-500/10 text-blue-400 border border-blue-500/20">
                <Receipt className="w-4 h-4" />
              </div>
              <div>
                <div className="font-semibold text-white">Anti-Pilferage Shift Audits</div>
                <div className="text-[11px] text-slate-400">Match push-button coins with staff float</div>
              </div>
            </div>

            <div className="flex items-center gap-3 text-slate-300">
              <div className="p-2 rounded-xl bg-purple-500/10 text-purple-400 border border-purple-500/20">
                <Zap className="w-4 h-4" />
              </div>
              <div>
                <div className="font-semibold text-white">Instant Remote Unlock</div>
                <div className="text-[11px] text-slate-400">One-click cloud trigger & maintenance locks</div>
              </div>
            </div>
          </div>

          {/* Live Node Status Pill */}
          <div className="p-3.5 rounded-2xl bg-slate-950/70 border border-slate-800 flex items-center justify-between text-xs font-mono">
            <div className="flex items-center gap-2">
              <span className="w-2.5 h-2.5 rounded-full bg-emerald-400 animate-pulse" />
              <span className="text-slate-300">Fleet Mesh Status</span>
            </div>
            <span className="text-emerald-400 font-semibold">100% ENCRYPTED</span>
          </div>
        </div>

        {/* Right Form Card (Mobile & Desktop) */}
        <div className="lg:col-span-7 w-full max-w-md mx-auto">
          {/* Mobile Header Branding */}
          <div className="lg:hidden flex flex-col items-center text-center mb-6">
            <div className="w-14 h-14 rounded-2xl bg-slate-900 border border-emerald-500/30 p-2 shadow-xl shadow-emerald-950/50 flex items-center justify-center mb-3">
              <Image
                src="/logo.png"
                alt="Smarthive Pool"
                width={48}
                height={48}
                className="object-contain"
              />
            </div>
            <h2 className="text-xl font-extrabold tracking-tight text-white">
              SMARTHIVE POOL
            </h2>
            <p className="text-[11px] text-emerald-400 font-semibold tracking-wider uppercase mt-0.5">
              Venue Console Sign In
            </p>
          </div>

          <div className="bg-slate-900/90 border border-slate-800/90 backdrop-blur-xl p-6 sm:p-8 shadow-2xl rounded-3xl">
            <div className="mb-6">
              <h2 className="text-xl font-bold text-white tracking-tight">
                Welcome Back
              </h2>
              <p className="text-xs text-slate-400 mt-1">
                Enter your credentials to access your table fleet and live audit logs.
              </p>
            </div>

            {errorMsg && (
              <div className="mb-5 flex items-center gap-2.5 p-3 rounded-xl bg-rose-950/50 border border-rose-800 text-rose-300 text-xs">
                <AlertTriangle className="w-4 h-4 flex-shrink-0 text-rose-400" />
                <span>{errorMsg}</span>
              </div>
            )}

            {/* Google Auth Button */}
            <button
              type="button"
              onClick={handleGoogleSignIn}
              disabled={isGoogleSubmitting}
              className="w-full flex items-center justify-center gap-3 py-2.5 px-4 rounded-xl border border-slate-700/80 bg-slate-800/80 hover:bg-slate-800 text-xs font-semibold text-white shadow-sm transition-all hover:border-slate-600 mb-5 disabled:opacity-50"
            >
              <svg className="w-4 h-4" viewBox="0 0 24 24">
                <path
                  fill="#4285F4"
                  d="M23.745 12.27c0-.7-.06-1.4-.19-2.07H12v4.51h6.6c-.29 1.52-1.14 2.8-2.4 3.65v3.05h3.88c2.27-2.09 3.665-5.17 3.665-9.14z"
                />
                <path
                  fill="#34A853"
                  d="M12 24c3.24 0 5.95-1.08 7.93-2.91l-3.88-3.05c-1.08.72-2.45 1.16-4.05 1.16-3.12 0-5.77-2.1-6.72-4.93H1.25v3.15C3.26 21.36 7.33 24 12 24z"
                />
                <path
                  fill="#FBBC05"
                  d="M5.28 14.27c-.25-.72-.38-1.49-.38-2.27s.13-1.55.38-2.27V6.58H1.25C.45 8.18 0 9.99 0 12s.45 3.82 1.25 5.42l4.03-3.15z"
                />
                <path
                  fill="#EA4335"
                  d="M12 4.75c1.77 0 3.35.61 4.6 1.8l3.42-3.42C17.95 1.19 15.24 0 12 0 7.33 0 3.26 2.64 1.25 6.58l4.03 3.15c.95-2.83 3.6-4.98 6.72-4.98z"
                />
              </svg>
              <span>{isGoogleSubmitting ? "Connecting to Google..." : "Continue with Google"}</span>
            </button>

            <div className="relative mb-5">
              <div className="absolute inset-0 flex items-center">
                <div className="w-full border-t border-slate-800" />
              </div>
              <div className="relative flex justify-center text-xs uppercase">
                <span className="bg-slate-900 px-3 text-slate-500 font-mono text-[10px]">
                  Or sign in with email
                </span>
              </div>
            </div>

            {/* Email / Password Form */}
            <form onSubmit={handleSubmit} className="space-y-4">
              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">
                  Manager / Owner Email
                </label>
                <div className="relative">
                  <input
                    type="email"
                    required
                    placeholder="owner@smarthivetech.com"
                    value={email}
                    onChange={(e) => setEmail(e.target.value)}
                    className="w-full pl-9 pr-3 py-2.5 rounded-xl bg-slate-800/80 border border-slate-700 text-xs text-white placeholder-slate-500 focus:outline-none focus:border-emerald-500 transition-colors"
                  />
                  <Mail className="w-4 h-4 text-slate-500 absolute left-3 top-3" />
                </div>
              </div>

              <div>
                <div className="flex items-center justify-between mb-1">
                  <label className="block text-xs font-medium text-slate-300">
                    Password
                  </label>
                  <button
                    type="button"
                    onClick={() => setShowPassword(!showPassword)}
                    className="text-[11px] text-slate-400 hover:text-slate-200 flex items-center gap-1"
                  >
                    {showPassword ? (
                      <>
                        <EyeOff className="w-3 h-3" />
                        <span>Hide</span>
                      </>
                    ) : (
                      <>
                        <Eye className="w-3 h-3" />
                        <span>Show</span>
                      </>
                    )}
                  </button>
                </div>
                <div className="relative">
                  <input
                    type={showPassword ? "text" : "password"}
                    required
                    placeholder="••••••••••••"
                    value={password}
                    onChange={(e) => setPassword(e.target.value)}
                    className="w-full pl-9 pr-10 py-2.5 rounded-xl bg-slate-800/80 border border-slate-700 text-xs text-white placeholder-slate-500 focus:outline-none focus:border-emerald-500 transition-colors"
                  />
                  <Lock className="w-4 h-4 text-slate-500 absolute left-3 top-3" />
                </div>
              </div>

              <div className="flex items-center justify-between text-xs pt-1">
                <label className="flex items-center gap-2 text-slate-400 cursor-pointer select-none">
                  <input
                    type="checkbox"
                    defaultChecked
                    className="rounded bg-slate-800 border-slate-700 text-emerald-500 focus:ring-emerald-500"
                  />
                  <span>Remember this terminal</span>
                </label>
                <Link
                  href="/auth/register"
                  className="text-xs text-emerald-400 hover:text-emerald-300 font-medium"
                >
                  Create Account
                </Link>
              </div>

              <button
                type="submit"
                disabled={isSubmitting}
                className="w-full flex items-center justify-center gap-2 py-2.5 px-4 rounded-xl bg-emerald-600 hover:bg-emerald-500 text-xs font-bold text-white shadow-lg shadow-emerald-950/60 transition-all disabled:opacity-50 mt-2"
              >
                <span>
                  {isSubmitting
                    ? "Authenticating..."
                    : "Access Venue Console"}
                </span>
                <ArrowRight className="w-4 h-4" />
              </button>
            </form>

            {/* Footer Links */}
            <div className="mt-6 pt-5 border-t border-slate-800 text-center text-xs text-slate-400">
              <span>Don&apos;t have a venue account yet? </span>
              <Link
                href="/auth/register"
                className="text-emerald-400 font-semibold hover:text-emerald-300 hover:underline"
              >
                Create Account
              </Link>
            </div>

            <div className="mt-4 flex items-center justify-center gap-2 text-[10px] text-slate-500">
              <ShieldCheck className="w-3.5 h-3.5 text-emerald-400" />
              <span>TLS 1.3 / AES Hardware Node Encrypted Session</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
