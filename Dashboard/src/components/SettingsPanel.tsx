"use client";

import React, { useEffect, useState } from "react";
import Image from "next/image";
import { useSession } from "next-auth/react";
import {
  User,
  Phone,
  Building,
  MapPin,
  Mail,
  Save,
  Loader2,
  Plus,
  Pencil,
  Trash2,
  Check,
  X,
  Star,
  BarChart3,
  Server,
  Coins,
  Activity,
  ShieldCheck,
  AlertTriangle,
  ArrowRightLeft,
  Layers,
} from "lucide-react";
import type { ZoneSummary } from "@/components/ZoneSwitcher";

interface Profile {
  name: string;
  email: string;
  image: string;
  phoneNumber: string;
  companyName: string;
  city: string;
  role: string;
  subscriptionPlan: string;
  subscriptionStatus: string;
  hasPassword: boolean;
}

interface SettingsPanelProps {
  zones: ZoneSummary[];
  selectedZone: string;
  /** Called with the fresh zone list after any zone mutation. */
  onZonesChanged: (zones: ZoneSummary[]) => void;
  /** Switch the dashboard to a zone and open its report. */
  onViewZone: (zone: string) => void;
  notify: (text: string, type?: "success" | "error") => void;
}

const inputClass =
  "w-full pl-9 pr-3 py-2.5 rounded-xl bg-slate-800/80 border border-slate-700 text-sm sm:text-xs text-white placeholder-slate-500 focus:outline-none focus:border-emerald-500 transition-colors disabled:opacity-60";

const formatNaira = (n: number) => `₦${n.toLocaleString()}`;

