"use client";

import React, { useEffect, useState } from "react";
import Image from "next/image";
import { signOut, useSession } from "next-auth/react";
import {
  User,
  Phone,
  Building,
  MapPin,
  Layers,
  Store,
  ArrowRight,
  AlertTriangle,
  LogOut,
  ShieldCheck,
  Loader2,
} from "lucide-react";

const inputClass =
  "w-full pl-9 pr-3 py-2.5 rounded-xl bg-slate-800/80 border border-slate-700 text-sm sm:text-xs text-white placeholder-slate-500 focus:outline-none focus:border-emerald-500 transition-colors";

/**
 * Blocking modal shown to Google (OAuth) users after sign-in/sign-up
 * to collect the profile details the email registration form normally collects.
 */
export function OnboardingModal() {
  const { data: session, update } = useSession();

  const [name, setName] = useState(session?.user?.name || "");
  const [phoneNumber, setPhoneNumber] = useState("");
  const [companyName, setCompanyName] = useState("");
  const [activeVenue, setActiveVenue] = useState("");
  const [city, setCity] = useState("");
  const [tableCount, setTableCount] = useState<number | "">("");
  const [loadingProfile, setLoadingProfile] = useState(true);
  const [busy, setBusy] = useState(false);
  const [errorMsg, setErrorMsg] = useState<string | null>(null);

  // Prefill with anything already stored on the account
  useEffect(() => {
    let cancelled = false;
    (async () => {
      try {
        const res = await fetch("/api/auth/onboarding", { cache: "no-store" });
        const data = await res.json();
        if (!cancelled && data.ok) {
          const p = data.profile;
          if (p.name) setName(p.name);
          setPhoneNumber(p.phoneNumber || "");
          setCompanyName(p.companyName || "");
          setActiveVenue(p.activeVenue && p.activeVenue !== "Main Hall" ? p.activeVenue : "");
          setCity(p.city || "");
          setTableCount(p.tableCount || "");
        }
      } catch {
        // Prefill is best-effort
      } finally {
        if (!cancelled) setLoadingProfile(false);
      }
    })();
    return () => {
      cancelled = true;
    };
  }, []);

  // Lock background scroll while the modal is open
  useEffect(() => {
    const prev = document.body.style.overflow;
    document.body.style.overflow = "hidden";
    return () => {
      document.body.style.overflow = prev;
    };
  }, []);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setBusy(true);
    setErrorMsg(null);
    try {
      const res = await fetch("/api/auth/onboarding", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          name,
          phoneNumber,
          companyName,
          activeVenue,
          city,
          tableCount: Number(tableCount) || 0,
        }),
      });
      const data = await res.json();
      if (!data.ok) {
        setErrorMsg(data.error || "Failed to save your details");
        setBusy(false);
        return;
      }
      // Server re-reads the profile from the DB on update and clears the onboarding flag
      const updated = await update({ refresh: true });
      if (updated?.user?.needsOnboarding !== false) {
        // Fallback: hard reload so the JWT callback re-checks the database
        window.location.replace("/");
      }
    } catch (err: unknown) {
      setErrorMsg(err instanceof Error ? err.message : "Network error");
      setBusy(false);
    }
  };

  const initials = (name || session?.user?.email || "U")
    .split(" ")
    .map((n) => n[0])
    .join("")
    .slice(0, 2)
    .toUpperCase();

  return (
    <div
      className="fixed inset-0 z-[100] flex items-end sm:items-center justify-center bg-slate-950/90 backdrop-blur-md sm:p-4"
      role="dialog"
      aria-modal="true"
      aria-labelledby="onboarding-title"
    >
      <div className="absolute top-1/4 left-1/2 -translate-x-1/2 -translate-y-1/2 w-[500px] h-[500px] bg-emerald-500/10 rounded-full blur-[140px] pointer-events-none" />

      <div className="relative w-full sm:max-w-lg max-h-[100dvh] sm:max-h-[92vh] flex flex-col bg-slate-900 border border-slate-800 sm:rounded-3xl rounded-t-3xl shadow-2xl overflow-hidden">
        {/* Header */}
        <div className="px-5 sm:px-7 pt-6 pb-4 border-b border-slate-800/80 flex-shrink-0">
          <div className="flex items-center gap-3 mb-4">
            <div className="w-10 h-10 rounded-xl bg-slate-800 border border-emerald-500/40 p-1.5 flex items-center justify-center flex-shrink-0">
              <Image src="/logo.png" alt="Smarthive Pool" width={28} height={28} className="object-contain" />
            </div>
            <div className="min-w-0">
              <p className="text-[11px] text-emerald-400 font-semibold tracking-wider uppercase">
                Step 2 of 2
              </p>
              <h2 id="onboarding-title" className="text-lg font-bold text-white tracking-tight">
                Complete your venue profile
              </h2>
            </div>
          </div>

          <div className="flex items-center gap-3 p-2.5 rounded-xl bg-slate-950/60 border border-slate-800">
            {session?.user?.image ? (
              <Image
                src={session.user.image}
                alt=""
                width={32}
                height={32}
                unoptimized
                className="w-8 h-8 rounded-lg object-cover flex-shrink-0"
              />
            ) : (
              <div className="w-8 h-8 rounded-lg bg-emerald-600/20 border border-emerald-500/40 text-emerald-300 flex items-center justify-center font-bold text-xs flex-shrink-0">
                {initials}
              </div>
            )}
            <div className="min-w-0 flex-1">
              <div className="text-[10px] text-slate-500">Signed in with Google</div>
              <div className="text-xs text-slate-200 font-medium truncate">{session?.user?.email}</div>
            </div>
            <ShieldCheck className="w-4 h-4 text-emerald-400 flex-shrink-0" />
          </div>
        </div>

        {/* Form */}
        <form onSubmit={handleSubmit} className="flex-1 flex flex-col min-h-0">
          <div className="flex-1 overflow-y-auto px-5 sm:px-7 py-5 space-y-3.5">
            <p className="text-xs text-slate-400">
              We need a few more details to set up your arena dashboard and send you table alerts.
            </p>

            {errorMsg && (
              <div className="flex items-center gap-2.5 p-3 rounded-xl bg-rose-950/50 border border-rose-800 text-rose-300 text-xs">
                <AlertTriangle className="w-4 h-4 flex-shrink-0 text-rose-400" />
                <span>{errorMsg}</span>
              </div>
            )}

            <fieldset disabled={loadingProfile || busy} className="space-y-3.5 disabled:opacity-70">
              <div>
                <label htmlFor="ob-name" className="block text-xs font-medium text-slate-300 mb-1">
                  Full Name <span className="text-rose-400">*</span>
                </label>
                <div className="relative">
                  <input
                    id="ob-name"
                    type="text"
                    required
                    autoComplete="name"
                    placeholder="e.g. Adebayo Adeleke"
                    value={name}
                    onChange={(e) => setName(e.target.value)}
                    className={inputClass}
                  />
                  <User className="w-4 h-4 text-slate-500 absolute left-3 top-1/2 -translate-y-1/2" />
                </div>
              </div>

              <div>
                <label htmlFor="ob-phone" className="block text-xs font-medium text-slate-300 mb-1">
                  Phone Number (for alerts) <span className="text-rose-400">*</span>
                </label>
                <div className="relative">
                  <input
                    id="ob-phone"
                    type="tel"
                    required
                    autoComplete="tel"
                    inputMode="tel"
                    placeholder="+234 801 234 5678"
                    value={phoneNumber}
                    onChange={(e) => setPhoneNumber(e.target.value)}
                    className={inputClass}
                  />
                  <Phone className="w-4 h-4 text-slate-500 absolute left-3 top-1/2 -translate-y-1/2" />
                </div>
              </div>

              <div>
                <label htmlFor="ob-company" className="block text-xs font-medium text-slate-300 mb-1">
                  Arena / Club Name <span className="text-rose-400">*</span>
                </label>
                <div className="relative">
                  <input
                    id="ob-company"
                    type="text"
                    required
                    autoComplete="organization"
                    placeholder="Grand Pool Lounge"
                    value={companyName}
                    onChange={(e) => setCompanyName(e.target.value)}
                    className={inputClass}
                  />
                  <Building className="w-4 h-4 text-slate-500 absolute left-3 top-1/2 -translate-y-1/2" />
                </div>
              </div>

              <div className="grid grid-cols-1 sm:grid-cols-2 gap-3.5 sm:gap-3">
                <div>
                  <label htmlFor="ob-venue" className="block text-xs font-medium text-slate-300 mb-1">
                    Main Venue / Branch <span className="text-rose-400">*</span>
                  </label>
                  <div className="relative">
                    <input
                      id="ob-venue"
                      type="text"
                      required
                      placeholder="e.g. Lekki Branch"
                      value={activeVenue}
                      onChange={(e) => setActiveVenue(e.target.value)}
                      className={inputClass}
                    />
                    <Store className="w-4 h-4 text-slate-500 absolute left-3 top-1/2 -translate-y-1/2" />
                  </div>
                </div>
                <div>
                  <label htmlFor="ob-city" className="block text-xs font-medium text-slate-300 mb-1">
                    City <span className="text-rose-400">*</span>
                  </label>
                  <div className="relative">
                    <input
                      id="ob-city"
                      type="text"
                      required
                      autoComplete="address-level2"
                      placeholder="e.g. Lagos"
                      value={city}
                      onChange={(e) => setCity(e.target.value)}
                      className={inputClass}
                    />
                    <MapPin className="w-4 h-4 text-slate-500 absolute left-3 top-1/2 -translate-y-1/2" />
                  </div>
                </div>
              </div>

              <div>
                <label htmlFor="ob-tables" className="block text-xs font-medium text-slate-300 mb-1">
                  Number of Tables
                </label>
                <div className="relative">
                  <input
                    id="ob-tables"
                    type="number"
                    min={0}
                    max={500}
                    inputMode="numeric"
                    placeholder="e.g. 6"
                    value={tableCount}
                    onChange={(e) => setTableCount(e.target.value === "" ? "" : Number(e.target.value))}
                    className={inputClass}
                  />
                  <Layers className="w-4 h-4 text-slate-500 absolute left-3 top-1/2 -translate-y-1/2" />
                </div>
              </div>
            </fieldset>
          </div>

          {/* Footer */}
          <div className="flex-shrink-0 px-5 sm:px-7 py-4 border-t border-slate-800/80 bg-slate-900 flex flex-col-reverse sm:flex-row sm:items-center sm:justify-between gap-3 pb-[max(1rem,env(safe-area-inset-bottom))]">
            <button
              type="button"
              onClick={() => signOut({ callbackUrl: "/auth/signin" })}
              className="flex items-center justify-center gap-1.5 text-xs text-slate-400 hover:text-rose-400 transition-colors py-2"
            >
              <LogOut className="w-3.5 h-3.5" />
              <span>Not you? Sign out</span>
            </button>
            <button
              type="submit"
              disabled={busy || loadingProfile}
              className="flex items-center justify-center gap-2 py-3 sm:py-2.5 px-5 rounded-xl bg-emerald-600 hover:bg-emerald-500 text-sm sm:text-xs font-bold text-white shadow-lg shadow-emerald-950/60 transition-all disabled:opacity-50"
            >
              {busy ? (
                <>
                  <Loader2 className="w-4 h-4 animate-spin" />
                  <span>Setting up your arena...</span>
                </>
              ) : (
                <>
                  <span>Continue to Dashboard</span>
                  <ArrowRight className="w-4 h-4" />
                </>
              )}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}