export function SettingsPanel({ zones, selectedZone, onZonesChanged, onViewZone, notify }: SettingsPanelProps) {
  const { update } = useSession();

  // ---------- Profile ----------
  const [profile, setProfile] = useState<Profile | null>(null);
  const [name, setName] = useState("");
  const [phoneNumber, setPhoneNumber] = useState("");
  const [companyName, setCompanyName] = useState("");
  const [city, setCity] = useState("");
  const [profileBusy, setProfileBusy] = useState(false);
  const [profileError, setProfileError] = useState<string | null>(null);

  // ---------- Zones ----------
  const [primaryZone, setPrimaryZone] = useState<string>("");
  const [newZone, setNewZone] = useState("");
  const [zoneBusy, setZoneBusy] = useState<string | null>(null);
  const [zoneError, setZoneError] = useState<string | null>(null);
  const [renaming, setRenaming] = useState<string | null>(null);
  const [renameValue, setRenameValue] = useState("");
  const [deleting, setDeleting] = useState<string | null>(null);
  const [moveTo, setMoveTo] = useState("");

  useEffect(() => {
    (async () => {
      try {
        const [pRes, zRes] = await Promise.all([
          fetch("/api/profile", { cache: "no-store" }),
          fetch("/api/zones", { cache: "no-store" }),
        ]);
        const p = await pRes.json();
        const z = await zRes.json();
        if (p.ok) {
          setProfile(p.profile);
          setName(p.profile.name);
          setPhoneNumber(p.profile.phoneNumber);
          setCompanyName(p.profile.companyName);
          setCity(p.profile.city);
        }
        if (z.ok) {
          setPrimaryZone(z.primaryZone);
          onZonesChanged(z.zones);
        }
      } catch {
        setProfileError("Failed to load your profile");
      }
    })();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const profileDirty =
    !!profile &&
    (name !== profile.name ||
      phoneNumber !== profile.phoneNumber ||
      companyName !== profile.companyName ||
      city !== profile.city);

  const saveProfile = async (e: React.FormEvent) => {
    e.preventDefault();
    setProfileBusy(true);
    setProfileError(null);
    try {
      const res = await fetch("/api/profile", {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ name, phoneNumber, companyName, city }),
      });
      const data = await res.json();
      if (!data.ok) {
        setProfileError(data.error || "Failed to save profile");
        return;
      }
      setProfile((p) =>
        p ? { ...p, name: name.trim(), phoneNumber: phoneNumber.trim(), companyName: companyName.trim(), city: city.trim() } : p
      );
      await update({ refresh: true }); // refresh name/company shown in the sidebar
      notify("Profile updated");
    } catch (err: unknown) {
      setProfileError(err instanceof Error ? err.message : "Network error");
    } finally {
      setProfileBusy(false);
    }
  };

  const zoneAction = async (key: string, payload: Record<string, unknown>) => {
    setZoneBusy(key);
    setZoneError(null);
    try {
      const res = await fetch("/api/zones", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload),
      });
      const data = await res.json();
      if (!data.ok) {
        setZoneError(data.error || "Zone action failed");
        return false;
      }
      if (data.zones) onZonesChanged(data.zones);
      if (data.primaryZone) setPrimaryZone(data.primaryZone);
      if (data.message) notify(data.message);
      return true;
    } catch (err: unknown) {
      setZoneError(err instanceof Error ? err.message : "Network error");
      return false;
    } finally {
      setZoneBusy(null);
    }
  };

  const handleCreateZone = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!newZone.trim()) return;
    if (await zoneAction("create", { action: "create", name: newZone })) setNewZone("");
  };

  const handleRename = async (zone: string) => {
    if (!renameValue.trim() || renameValue.trim() === zone) {
      setRenaming(null);
      return;
    }
    if (await zoneAction(`rename:${zone}`, { action: "rename", name: zone, newName: renameValue })) {
      setRenaming(null);
    }
  };

  const handleDelete = async (zone: ZoneSummary) => {
    const ok = await zoneAction(`delete:${zone.name}`, {
      action: "delete",
      name: zone.name,
      moveTo: zone.tableCount > 0 ? moveTo : undefined,
    });
    if (ok) setDeleting(null);
  };

  const initials = (name || profile?.email || "U")
    .split(" ")
    .map((n) => n[0])
    .join("")
    .slice(0, 2)
    .toUpperCase();

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-lg sm:text-xl font-bold text-white tracking-tight">Profile &amp; Settings</h1>
        <p className="text-xs text-slate-400 mt-1">
          Manage your account details and the zones (venues/halls) your tables are grouped into.
        </p>
      </div>

      <div className="grid grid-cols-1 xl:grid-cols-5 gap-6">
        {/* ---------------- Profile ---------------- */}
        <section className="xl:col-span-2 p-5 rounded-2xl bg-slate-900/80 border border-slate-800/90 space-y-5 h-fit">
          <div className="flex items-center gap-3">
            {profile?.image ? (
              <Image
                src={profile.image}
                alt=""
                width={48}
                height={48}
                unoptimized
                className="w-12 h-12 rounded-xl object-cover flex-shrink-0"
              />
            ) : (
              <div className="w-12 h-12 rounded-xl bg-emerald-600/20 border border-emerald-500/40 text-emerald-300 flex items-center justify-center font-bold text-sm flex-shrink-0">
                {initials}
              </div>
            )}
            <div className="min-w-0 flex-1">
              <div className="text-sm font-semibold text-white truncate">{profile?.name || "Loading..."}</div>
              <div className="text-[11px] text-slate-400 truncate">{profile?.email}</div>
              {profile && (
                <div className="flex flex-wrap items-center gap-1.5 mt-1.5">
                  <span className="text-[10px] px-1.5 py-0.5 rounded bg-slate-800 border border-slate-700 text-slate-300 capitalize">
                    {profile.role}
                  </span>
                  <span className="text-[10px] px-1.5 py-0.5 rounded bg-emerald-500/10 border border-emerald-500/30 text-emerald-300 capitalize">
                    {profile.subscriptionPlan} · {profile.subscriptionStatus}
                  </span>
                </div>
              )}
            </div>
          </div>

          {profileError && (
            <div className="flex items-center gap-2 p-3 rounded-xl bg-rose-950/50 border border-rose-800 text-rose-300 text-xs">
              <AlertTriangle className="w-4 h-4 flex-shrink-0" />
              <span>{profileError}</span>
            </div>
          )}

          <form onSubmit={saveProfile} className="space-y-3.5">
            <fieldset disabled={!profile || profileBusy} className="space-y-3.5">
              <Field id="pf-name" label="Full Name" icon={<User className="w-4 h-4" />}>
                <input
                  id="pf-name"
                  required
                  value={name}
                  onChange={(e) => setName(e.target.value)}
                  autoComplete="name"
                  className={inputClass}
                />
              </Field>
              <Field id="pf-email" label="Email" icon={<Mail className="w-4 h-4" />}>
                <input id="pf-email" value={profile?.email || ""} disabled className={inputClass} />
              </Field>
              <Field id="pf-company" label="Arena / Club Name" icon={<Building className="w-4 h-4" />}>
                <input
                  id="pf-company"
                  required
                  value={companyName}
                  onChange={(e) => setCompanyName(e.target.value)}
                  autoComplete="organization"
                  className={inputClass}
                />
              </Field>
              <div className="grid grid-cols-1 sm:grid-cols-2 xl:grid-cols-1 2xl:grid-cols-2 gap-3.5">
                <Field id="pf-phone" label="Phone (alerts)" icon={<Phone className="w-4 h-4" />}>
                  <input
                    id="pf-phone"
                    type="tel"
                    inputMode="tel"
                    value={phoneNumber}
                    onChange={(e) => setPhoneNumber(e.target.value)}
                    placeholder="+234..."
                    autoComplete="tel"
                    className={inputClass}
                  />
                </Field>
                <Field id="pf-city" label="City" icon={<MapPin className="w-4 h-4" />}>
                  <input
                    id="pf-city"
                    value={city}
                    onChange={(e) => setCity(e.target.value)}
                    placeholder="e.g. Lagos"
                    autoComplete="address-level2"
                    className={inputClass}
                  />
                </Field>
              </div>
            </fieldset>

            <div className="flex flex-col-reverse sm:flex-row sm:items-center sm:justify-between gap-3 pt-1">
              <span className="flex items-center gap-1.5 text-[11px] text-slate-500">
                <ShieldCheck className="w-3.5 h-3.5 text-emerald-400" />
                {profile ? (profile.hasPassword ? "Email & password sign-in" : "Google sign-in") : ""}
              </span>
              <button
                type="submit"
                disabled={!profileDirty || profileBusy}
                className="flex items-center justify-center gap-2 py-2.5 px-4 rounded-xl bg-emerald-600 hover:bg-emerald-500 text-xs font-bold text-white shadow-md shadow-emerald-950/60 transition-all disabled:opacity-40 disabled:cursor-not-allowed"
              >
                {profileBusy ? <Loader2 className="w-4 h-4 animate-spin" /> : <Save className="w-4 h-4" />}
                <span>Save Profile</span>
              </button>
            </div>
          </form>
        </section>

        {/* ---------------- Zones ---------------- */}
        <section className="xl:col-span-3 p-5 rounded-2xl bg-slate-900/80 border border-slate-800/90 space-y-4">
          <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-2">
            <div>
              <h2 className="text-sm font-semibold text-white flex items-center gap-2">
                <Layers className="w-4 h-4 text-emerald-400" />
                Venues &amp; Zones
              </h2>
              <p className="text-[11px] text-slate-400 mt-0.5">
                Group tables by branch, hall or floor. Switch zones to see each zone&apos;s report.
              </p>
            </div>
            <span className="text-[11px] font-mono text-slate-500">{zones.length} zone(s)</span>
          </div>

          <form onSubmit={handleCreateZone} className="flex flex-col sm:flex-row gap-2">
            <div className="relative flex-1">
              <input
                value={newZone}
                onChange={(e) => setNewZone(e.target.value)}
                maxLength={50}
                placeholder="New zone name, e.g. VIP Hall or Ikeja Branch"
                aria-label="New zone name"
                className={inputClass}
              />
              <Building className="w-4 h-4 text-slate-500 absolute left-3 top-1/2 -translate-y-1/2" />
            </div>
            <button
              type="submit"
              disabled={!newZone.trim() || zoneBusy === "create"}
              className="flex items-center justify-center gap-1.5 py-2.5 px-4 rounded-xl bg-emerald-600 hover:bg-emerald-500 text-xs font-bold text-white transition-all disabled:opacity-40"
            >
              {zoneBusy === "create" ? <Loader2 className="w-4 h-4 animate-spin" /> : <Plus className="w-4 h-4" />}
              <span>Add Zone</span>
            </button>
          </form>

          {zoneError && (
            <div className="flex items-center gap-2 p-3 rounded-xl bg-rose-950/50 border border-rose-800 text-rose-300 text-xs">
              <AlertTriangle className="w-4 h-4 flex-shrink-0" />
              <span className="flex-1">{zoneError}</span>
              <button onClick={() => setZoneError(null)} aria-label="Dismiss" className="p-0.5 hover:text-white">
                <X className="w-3.5 h-3.5" />
              </button>
            </div>
          )}

          <ul className="space-y-2.5">
            {zones.map((z) => {
              const isPrimary = z.name === primaryZone;
              const isViewing = z.name === selectedZone;
              const busy = zoneBusy?.endsWith(`:${z.name}`);
              const others = zones.filter((o) => o.name !== z.name);

              return (
                <li
                  key={z.name}
                  className={`p-3.5 rounded-xl border transition-colors ${
                    isViewing ? "bg-emerald-950/20 border-emerald-700/50" : "bg-slate-950/50 border-slate-800"
                  }`}
                >
                  <div className="flex flex-col sm:flex-row sm:items-center gap-3">
                    {/* Name / rename */}
                    <div className="flex-1 min-w-0">
                      {renaming === z.name ? (
                        <form
                          onSubmit={(e) => {
                            e.preventDefault();
                            handleRename(z.name);
                          }}
                          className="flex items-center gap-1.5"
                        >
                          <input
                            autoFocus
                            value={renameValue}
                            onChange={(e) => setRenameValue(e.target.value)}
                            maxLength={50}
                            aria-label={`Rename ${z.name}`}
                            className="flex-1 min-w-0 px-2.5 py-1.5 rounded-lg bg-slate-800 border border-emerald-600 text-sm sm:text-xs text-white focus:outline-none"
                          />
                          <IconButton title="Save name" type="submit" disabled={busy} tone="emerald">
                            {busy ? <Loader2 className="w-3.5 h-3.5 animate-spin" /> : <Check className="w-3.5 h-3.5" />}
                          </IconButton>
                          <IconButton title="Cancel" onClick={() => setRenaming(null)}>
                            <X className="w-3.5 h-3.5" />
                          </IconButton>
                        </form>
                      ) : (
                        <div className="flex items-center gap-2 flex-wrap">
                          <span className="text-sm font-semibold text-white truncate">{z.name}</span>
                          {isPrimary && (
                            <span className="flex items-center gap-1 text-[10px] px-1.5 py-0.5 rounded bg-amber-500/10 border border-amber-500/30 text-amber-300">
                              <Star className="w-2.5 h-2.5" /> Primary
                            </span>
                          )}
                          {isViewing && (
                            <span className="text-[10px] px-1.5 py-0.5 rounded bg-emerald-500/10 border border-emerald-500/30 text-emerald-300">
                              Viewing
                            </span>
                          )}
                        </div>
                      )}
                      <div className="mt-1.5 grid grid-cols-3 gap-2 text-[11px] text-slate-400 max-w-sm">
                        <span className="flex items-center gap-1">
                          <Server className="w-3 h-3 text-slate-500" />
                          <span className="font-mono text-slate-200">
                            {z.onlineCount}/{z.tableCount}
                          </span>
                          <span className="hidden sm:inline">online</span>
                        </span>
                        <span className="flex items-center gap-1">
                          <Activity className="w-3 h-3 text-slate-500" />
                          <span className="font-mono text-slate-200">{z.todayRacks}</span>
                          <span className="hidden sm:inline">racks</span>
                        </span>
                        <span className="flex items-center gap-1">
                          <Coins className="w-3 h-3 text-slate-500" />
                          <span className="font-mono text-emerald-300">{formatNaira(z.todayRevenue)}</span>
                        </span>
                      </div>
                    </div>

                    {/* Actions */}
                    {renaming !== z.name && (
                      <div className="flex items-center gap-1.5 flex-shrink-0">
                        <button
                          type="button"
                          onClick={() => onViewZone(z.name)}
                          className="flex-1 sm:flex-none flex items-center justify-center gap-1.5 px-3 py-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 border border-slate-700 text-[11px] font-semibold text-emerald-300 transition-colors"
                        >
                          <BarChart3 className="w-3.5 h-3.5" />
                          <span>View Report</span>
                        </button>
                        {!isPrimary && (
                          <IconButton
                            title="Set as primary zone"
                            disabled={busy}
                            onClick={() => zoneAction(`primary:${z.name}`, { action: "set_primary", name: z.name })}
                          >
                            <Star className="w-3.5 h-3.5" />
                          </IconButton>
                        )}
                        <IconButton
                          title="Rename zone"
                          onClick={() => {
                            setDeleting(null);
                            setRenaming(z.name);
                            setRenameValue(z.name);
                          }}
                        >
                          <Pencil className="w-3.5 h-3.5" />
                        </IconButton>
                        <IconButton
                          title={zones.length <= 1 ? "You must keep at least one zone" : "Delete zone"}
                          tone="rose"
                          disabled={zones.length <= 1}
                          onClick={() => {
                            setRenaming(null);
                            setDeleting(deleting === z.name ? null : z.name);
                            setMoveTo(others.find((o) => o.name === primaryZone)?.name || others[0]?.name || "");
                          }}
                        >
                          <Trash2 className="w-3.5 h-3.5" />
                        </IconButton>
                      </div>
                    )}
                  </div>

                  {/* Delete confirmation */}
                  {deleting === z.name && (
                    <div className="mt-3 p-3 rounded-lg bg-rose-950/30 border border-rose-900/60 space-y-2.5">
                      <p className="text-[11px] text-rose-200">
                        {z.tableCount > 0
                          ? `This zone has ${z.tableCount} table(s). Choose where to move them and their history before deleting.`
                          : "Delete this empty zone? This cannot be undone."}
                      </p>
                      <div className="flex flex-col sm:flex-row sm:items-center gap-2">
                        {z.tableCount > 0 && (
                          <label className="flex items-center gap-2 flex-1 text-[11px] text-slate-300">
                            <ArrowRightLeft className="w-3.5 h-3.5 text-slate-400 flex-shrink-0" />
                            <span className="flex-shrink-0">Move to</span>
                            <select
                              value={moveTo}
                              onChange={(e) => setMoveTo(e.target.value)}
                              className="flex-1 min-w-0 px-2 py-1.5 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                            >
                              {others.map((o) => (
                                <option key={o.name} value={o.name}>
                                  {o.name}
                                </option>
                              ))}
                            </select>
                          </label>
                        )}
                        <div className="flex items-center gap-2 sm:ml-auto">
                          <button
                            type="button"
                            onClick={() => setDeleting(null)}
                            className="flex-1 sm:flex-none px-3 py-1.5 rounded-lg text-[11px] text-slate-300 hover:bg-slate-800 border border-slate-700"
                          >
                            Cancel
                          </button>
                          <button
                            type="button"
                            disabled={busy || (z.tableCount > 0 && !moveTo)}
                            onClick={() => handleDelete(z)}
                            className="flex-1 sm:flex-none flex items-center justify-center gap-1.5 px-3 py-1.5 rounded-lg bg-rose-600 hover:bg-rose-500 text-[11px] font-semibold text-white disabled:opacity-50"
                          >
                            {busy ? <Loader2 className="w-3.5 h-3.5 animate-spin" /> : <Trash2 className="w-3.5 h-3.5" />}
                            <span>Delete Zone</span>
                          </button>
                        </div>
                      </div>
                    </div>
                  )}
                </li>
              );
            })}
          </ul>
        </section>
      </div>
    </div>
  );
}

function Field({
  id,
  label,
  icon,
  children,
}: {
  id: string;
  label: string;
  icon: React.ReactNode;
  children: React.ReactNode;
}) {
  return (
    <div>
      <label htmlFor={id} className="block text-xs font-medium text-slate-300 mb-1">
        {label}
      </label>
      <div className="relative">
        {children}
        <span className="text-slate-500 absolute left-3 top-1/2 -translate-y-1/2 pointer-events-none">{icon}</span>
      </div>
    </div>
  );
}

function IconButton({
  title,
  children,
  onClick,
  disabled,
  type = "button",
  tone = "slate",
}: {
  title: string;
  children: React.ReactNode;
  onClick?: () => void;
  disabled?: boolean;
  type?: "button" | "submit";
  tone?: "slate" | "rose" | "emerald";
}) {
  const tones = {
    slate: "text-slate-400 hover:text-white hover:bg-slate-800",
    rose: "text-slate-400 hover:text-rose-400 hover:bg-rose-950/40",
    emerald: "text-emerald-400 hover:bg-emerald-950/40",
  };
  return (
    <button
      type={type}
      title={title}
      aria-label={title}
      onClick={onClick}
      disabled={disabled}
      className={`p-2 rounded-lg border border-slate-800 transition-colors disabled:opacity-40 disabled:cursor-not-allowed ${tones[tone]}`}
    >
      {children}
    </button>
  );
}

