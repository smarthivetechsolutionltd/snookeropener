"use client";

import React, { useState, useEffect, useCallback } from "react";
import Link from "next/link";
import { useSession, signOut } from "next-auth/react";
import {
  Shield,
  ShieldAlert,
  ShieldCheck,
  LayoutDashboard,
  Users,
  Server,
  Cpu,
  AlertTriangle,
  CheckCircle2,
  RefreshCw,
  Search,
  Filter,
  Plus,
  Trash2,
  Lock,
  Unlock,
  Activity,
  Wifi,
  UploadCloud,
  Download,
  KeyRound,
  Coins,
  Sliders,
  X,
  ChevronRight,
  Clock,
  ArrowUpRight,
  LogOut,
  Check,
  Building,
  Radio,
  FileCode,
  Layers,
} from "lucide-react";

interface AdminKpi {
  totalUsers: number;
  ownersCount: number;
  managersCount: number;
  attendantsCount: number;
  adminsCount: number;
  totalDevices: number;
  onlineDevices: number;
  offlineDevices: number;
  maintenanceLocked: number;
  unclaimedDevices: number;
  totalCards: number;
  totalLifetimeRacks: number;
  totalGames7d: number;
  totalRevenue7d: number;
  unresolvedErrors: number;
}

interface OverviewData {
  caller: { id: string; email: string; name: string; role: string };
  kpi: AdminKpi;
  trends: {
    daily: { date: string; games: number; revenue: number }[];
    hourly: { hour: number; hourLabel: string; games: number }[];
    triggers: Record<string, number>;
    rssi: { good: number; fair: number; weak: number };
    firmwares: Record<string, number>;
  };
  firmwareReleasesCount: number;
  recentErrors: ErrorItem[];
}

interface UserItem {
  _id: string;
  name: string;
  email: string;
  role: string;
  companyName: string;
  phoneNumber?: string;
  venues: string[];
  subscriptionPlan: string;
  subscriptionStatus: string;
  tableCount: number;
  createdAt: string;
}

interface DeviceItem {
  _id: string;
  deviceId: string;
  serialNumber: string;
  claimCode: string;
  isClaimed: boolean;
  ownerEmail?: string | null;
  name: string;
  venueName: string;
  location: string;
  firmwareVersion: string;
  lastHeartbeat: string | null;
  rssi: number;
  localIp: string;
  totalRacksLifetime: number;
  maintenanceLocked: boolean;
  isOnline: boolean;
  secondsSinceHeartbeat: number | null;
  otaCmd?: {
    pending: boolean;
    targetVersion: string;
    status: string;
    errorMessage?: string;
  };
}

interface FirmwareItem {
  _id: string;
  version: string;
  targetHardware: string;
  fileName: string;
  downloadUrl: string;
  fileSizeBytes: number;
  releaseNotes: string;
  isDefault: boolean;
  isActive: boolean;
  activeDeviceCount: number;
  createdAt: string;
}

interface ErrorItem {
  _id: string;
  type: string;
  severity: "critical" | "warning" | "info";
  message: string;
  details?: string;
  deviceId?: string;
  venueName?: string;
  ownerEmail?: string;
  resolved: boolean;
  createdAt: string;
}

type TabType = "metrics" | "users" | "devices" | "firmware" | "diagnostics";

export default function AdminPage() {
  const { data: session, status, update: updateSession } = useSession();
  const [activeTab, setActiveTab] = useState<TabType>("metrics");
  const [loading, setLoading] = useState(true);
  const [authorized, setAuthorized] = useState<boolean | null>(null);

  // Overview Data
  const [overview, setOverview] = useState<OverviewData | null>(null);

  // Users Data
  const [users, setUsers] = useState<UserItem[]>([]);
  const [userQuery, setUserQuery] = useState("");
  const [userRoleFilter, setUserRoleFilter] = useState("all");
  const [editUserModal, setEditUserModal] = useState<UserItem | null>(null);
  const [editRole, setEditRole] = useState("owner");
  const [editPlan, setEditPlan] = useState("growth");
  const [editStatus, setEditStatus] = useState("active");

  // Devices Data
  const [devices, setDevices] = useState<DeviceItem[]>([]);
  const [deviceQuery, setDeviceQuery] = useState("");
  const [deviceStatusFilter, setDeviceStatusFilter] = useState("all");
  const [provisionModal, setProvisionModal] = useState(false);
  const [provisionName, setProvisionName] = useState("");
  const [provisionVenue, setProvisionVenue] = useState("");
  const [provisionLocation, setProvisionLocation] = useState("");
  const [provisionPrice, setProvisionPrice] = useState(500);
  const [provisionMsg, setProvisionMsg] = useState<{ serial: string; pin: string } | null>(null);

  // Firmware Data
  const [firmwares, setFirmwares] = useState<FirmwareItem[]>([]);
  const [uploadModal, setUploadModal] = useState(false);
  const [fwVersion, setFwVersion] = useState("v2.1.0-esp32");
  const [fwNotes, setFwNotes] = useState("");
  const [fwIsDefault, setFwIsDefault] = useState(false);
  const [fwFile, setFwFile] = useState<File | null>(null);
  const [fwUploading, setFwUploading] = useState(false);
  const [rolloutModal, setRolloutModal] = useState<FirmwareItem | null>(null);
  const [rolloutTargetDevice, setRolloutTargetDevice] = useState("*");

  // Diagnostics Data
  const [errors, setErrors] = useState<ErrorItem[]>([]);
  const [errorSeverityFilter, setErrorSeverityFilter] = useState("all");
  const [errorResolvedFilter, setErrorResolvedFilter] = useState("all");

  // Notifications & Admin Claim State
  const [notification, setNotification] = useState<{ text: string; type: "success" | "error" } | null>(null);
  const [claimSecretKey, setClaimSecretKey] = useState("");
  const [claimBusy, setClaimBusy] = useState(false);
  const [claimError, setClaimError] = useState("");

  const showNotification = (text: string, type: "success" | "error" = "success") => {
    setNotification({ text, type });
    setTimeout(() => setNotification(null), 3500);
  };

  const fetchOverview = useCallback(async () => {
    try {
      const res = await fetch("/api/admin/overview");
      if (res.status === 403 || res.status === 401) {
        setAuthorized(false);
        return;
      }
      const data = await res.json();
      if (data.ok) {
        setAuthorized(true);
        setOverview(data.data);
      } else {
        setAuthorized(false);
      }
    } catch {
      setAuthorized(false);
    }
  }, []);

  const fetchUsers = useCallback(async () => {
    try {
      const params = new URLSearchParams();
      if (userQuery) params.set("q", userQuery);
      if (userRoleFilter !== "all") params.set("role", userRoleFilter);
      const res = await fetch(`/api/admin/users?${params.toString()}`);
      const data = await res.json();
      if (data.ok) setUsers(data.users || []);
    } catch {
      // ignore
    }
  }, [userQuery, userRoleFilter]);

  const fetchDevices = useCallback(async () => {
    try {
      const params = new URLSearchParams();
      if (deviceQuery) params.set("q", deviceQuery);
      if (deviceStatusFilter !== "all") params.set("status", deviceStatusFilter);
      const res = await fetch(`/api/admin/devices?${params.toString()}`);
      const data = await res.json();
      if (data.ok) setDevices(data.devices || []);
    } catch {
      // ignore
    }
  }, [deviceQuery, deviceStatusFilter]);

  const fetchFirmwares = useCallback(async () => {
    try {
      const res = await fetch("/api/admin/firmwares");
      const data = await res.json();
      if (data.ok) setFirmwares(data.releases || []);
    } catch {
      // ignore
    }
  }, []);

  const fetchErrors = useCallback(async () => {
    try {
      const params = new URLSearchParams();
      if (errorSeverityFilter !== "all") params.set("severity", errorSeverityFilter);
      if (errorResolvedFilter !== "all") params.set("resolved", errorResolvedFilter);
      const res = await fetch(`/api/admin/errors?${params.toString()}`);
      const data = await res.json();
      if (data.ok) setErrors(data.logs || []);
    } catch {
      // ignore
    }
  }, [errorSeverityFilter, errorResolvedFilter]);

  const refreshAll = useCallback(async () => {
    setLoading(true);
    await fetchOverview();
    if (activeTab === "users") await fetchUsers();
    else if (activeTab === "devices") await fetchDevices();
    else if (activeTab === "firmware") await fetchFirmwares();
    else if (activeTab === "diagnostics") await fetchErrors();
    setLoading(false);
  }, [activeTab, fetchOverview, fetchUsers, fetchDevices, fetchFirmwares, fetchErrors]);

  useEffect(() => {
    refreshAll();
  }, [refreshAll]);

  // Load specific tab data on switch
  useEffect(() => {
    if (activeTab === "users") fetchUsers();
    if (activeTab === "devices") fetchDevices();
    if (activeTab === "firmware") fetchFirmwares();
    if (activeTab === "diagnostics") fetchErrors();
  }, [activeTab, fetchUsers, fetchDevices, fetchFirmwares, fetchErrors]);

  // Bootstrap/Claim Admin Handler
  const handleClaimAdmin = async (e: React.FormEvent) => {
    e.preventDefault();
    setClaimBusy(true);
    setClaimError("");
    try {
      const res = await fetch("/api/admin/claim", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ secretKey: claimSecretKey }),
      });
      const data = await res.json();
      if (data.ok) {
        showNotification("Admin privileges granted! Refreshing credentials...");
        await updateSession();
        window.location.reload();
      } else {
        setClaimError(data.error || "Failed to authorize admin claim.");
      }
    } catch {
      setClaimError("Network error while verifying admin authorization.");
    } finally {
      setClaimBusy(false);
    }
  };

  // User Actions
  const handleSaveUser = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!editUserModal) return;
    try {
      const res = await fetch("/api/admin/users", {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          userId: editUserModal._id,
          role: editRole,
          subscriptionPlan: editPlan,
          subscriptionStatus: editStatus,
        }),
      });
      const data = await res.json();
      if (data.ok) {
        showNotification(data.message);
        setEditUserModal(null);
        fetchUsers();
        fetchOverview();
      } else {
        showNotification(data.error || "Update failed", "error");
      }
    } catch {
      showNotification("Network error", "error");
    }
  };

  const handleDeleteUser = async (user: UserItem) => {
    if (!confirm(`Are you sure you want to permanently delete ${user.email}? Any assigned tables will be unlinked.`)) {
      return;
    }
    try {
      const res = await fetch(`/api/admin/users?userId=${user._id}`, { method: "DELETE" });
      const data = await res.json();
      if (data.ok) {
        showNotification(data.message);
        fetchUsers();
        fetchOverview();
      } else {
        showNotification(data.error || "Delete failed", "error");
      }
    } catch {
      showNotification("Network error", "error");
    }
  };

  // Device Actions
  const handleProvisionDevice = async (e: React.FormEvent) => {
    e.preventDefault();
    try {
      const res = await fetch("/api/admin/devices", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          name: provisionName,
          venueName: provisionVenue,
          location: provisionLocation,
          pricePerGame: provisionPrice,
        }),
      });
      const data = await res.json();
      if (data.ok) {
        setProvisionMsg({
          serial: data.device.serialNumber,
          pin: data.device.claimCode,
        });
        showNotification(data.message);
        fetchDevices();
        fetchOverview();
      } else {
        showNotification(data.error || "Provisioning failed", "error");
      }
    } catch {
      showNotification("Network error", "error");
    }
  };

  const handleDeviceAction = async (deviceId: string, action: string, extra?: Record<string, unknown>) => {
    try {
      const res = await fetch("/api/admin/devices", {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ deviceId, action, ...extra }),
      });
      const data = await res.json();
      if (data.ok) {
        showNotification(data.message);
        fetchDevices();
        fetchOverview();
      } else {
        showNotification(data.error || "Action failed", "error");
      }
    } catch {
      showNotification("Network error", "error");
    }
  };

  // Firmware Actions
  const handleUploadFirmware = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!fwFile) {
      showNotification("Please select a .bin firmware binary file.", "error");
      return;
    }
    setFwUploading(true);
    try {
      const formData = new FormData();
      formData.append("file", fwFile);
      formData.append("version", fwVersion);
      formData.append("releaseNotes", fwNotes);
      formData.append("isDefault", String(fwIsDefault));

      const res = await fetch("/api/admin/firmwares", {
        method: "POST",
        body: formData,
      });
      const data = await res.json();
      if (data.ok) {
        showNotification(data.message);
        setUploadModal(false);
        setFwFile(null);
        setFwNotes("");
        fetchFirmwares();
        fetchOverview();
      } else {
        showNotification(data.error || "Upload failed", "error");
      }
    } catch {
      showNotification("Network error", "error");
    } finally {
      setFwUploading(false);
    }
  };

  const handleSetDefaultFirmware = async (fw: FirmwareItem) => {
    try {
      const res = await fetch("/api/admin/firmwares", {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action: "set_default", releaseId: fw._id, version: fw.version }),
      });
      const data = await res.json();
      if (data.ok) {
        showNotification(data.message);
        fetchFirmwares();
      }
    } catch {
      showNotification("Network error", "error");
    }
  };

  const handleFleetRollout = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!rolloutModal) return;
    try {
      const res = await fetch("/api/admin/firmwares", {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          action: "fleet_rollout",
          releaseId: rolloutModal._id,
          version: rolloutModal.version,
          targetDeviceId: rolloutTargetDevice,
        }),
      });
      const data = await res.json();
      if (data.ok) {
        showNotification(data.message);
        setRolloutModal(null);
        fetchDevices();
        fetchOverview();
      } else {
        showNotification(data.error || "Rollout failed", "error");
      }
    } catch {
      showNotification("Network error", "error");
    }
  };

  const handleDeleteFirmware = async (fw: FirmwareItem) => {
    if (!confirm(`Are you sure you want to delete firmware release ${fw.version}?`)) return;
    try {
      const res = await fetch(`/api/admin/firmwares?releaseId=${fw._id}`, { method: "DELETE" });
      const data = await res.json();
      if (data.ok) {
        showNotification(data.message);
        fetchFirmwares();
        fetchOverview();
      } else {
        showNotification(data.error || "Delete failed", "error");
      }
    } catch {
      showNotification("Network error", "error");
    }
  };

  // Diagnostics Actions
  const handleToggleResolveError = async (logId: string) => {
    try {
      const res = await fetch("/api/admin/errors", {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action: "toggle_resolved", logId }),
      });
      const data = await res.json();
      if (data.ok) {
        showNotification(data.message);
        fetchErrors();
        fetchOverview();
      }
    } catch {
      showNotification("Network error", "error");
    }
  };

  const handleResolveAllErrors = async () => {
    try {
      const res = await fetch("/api/admin/errors", {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action: "resolve_all" }),
      });
      const data = await res.json();
      if (data.ok) {
        showNotification(data.message);
        fetchErrors();
        fetchOverview();
      }
    } catch {
      showNotification("Network error", "error");
    }
  };

  // 1. Loading State
  if (status === "loading" || (loading && authorized === null)) {
    return (
      <div className="min-h-screen bg-slate-950 flex flex-col items-center justify-center p-4">
        <div className="w-14 h-14 rounded-2xl bg-blue-500/10 border border-blue-500/30 flex items-center justify-center animate-pulse mb-4">
          <Shield className="w-7 h-7 text-blue-400" />
        </div>
        <p className="text-sm font-semibold text-white tracking-wide">Smarthive Pool Systems</p>
        <p className="text-xs text-slate-500 mt-1 font-mono">Verifying administrative access control...</p>
      </div>
    );
  }

  // 2. Access Denied State (User is not an admin)
  if (authorized === false) {
    return (
      <div className="min-h-screen bg-slate-950 flex items-center justify-center p-4">
        <div className="w-full max-w-md rounded-2xl bg-slate-900 border border-slate-800 p-6 sm:p-8 shadow-2xl text-center">
          <div className="w-16 h-16 rounded-2xl bg-rose-500/10 border border-rose-500/30 flex items-center justify-center mx-auto mb-4">
            <ShieldAlert className="w-8 h-8 text-rose-400" />
          </div>
          <h2 className="text-lg font-bold text-white">Administrator Access Required</h2>
          <p className="text-xs text-slate-400 mt-1.5 leading-relaxed">
            Your current account (<span className="font-mono text-slate-200">{session?.user?.email}</span>) does not have administrator privileges.
          </p>

          <div className="mt-6 p-4 rounded-xl bg-slate-800/60 border border-slate-700/80 text-left">
            <h4 className="text-xs font-semibold text-slate-200 flex items-center gap-1.5 mb-2">
              <KeyRound className="w-4 h-4 text-amber-400" />
              <span>Bootstrap Initial Administrator</span>
            </h4>
            <p className="text-[11px] text-slate-400 mb-3">
              If this is a new installation or you have the system master key, submit it below to authorize this account.
            </p>
            {claimError && (
              <div className="p-2 rounded bg-rose-950/70 border border-rose-500/40 text-[11px] text-rose-300 mb-3">
                {claimError}
              </div>
            )}
            <form onSubmit={handleClaimAdmin} className="space-y-2.5">
              <input
                type="password"
                placeholder="Enter Master Secret Key"
                value={claimSecretKey}
                onChange={(e) => setClaimSecretKey(e.target.value)}
                className="w-full px-3 py-2 rounded-lg bg-slate-900 border border-slate-700 text-xs text-white"
              />
              <button
                type="submit"
                disabled={claimBusy}
                className="w-full py-2 rounded-lg bg-blue-600 hover:bg-blue-500 text-white text-xs font-semibold shadow-md transition-all disabled:opacity-50"
              >
                {claimBusy ? "Verifying..." : "Authorize Admin Privileges"}
              </button>
            </form>
          </div>

          <div className="mt-6 pt-4 border-t border-slate-800">
            <Link
              href="/"
              className="inline-flex items-center gap-1.5 px-4 py-2 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-medium border border-slate-700 transition-all"
            >
              <ArrowUpRight className="w-3.5 h-3.5" />
              <span>Return to Club Manager</span>
            </Link>
          </div>
        </div>
      </div>
    );
  }

  const kpi = overview?.kpi;
  const trends = overview?.trends;

  return (
    <div className="min-h-screen bg-slate-950 text-slate-100 pb-20">
      {/* Toast Notification */}
      {notification && (
        <div
          className={`fixed top-4 right-4 z-50 px-4 py-2.5 rounded-xl text-xs font-medium shadow-2xl flex items-center gap-2 border ${
            notification.type === "success"
              ? "bg-emerald-950/90 border-emerald-500/50 text-emerald-200"
              : "bg-rose-950/90 border-rose-500/50 text-rose-200"
          }`}
        >
          {notification.type === "success" ? (
            <CheckCircle2 className="w-4 h-4 text-emerald-400" />
          ) : (
            <AlertTriangle className="w-4 h-4 text-rose-400" />
          )}
          <span>{notification.text}</span>
        </div>
      )}

      {/* Top Header */}
      <header className="sticky top-0 z-30 bg-slate-900/90 backdrop-blur-md border-b border-slate-800 px-4 sm:px-6 py-3">
        <div className="max-w-7xl mx-auto flex flex-col sm:flex-row sm:items-center justify-between gap-3">
          <div className="flex items-center gap-3">
            <div className="w-9 h-9 rounded-xl bg-blue-500/15 border border-blue-500/30 flex items-center justify-center">
              <Shield className="w-5 h-5 text-blue-400" />
            </div>
            <div>
              <div className="flex items-center gap-2">
                <h1 className="text-base font-bold text-white tracking-tight">Admin Console</h1>
                <span className="text-[10px] uppercase font-bold tracking-wider px-2 py-0.5 rounded bg-blue-500/20 text-blue-300 border border-blue-500/40">
                  {overview?.caller?.role || "Admin"}
                </span>
              </div>
              <p className="text-[11px] text-slate-400">
                Cross-tenant fleet telemetry, global metrics, firmware releases, and bug diagnostics
              </p>
            </div>
          </div>

          <div className="flex items-center gap-2 flex-wrap">
            <button
              onClick={refreshAll}
              disabled={loading}
              className="p-2 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-300 border border-slate-700 transition-colors"
              title="Refresh Global Data"
            >
              <RefreshCw className={`w-4 h-4 ${loading ? "animate-spin text-blue-400" : ""}`} />
            </button>

            {/* Switch to standard Club Manager */}
            <Link
              href="/"
              className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-emerald-700/80 hover:bg-emerald-600 text-white text-xs font-semibold shadow-sm border border-emerald-500/40 transition-all"
              title="Switch to Billiards Club Management Dashboard"
            >
              <LayoutDashboard className="w-3.5 h-3.5" />
              <span>Switch to Club Manager</span>
            </Link>

            <button
              onClick={() => signOut({ callbackUrl: "/auth/signin" })}
              className="p-2 rounded-lg bg-slate-800 hover:bg-rose-950/60 text-slate-400 hover:text-rose-300 border border-slate-700 transition-colors"
              title="Sign Out"
            >
              <LogOut className="w-4 h-4" />
            </button>
          </div>
        </div>
      </header>

      {/* Main Container */}
      <main className="max-w-7xl mx-auto px-4 sm:px-6 pt-6 space-y-6">
        {/* Navigation Tabs (Mobile scrollable & Desktop) */}
        <div className="flex items-center gap-1 border-b border-slate-800 overflow-x-auto pb-1 scrollbar-none">
          <button
            onClick={() => setActiveTab("metrics")}
            className={`flex items-center gap-2 px-3.5 py-2 rounded-lg text-xs font-semibold whitespace-nowrap transition-all ${
              activeTab === "metrics"
                ? "bg-blue-600 text-white shadow-md shadow-blue-950/40"
                : "text-slate-400 hover:text-slate-200 hover:bg-slate-900"
            }`}
          >
            <Activity className="w-4 h-4" />
            <span>Usage Metrics & Analytics</span>
          </button>

          <button
            onClick={() => setActiveTab("users")}
            className={`flex items-center gap-2 px-3.5 py-2 rounded-lg text-xs font-semibold whitespace-nowrap transition-all relative ${
              activeTab === "users"
                ? "bg-blue-600 text-white shadow-md shadow-blue-950/40"
                : "text-slate-400 hover:text-slate-200 hover:bg-slate-900"
            }`}
          >
            <Users className="w-4 h-4" />
            <span>Customer Accounts ({kpi?.totalUsers || 0})</span>
          </button>

          <button
            onClick={() => setActiveTab("devices")}
            className={`flex items-center gap-2 px-3.5 py-2 rounded-lg text-xs font-semibold whitespace-nowrap transition-all relative ${
              activeTab === "devices"
                ? "bg-blue-600 text-white shadow-md shadow-blue-950/40"
                : "text-slate-400 hover:text-slate-200 hover:bg-slate-900"
            }`}
          >
            <Server className="w-4 h-4" />
            <span>Hardware Fleet ({kpi?.totalDevices || 0})</span>
          </button>

          <button
            onClick={() => setActiveTab("firmware")}
            className={`flex items-center gap-2 px-3.5 py-2 rounded-lg text-xs font-semibold whitespace-nowrap transition-all ${
              activeTab === "firmware"
                ? "bg-blue-600 text-white shadow-md shadow-blue-950/40"
                : "text-slate-400 hover:text-slate-200 hover:bg-slate-900"
            }`}
          >
            <Cpu className="w-4 h-4" />
            <span>Firmware OTA Manager</span>
          </button>

          <button
            onClick={() => setActiveTab("diagnostics")}
            className={`flex items-center gap-2 px-3.5 py-2 rounded-lg text-xs font-semibold whitespace-nowrap transition-all relative ${
              activeTab === "diagnostics"
                ? "bg-blue-600 text-white shadow-md shadow-blue-950/40"
                : "text-slate-400 hover:text-slate-200 hover:bg-slate-900"
            }`}
          >
            <AlertTriangle className="w-4 h-4" />
            <span>Error Reports & Bugs</span>
            {(kpi?.unresolvedErrors || 0) > 0 && (
              <span className="px-1.5 py-0.2 rounded-full bg-rose-500 text-[10px] font-mono text-white">
                {kpi?.unresolvedErrors}
              </span>
            )}
          </button>
        </div>

        {/* ============================================================ */}
        {/* TAB 1: USAGE METRICS & PRODUCT ANALYTICS */}
        {/* ============================================================ */}
        {activeTab === "metrics" && (
          <div className="space-y-6">
            {/* KPI Overview Cards */}
            <div className="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-6 gap-3">
              <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-800">
                <div className="flex items-center justify-between text-slate-400 mb-1">
                  <span className="text-[11px] font-medium">Customers</span>
                  <Users className="w-4 h-4 text-blue-400" />
                </div>
                <div className="text-xl font-bold text-white font-mono">{kpi?.totalUsers || 0}</div>
                <div className="text-[10px] text-slate-400 mt-1">
                  {kpi?.ownersCount} owners • {kpi?.managersCount} managers
                </div>
              </div>

              <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-800">
                <div className="flex items-center justify-between text-slate-400 mb-1">
                  <span className="text-[11px] font-medium">Online Fleet</span>
                  <Server className="w-4 h-4 text-emerald-400" />
                </div>
                <div className="text-xl font-bold text-emerald-400 font-mono">
                  {kpi?.onlineDevices || 0} / {kpi?.totalDevices || 0}
                </div>
                <div className="text-[10px] text-slate-400 mt-1">
                  {kpi?.offlineDevices} offline • {kpi?.unclaimedDevices} unassigned
                </div>
              </div>

              <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-800">
                <div className="flex items-center justify-between text-slate-400 mb-1">
                  <span className="text-[11px] font-medium">7-Day Games</span>
                  <Activity className="w-4 h-4 text-purple-400" />
                </div>
                <div className="text-xl font-bold text-white font-mono">{kpi?.totalGames7d || 0}</div>
                <div className="text-[10px] text-slate-400 mt-1">
                  Total lifetime: {kpi?.totalLifetimeRacks?.toLocaleString()}
                </div>
              </div>

              <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-800">
                <div className="flex items-center justify-between text-slate-400 mb-1">
                  <span className="text-[11px] font-medium">7-Day Revenue</span>
                  <Coins className="w-4 h-4 text-amber-400" />
                </div>
                <div className="text-xl font-bold text-amber-400 font-mono">
                  ₦{(kpi?.totalRevenue7d || 0).toLocaleString()}
                </div>
                <div className="text-[10px] text-slate-400 mt-1">Across all paired venues</div>
              </div>

              <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-800">
                <div className="flex items-center justify-between text-slate-400 mb-1">
                  <span className="text-[11px] font-medium">Wi-Fi Quality</span>
                  <Wifi className="w-4 h-4 text-sky-400" />
                </div>
                <div className="text-xl font-bold text-sky-400 font-mono">
                  {trends?.rssi?.good || 0} strong
                </div>
                <div className="text-[10px] text-slate-400 mt-1">
                  {trends?.rssi?.weak || 0} weak signals (&lt; -78 dBm)
                </div>
              </div>

              <div className="p-4 rounded-xl bg-slate-900/80 border border-slate-800">
                <div className="flex items-center justify-between text-slate-400 mb-1">
                  <span className="text-[11px] font-medium">Open Errors</span>
                  <AlertTriangle className="w-4 h-4 text-rose-400" />
                </div>
                <div
                  className={`text-xl font-bold font-mono ${
                    (kpi?.unresolvedErrors || 0) > 0 ? "text-rose-400" : "text-emerald-400"
                  }`}
                >
                  {kpi?.unresolvedErrors || 0}
                </div>
                <div className="text-[10px] text-slate-400 mt-1">Active diagnostics to review</div>
              </div>
            </div>

            {/* 7-Day Velocity & Peak Playing Hours */}
            <div className="grid grid-cols-1 lg:grid-cols-2 gap-5">
              {/* Daily Volume Bar Chart */}
              <div className="p-5 rounded-2xl bg-slate-900/80 border border-slate-800">
                <div className="flex items-center justify-between mb-4">
                  <div>
                    <h3 className="text-sm font-bold text-white flex items-center gap-2">
                      <Activity className="w-4 h-4 text-blue-400" />
                      <span>7-Day Platform Utilization Velocity</span>
                    </h3>
                    <p className="text-xs text-slate-400">Total racks played daily across all customer venues</p>
                  </div>
                </div>

                <div className="h-44 flex items-end gap-2 pt-4">
                  {trends?.daily && trends.daily.length > 0 ? (
                    trends.daily.map((d) => {
                      const maxGames = Math.max(...trends.daily.map((x) => x.games), 1);
                      const heightPercent = Math.max(Math.round((d.games / maxGames) * 100), 8);
                      return (
                        <div key={d.date} className="flex-1 flex flex-col items-center gap-1.5 h-full justify-end">
                          <span className="text-[10px] font-mono text-slate-300 font-semibold">{d.games}</span>
                          <div
                            style={{ height: `${heightPercent}%` }}
                            className="w-full rounded-t-md bg-gradient-to-t from-blue-700 to-blue-500 hover:from-blue-600 hover:to-blue-400 transition-all cursor-pointer"
                            title={`${d.date}: ${d.games} racks (₦${d.revenue.toLocaleString()})`}
                          />
                          <span className="text-[10px] font-mono text-slate-500 truncate w-full text-center">
                            {d.date.slice(5)}
                          </span>
                        </div>
                      );
                    })
                  ) : (
                    <div className="w-full h-full flex items-center justify-center text-xs text-slate-500">
                      No rack telemetry recorded yet
                    </div>
                  )}
                </div>
              </div>

              {/* 24-Hour Peak Play Heatmap / Histogram */}
              <div className="p-5 rounded-2xl bg-slate-900/80 border border-slate-800">
                <div className="flex items-center justify-between mb-4">
                  <div>
                    <h3 className="text-sm font-bold text-white flex items-center gap-2">
                      <Clock className="w-4 h-4 text-purple-400" />
                      <span>Peak Hours of the Day (00:00 - 23:00)</span>
                    </h3>
                    <p className="text-xs text-slate-400">Discover when players play billiards to optimize club hours</p>
                  </div>
                </div>

                <div className="h-44 flex items-end gap-1 pt-4 overflow-x-auto scrollbar-none">
                  {trends?.hourly && trends.hourly.length > 0 ? (
                    trends.hourly.map((h) => {
                      const maxHourly = Math.max(...trends.hourly.map((x) => x.games), 1);
                      const heightPercent = Math.max(Math.round((h.games / maxHourly) * 100), 6);
                      return (
                        <div key={h.hour} className="flex-1 min-w-[14px] flex flex-col items-center gap-1.5 h-full justify-end">
                          {h.games > 0 && (
                            <span className="text-[9px] font-mono text-purple-300">{h.games}</span>
                          )}
                          <div
                            style={{ height: `${heightPercent}%` }}
                            className={`w-full rounded-t-sm transition-all ${
                              h.games > 0
                                ? "bg-gradient-to-t from-purple-700 to-purple-400 hover:to-purple-300"
                                : "bg-slate-800"
                            }`}
                            title={`${h.hourLabel}: ${h.games} games`}
                          />
                          <span className="text-[8px] font-mono text-slate-500">
                            {h.hour % 3 === 0 ? h.hour : ""}
                          </span>
                        </div>
                      );
                    })
                  ) : (
                    <div className="w-full h-full flex items-center justify-center text-xs text-slate-500">
                      No hourly telemetry recorded yet
                    </div>
                  )}
                </div>
              </div>
            </div>

            {/* Triggers, RSSI & Firmware Breakdown */}
            <div className="grid grid-cols-1 md:grid-cols-3 gap-5">
              {/* Trigger Breakdown */}
              <div className="p-5 rounded-2xl bg-slate-900/80 border border-slate-800">
                <h4 className="text-xs font-bold text-white uppercase tracking-wider mb-3 flex items-center gap-1.5">
                  <Layers className="w-4 h-4 text-blue-400" />
                  <span>Game Trigger Method</span>
                </h4>
                <div className="space-y-3">
                  {Object.entries(trends?.triggers || {}).map(([key, count]) => (
                    <div key={key} className="space-y-1">
                      <div className="flex justify-between text-xs">
                        <span className="capitalize text-slate-300">
                          {key === "rfid" ? "RFID Staff/Member Card" : key === "button" ? "Hardware Coin/Button" : key === "cloud" ? "Remote Web Unlock" : "System Event"}
                        </span>
                        <span className="font-mono text-white font-semibold">{count}</span>
                      </div>
                      <div className="w-full h-1.5 rounded-full bg-slate-800 overflow-hidden">
                        <div
                          style={{
                            width: `${Math.min(
                              100,
                              Math.round(
                                (count /
                                  Math.max(
                                    Object.values(trends?.triggers || {}).reduce((a, b) => a + b, 0),
                                    1
                                  )) *
                                  100
                              )
                            )}%`,
                          }}
                          className="h-full bg-blue-500 rounded-full"
                        />
                      </div>
                    </div>
                  ))}
                </div>
              </div>

              {/* Wi-Fi RSSI Signal Health */}
              <div className="p-5 rounded-2xl bg-slate-900/80 border border-slate-800">
                <h4 className="text-xs font-bold text-white uppercase tracking-wider mb-3 flex items-center gap-1.5">
                  <Wifi className="w-4 h-4 text-emerald-400" />
                  <span>Wi-Fi RSSI Signal Distribution</span>
                </h4>
                <div className="space-y-3 text-xs">
                  <div className="flex items-center justify-between p-2.5 rounded-lg bg-emerald-950/30 border border-emerald-500/20">
                    <div className="flex items-center gap-2 text-emerald-300">
                      <div className="w-2 h-2 rounded-full bg-emerald-400" />
                      <span>Strong Signal (&gt; -65 dBm)</span>
                    </div>
                    <span className="font-mono font-bold text-white">{trends?.rssi?.good || 0}</span>
                  </div>

                  <div className="flex items-center justify-between p-2.5 rounded-lg bg-amber-950/30 border border-amber-500/20">
                    <div className="flex items-center gap-2 text-amber-300">
                      <div className="w-2 h-2 rounded-full bg-amber-400" />
                      <span>Fair Signal (-65 to -78 dBm)</span>
                    </div>
                    <span className="font-mono font-bold text-white">{trends?.rssi?.fair || 0}</span>
                  </div>

                  <div className="flex items-center justify-between p-2.5 rounded-lg bg-rose-950/30 border border-rose-500/20">
                    <div className="flex items-center gap-2 text-rose-300">
                      <div className="w-2 h-2 rounded-full bg-rose-400" />
                      <span>Weak Signal (&lt; -78 dBm)</span>
                    </div>
                    <span className="font-mono font-bold text-white">{trends?.rssi?.weak || 0}</span>
                  </div>
                </div>
              </div>

              {/* Active Firmware Versions */}
              <div className="p-5 rounded-2xl bg-slate-900/80 border border-slate-800">
                <h4 className="text-xs font-bold text-white uppercase tracking-wider mb-3 flex items-center gap-1.5">
                  <Cpu className="w-4 h-4 text-purple-400" />
                  <span>Fleet Firmware Adoption</span>
                </h4>
                <div className="space-y-2.5 text-xs">
                  {Object.entries(trends?.firmwares || {}).map(([ver, count]) => (
                    <div key={ver} className="flex items-center justify-between p-2.5 rounded-lg bg-slate-800/60 border border-slate-700/60">
                      <div className="flex items-center gap-2">
                        <FileCode className="w-3.5 h-3.5 text-purple-400" />
                        <span className="font-mono text-slate-200">{ver}</span>
                      </div>
                      <span className="font-mono font-bold text-emerald-400">{count} tables</span>
                    </div>
                  ))}
                  {Object.keys(trends?.firmwares || {}).length === 0 && (
                    <p className="text-xs text-slate-500">No firmware telemetry received yet</p>
                  )}
                </div>
              </div>
            </div>
          </div>
        )}

        {/* ============================================================ */}
        {/* TAB 2: USER MANAGEMENT */}
        {/* ============================================================ */}
        {activeTab === "users" && (
          <div className="space-y-4">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
              <div>
                <h2 className="text-base font-bold text-white flex items-center gap-2">
                  <Users className="w-4 h-4 text-blue-400" />
                  <span>Customer & Team Accounts</span>
                </h2>
                <p className="text-xs text-slate-400">
                  Manage roles, promote administrators, inspect venue ownership, and adjust subscription plans
                </p>
              </div>

              <div className="flex items-center gap-2 flex-wrap">
                <div className="relative">
                  <Search className="w-3.5 h-3.5 absolute left-3 top-2.5 text-slate-400" />
                  <input
                    type="text"
                    placeholder="Search name, email..."
                    value={userQuery}
                    onChange={(e) => setUserQuery(e.target.value)}
                    className="pl-8 pr-3 py-1.5 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white w-48 sm:w-60 focus:outline-none focus:border-blue-500"
                  />
                </div>

                <select
                  value={userRoleFilter}
                  onChange={(e) => setUserRoleFilter(e.target.value)}
                  className="px-2.5 py-1.5 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white focus:outline-none"
                >
                  <option value="all">All Roles</option>
                  <option value="admin">Admins</option>
                  <option value="owner">Owners</option>
                  <option value="manager">Managers</option>
                  <option value="attendant">Attendants</option>
                </select>
              </div>
            </div>

            {/* Desktop Table View */}
            <div className="hidden sm:block rounded-2xl bg-slate-900/80 border border-slate-800 overflow-hidden">
              <table className="w-full text-left text-xs text-slate-300">
                <thead className="bg-slate-800/80 text-[11px] font-semibold text-slate-400 uppercase tracking-wider border-b border-slate-700">
                  <tr>
                    <th className="py-3 px-4">User</th>
                    <th className="py-3 px-4">Role</th>
                    <th className="py-3 px-4">Company / Venues</th>
                    <th className="py-3 px-4">Tables</th>
                    <th className="py-3 px-4">Plan</th>
                    <th className="py-3 px-4">Registered</th>
                    <th className="py-3 px-4 text-right">Actions</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-800/70">
                  {users.map((u) => (
                    <tr key={u._id} className="hover:bg-slate-800/40 transition-colors">
                      <td className="py-3 px-4">
                        <div className="font-semibold text-white">{u.name}</div>
                        <div className="font-mono text-[11px] text-slate-400">{u.email}</div>
                      </td>
                      <td className="py-3 px-4">
                        <span
                          className={`px-2 py-0.5 rounded text-[10px] font-semibold uppercase tracking-wider ${
                            u.role === "admin" || u.role === "superadmin"
                              ? "bg-blue-500/20 text-blue-300 border border-blue-500/40"
                              : u.role === "owner"
                              ? "bg-emerald-500/20 text-emerald-300 border border-emerald-500/40"
                              : "bg-slate-800 text-slate-300 border border-slate-700"
                          }`}
                        >
                          {u.role}
                        </span>
                      </td>
                      <td className="py-3 px-4">
                        <div className="text-white">{u.companyName || "Personal"}</div>
                        <div className="text-[10px] text-slate-500">
                          {u.venues?.join(", ") || "Main Floor"}
                        </div>
                      </td>
                      <td className="py-3 px-4">
                        <span className="font-mono font-bold text-emerald-400">{u.tableCount} tables</span>
                      </td>
                      <td className="py-3 px-4">
                        <span className="capitalize text-slate-200">{u.subscriptionPlan || "Growth"}</span>
                        <span className="text-[10px] text-slate-500 block capitalize">
                          {u.subscriptionStatus || "Active"}
                        </span>
                      </td>
                      <td className="py-3 px-4 text-slate-400 font-mono text-[11px]">
                        {new Date(u.createdAt).toISOString().split("T")[0]}
                      </td>
                      <td className="py-3 px-4 text-right space-x-1.5">
                        <button
                          onClick={() => {
                            setEditUserModal(u);
                            setEditRole(u.role);
                            setEditPlan(u.subscriptionPlan || "growth");
                            setEditStatus(u.subscriptionStatus || "active");
                          }}
                          className="px-2.5 py-1 rounded bg-slate-800 hover:bg-slate-700 text-blue-400 border border-slate-700 text-[11px] font-medium"
                        >
                          Edit
                        </button>
                        <button
                          onClick={() => handleDeleteUser(u)}
                          className="p-1 rounded bg-slate-800 hover:bg-rose-950/70 text-slate-400 hover:text-rose-300 border border-slate-700 text-[11px]"
                          title="Delete User"
                        >
                          <Trash2 className="w-3.5 h-3.5" />
                        </button>
                      </td>
                    </tr>
                  ))}
                  {users.length === 0 && (
                    <tr>
                      <td colSpan={7} className="py-8 text-center text-slate-500">
                        No customer accounts found matching criteria.
                      </td>
                    </tr>
                  )}
                </tbody>
              </table>
            </div>

            {/* Mobile Stacked Card View */}
            <div className="sm:hidden space-y-3">
              {users.map((u) => (
                <div key={u._id} className="p-4 rounded-xl bg-slate-900 border border-slate-800 space-y-2">
                  <div className="flex items-start justify-between">
                    <div>
                      <h4 className="font-bold text-white text-xs">{u.name}</h4>
                      <p className="text-[11px] font-mono text-slate-400">{u.email}</p>
                    </div>
                    <span
                      className={`px-2 py-0.5 rounded text-[9px] font-semibold uppercase tracking-wider ${
                        u.role === "admin" || u.role === "superadmin"
                          ? "bg-blue-500/20 text-blue-300 border border-blue-500/40"
                          : "bg-slate-800 text-slate-300 border border-slate-700"
                      }`}
                    >
                      {u.role}
                    </span>
                  </div>

                  <div className="text-[11px] text-slate-400 flex justify-between py-1 border-y border-slate-800/80">
                    <span>Company: {u.companyName || "Arena"}</span>
                    <span className="font-mono text-emerald-400 font-semibold">{u.tableCount} tables</span>
                  </div>

                  <div className="flex items-center justify-end gap-2 pt-1">
                    <button
                      onClick={() => {
                        setEditUserModal(u);
                        setEditRole(u.role);
                        setEditPlan(u.subscriptionPlan || "growth");
                        setEditStatus(u.subscriptionStatus || "active");
                      }}
                      className="px-3 py-1 rounded bg-slate-800 hover:bg-slate-700 text-blue-400 border border-slate-700 text-xs"
                    >
                      Edit Role & Plan
                    </button>
                    <button
                      onClick={() => handleDeleteUser(u)}
                      className="p-1.5 rounded bg-slate-800 hover:bg-rose-900/60 text-slate-400 hover:text-rose-300 border border-slate-700"
                    >
                      <Trash2 className="w-3.5 h-3.5" />
                    </button>
                  </div>
                </div>
              ))}
            </div>
          </div>
        )}

        {/* ============================================================ */}
        {/* TAB 3: HARDWARE FLEET & FACTORY PROVISIONING */}
        {/* ============================================================ */}
        {activeTab === "devices" && (
          <div className="space-y-4">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
              <div>
                <h2 className="text-base font-bold text-white flex items-center gap-2">
                  <Server className="w-4 h-4 text-emerald-400" />
                  <span>Hardware Fleet Registry</span>
                </h2>
                <p className="text-xs text-slate-400">
                  Global devices, hardware claim PINs, live Wi-Fi ping, maintenance overrides, and provisioning
                </p>
              </div>

              <div className="flex items-center gap-2 flex-wrap">
                <button
                  onClick={() => {
                    setProvisionMsg(null);
                    setProvisionName(`Hardware Table ${devices.length + 1}`);
                    setProvisionModal(true);
                  }}
                  className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold shadow-md border border-emerald-500/40 transition-all"
                >
                  <Plus className="w-4 h-4" />
                  <span>Provision Factory Node</span>
                </button>

                <div className="relative">
                  <Search className="w-3.5 h-3.5 absolute left-3 top-2.5 text-slate-400" />
                  <input
                    type="text"
                    placeholder="Search device, serial, owner..."
                    value={deviceQuery}
                    onChange={(e) => setDeviceQuery(e.target.value)}
                    className="pl-8 pr-3 py-1.5 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white w-44 sm:w-56 focus:outline-none"
                  />
                </div>

                <select
                  value={deviceStatusFilter}
                  onChange={(e) => setDeviceStatusFilter(e.target.value)}
                  className="px-2.5 py-1.5 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                >
                  <option value="all">All Devices</option>
                  <option value="claimed">Claimed</option>
                  <option value="unclaimed">Unclaimed / Factory</option>
                  <option value="maintenance">Maintenance Locked</option>
                </select>
              </div>
            </div>

            {/* Desktop Table View */}
            <div className="hidden sm:block rounded-2xl bg-slate-900/80 border border-slate-800 overflow-hidden">
              <table className="w-full text-left text-xs text-slate-300">
                <thead className="bg-slate-800/80 text-[11px] font-semibold text-slate-400 uppercase tracking-wider border-b border-slate-700">
                  <tr>
                    <th className="py-3 px-4">Node & Serial</th>
                    <th className="py-3 px-4">Claim PIN</th>
                    <th className="py-3 px-4">Owner / Venue</th>
                    <th className="py-3 px-4">Status & Ping</th>
                    <th className="py-3 px-4">Wi-Fi & IP</th>
                    <th className="py-3 px-4">Firmware</th>
                    <th className="py-3 px-4">Games</th>
                    <th className="py-3 px-4 text-right">Overrides</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-800/70">
                  {devices.map((d) => (
                    <tr key={d._id} className="hover:bg-slate-800/40 transition-colors">
                      <td className="py-3 px-4">
                        <div className="font-semibold text-white flex items-center gap-1.5">
                          <span
                            className={`w-2 h-2 rounded-full ${
                              d.isOnline ? "bg-emerald-400" : "bg-slate-600"
                            }`}
                          />
                          <span>{d.name}</span>
                        </div>
                        <div className="font-mono text-[11px] text-emerald-400 font-semibold">{d.serialNumber}</div>
                        <div className="font-mono text-[10px] text-slate-500">{d.deviceId}</div>
                      </td>
                      <td className="py-3 px-4">
                        <span className="font-mono font-bold text-amber-300 px-2 py-0.5 rounded bg-amber-500/10 border border-amber-500/30">
                          {d.claimCode}
                        </span>
                      </td>
                      <td className="py-3 px-4">
                        {d.isClaimed ? (
                          <>
                            <div className="text-white truncate max-w-[140px]">{d.ownerEmail}</div>
                            <div className="text-[10px] text-slate-400">{d.venueName} • {d.location}</div>
                          </>
                        ) : (
                          <span className="text-[10px] px-2 py-0.5 rounded bg-slate-800 text-slate-400 border border-slate-700">
                            Unassigned
                          </span>
                        )}
                      </td>
                      <td className="py-3 px-4">
                        <span
                          className={`px-2 py-0.5 rounded text-[10px] font-semibold ${
                            d.isOnline
                              ? "bg-emerald-500/20 text-emerald-300 border border-emerald-500/40"
                              : "bg-slate-800 text-slate-400 border border-slate-700"
                          }`}
                        >
                          {d.isOnline ? "Online" : "Offline"}
                        </span>
                        <div className="text-[10px] text-slate-500 font-mono mt-0.5">
                          {d.secondsSinceHeartbeat !== null ? `${d.secondsSinceHeartbeat}s ago` : "Never"}
                        </div>
                      </td>
                      <td className="py-3 px-4 font-mono text-[11px]">
                        <div className="text-slate-300">{d.rssi} dBm</div>
                        <div className="text-slate-500 text-[10px]">{d.localIp}</div>
                      </td>
                      <td className="py-3 px-4 font-mono text-[11px]">
                        <span className="text-purple-300">{d.firmwareVersion || "v2.0.0-esp32"}</span>
                        {d.otaCmd?.pending && (
                          <span className="block text-[10px] text-blue-400 font-medium">
                            OTA: {d.otaCmd.targetVersion}
                          </span>
                        )}
                      </td>
                      <td className="py-3 px-4 font-mono font-bold text-white">
                        {d.totalRacksLifetime || 0}
                      </td>
                      <td className="py-3 px-4 text-right space-x-1">
                        <button
                          onClick={() =>
                            handleDeviceAction(d.deviceId, "toggle_maintenance", {
                              maintenanceLocked: !d.maintenanceLocked,
                            })
                          }
                          className={`p-1.5 rounded border text-[11px] ${
                            d.maintenanceLocked
                              ? "bg-amber-950/70 border-amber-600 text-amber-300"
                              : "bg-slate-800 hover:bg-slate-700 text-slate-300 border-slate-700"
                          }`}
                          title={d.maintenanceLocked ? "Disable Maintenance Lock" : "Activate Maintenance Lock"}
                        >
                          <Lock className="w-3.5 h-3.5" />
                        </button>
                        <button
                          onClick={() => handleDeviceAction(d.deviceId, "reset_pin")}
                          className="p-1.5 rounded bg-slate-800 hover:bg-slate-700 text-amber-300 border border-slate-700"
                          title="Generate New Claim PIN"
                        >
                          <KeyRound className="w-3.5 h-3.5" />
                        </button>
                        {d.isClaimed && (
                          <button
                            onClick={() => handleDeviceAction(d.deviceId, "unpair")}
                            className="p-1.5 rounded bg-slate-800 hover:bg-slate-700 text-slate-400 hover:text-white border border-slate-700"
                            title="Unlink from Owner"
                          >
                            <Unlock className="w-3.5 h-3.5" />
                          </button>
                        )}
                      </td>
                    </tr>
                  ))}
                  {devices.length === 0 && (
                    <tr>
                      <td colSpan={8} className="py-8 text-center text-slate-500">
                        No devices found matching query.
                      </td>
                    </tr>
                  )}
                </tbody>
              </table>
            </div>

            {/* Mobile Stacked Device Cards */}
            <div className="sm:hidden space-y-3">
              {devices.map((d) => (
                <div key={d._id} className="p-4 rounded-xl bg-slate-900 border border-slate-800 space-y-2">
                  <div className="flex items-start justify-between">
                    <div>
                      <div className="font-bold text-white text-xs flex items-center gap-1.5">
                        <span className={`w-2 h-2 rounded-full ${d.isOnline ? "bg-emerald-400" : "bg-slate-600"}`} />
                        <span>{d.name}</span>
                      </div>
                      <div className="font-mono text-[11px] text-emerald-400">{d.serialNumber}</div>
                    </div>
                    <span className="font-mono font-bold text-amber-300 text-xs px-2 py-0.5 rounded bg-amber-500/10 border border-amber-500/30">
                      PIN: {d.claimCode}
                    </span>
                  </div>

                  <div className="text-[11px] text-slate-400 flex justify-between py-1 border-y border-slate-800/80">
                    <span>{d.ownerEmail || "Unassigned"}</span>
                    <span className="font-mono text-purple-300">{d.firmwareVersion || "v2.0.0"}</span>
                  </div>

                  <div className="flex items-center justify-between text-[11px] text-slate-400">
                    <span>Games: <strong className="text-white">{d.totalRacksLifetime || 0}</strong></span>
                    <span>RSSI: <strong className="text-slate-200">{d.rssi} dBm</strong></span>
                  </div>

                  <div className="flex items-center justify-end gap-2 pt-1">
                    <button
                      onClick={() =>
                        handleDeviceAction(d.deviceId, "toggle_maintenance", {
                          maintenanceLocked: !d.maintenanceLocked,
                        })
                      }
                      className="px-2.5 py-1 rounded bg-slate-800 text-xs border border-slate-700"
                    >
                      {d.maintenanceLocked ? "Unlock Table" : "Lock Table"}
                    </button>
                    <button
                      onClick={() => handleDeviceAction(d.deviceId, "reset_pin")}
                      className="px-2.5 py-1 rounded bg-slate-800 text-amber-300 text-xs border border-slate-700"
                    >
                      Regen PIN
                    </button>
                  </div>
                </div>
              ))}
            </div>
          </div>
        )}

        {/* ============================================================ */}
        {/* TAB 4: FIRMWARE OTA MANAGER */}
        {/* ============================================================ */}
        {activeTab === "firmware" && (
          <div className="space-y-5">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
              <div>
                <h2 className="text-base font-bold text-white flex items-center gap-2">
                  <Cpu className="w-4 h-4 text-purple-400" />
                  <span>Central Firmware Repository & Fleet Rollout</span>
                </h2>
                <p className="text-xs text-slate-400">
                  Manage compiled ESP32 binary releases, designate factory defaults, and trigger fleet-wide upgrades or rollbacks
                </p>
              </div>

              <button
                onClick={() => setUploadModal(true)}
                className="flex items-center gap-1.5 px-3.5 py-2 rounded-lg bg-blue-600 hover:bg-blue-500 text-white text-xs font-semibold shadow-md border border-blue-500/40 transition-all self-start sm:self-auto"
              >
                <UploadCloud className="w-4 h-4" />
                <span>Upload New Firmware (.bin)</span>
              </button>
            </div>

            {/* Firmware Cards Grid */}
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
              {firmwares.map((fw) => (
                <div
                  key={fw._id}
                  className={`p-5 rounded-2xl bg-slate-900 border flex flex-col justify-between transition-all ${
                    fw.isDefault ? "border-purple-500/50 shadow-lg shadow-purple-950/20" : "border-slate-800"
                  }`}
                >
                  <div className="space-y-3">
                    <div className="flex items-start justify-between">
                      <div>
                        <div className="flex items-center gap-2">
                          <span className="font-mono font-bold text-base text-white">{fw.version}</span>
                          {fw.isDefault && (
                            <span className="text-[10px] px-2 py-0.5 rounded font-bold uppercase tracking-wider bg-purple-500/20 text-purple-300 border border-purple-500/40">
                              Default
                            </span>
                          )}
                        </div>
                        <p className="text-xs text-slate-400 mt-0.5">{fw.targetHardware}</p>
                      </div>

                      <div className="text-right">
                        <span className="text-xs font-mono font-bold text-emerald-400">
                          {fw.activeDeviceCount} live tables
                        </span>
                        <div className="text-[10px] text-slate-500 font-mono">
                          {(fw.fileSizeBytes / 1024).toFixed(1)} KB
                        </div>
                      </div>
                    </div>

                    <p className="text-xs text-slate-300 leading-relaxed bg-slate-800/50 p-2.5 rounded-lg border border-slate-800">
                      {fw.releaseNotes || "Compiled ESP32 DevKit V1 binary image."}
                    </p>

                    <div className="text-[11px] text-slate-500 font-mono">
                      File: {fw.fileName}
                    </div>
                  </div>

                  <div className="mt-4 pt-3 border-t border-slate-800 flex items-center justify-between gap-2">
                    <button
                      onClick={() => {
                        setRolloutModal(fw);
                        setRolloutTargetDevice("*");
                      }}
                      className="flex-1 flex items-center justify-center gap-1.5 px-3 py-1.5 rounded-lg bg-blue-600 hover:bg-blue-500 text-white text-xs font-semibold shadow-sm transition-all"
                    >
                      <UploadCloud className="w-3.5 h-3.5" />
                      <span>Deploy OTA</span>
                    </button>

                    {!fw.isDefault && (
                      <button
                        onClick={() => handleSetDefaultFirmware(fw)}
                        className="px-2.5 py-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-300 text-xs border border-slate-700"
                        title="Set as Factory Default"
                      >
                        Set Default
                      </button>
                    )}

                    <button
                      onClick={() => handleDeleteFirmware(fw)}
                      className="p-1.5 rounded-lg bg-slate-800 hover:bg-rose-950/70 text-slate-400 hover:text-rose-300 border border-slate-700"
                      title="Delete Release"
                    >
                      <Trash2 className="w-3.5 h-3.5" />
                    </button>
                  </div>
                </div>
              ))}
            </div>

            {firmwares.length === 0 && (
              <div className="p-12 rounded-2xl bg-slate-900 border border-dashed border-slate-800 text-center">
                <FileCode className="w-8 h-8 text-slate-500 mx-auto mb-2" />
                <h4 className="text-sm font-semibold text-white">No Firmware Releases in Repository</h4>
                <p className="text-xs text-slate-400 mt-1">
                  Upload an exported binary (.bin) from Arduino IDE to make it available for over-the-air updates.
                </p>
                <button
                  onClick={() => setUploadModal(true)}
                  className="mt-4 px-3.5 py-2 rounded-lg bg-blue-600 hover:bg-blue-500 text-white text-xs font-semibold"
                >
                  Upload First Firmware
                </button>
              </div>
            )}
          </div>
        )}

        {/* ============================================================ */}
        {/* TAB 5: ERROR REPORTS & BUG DIAGNOSTICS */}
        {/* ============================================================ */}
        {activeTab === "diagnostics" && (
          <div className="space-y-4">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
              <div>
                <h2 className="text-base font-bold text-white flex items-center gap-2">
                  <AlertTriangle className="w-4 h-4 text-rose-400" />
                  <span>Diagnostics, Bug Reports & Telemetry Alerts</span>
                </h2>
                <p className="text-xs text-slate-400">
                  Real-time error tracking, OTA flash failures, unauthorized RFID tampering, and operational bugs
                </p>
              </div>

              <div className="flex items-center gap-2 flex-wrap">
                <button
                  onClick={handleResolveAllErrors}
                  className="px-3 py-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-emerald-400 text-xs font-semibold border border-slate-700 transition-colors"
                >
                  Resolve All Open
                </button>

                <select
                  value={errorSeverityFilter}
                  onChange={(e) => setErrorSeverityFilter(e.target.value)}
                  className="px-2.5 py-1.5 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                >
                  <option value="all">All Severities</option>
                  <option value="critical">Critical</option>
                  <option value="warning">Warning</option>
                  <option value="info">Info</option>
                </select>

                <select
                  value={errorResolvedFilter}
                  onChange={(e) => setErrorResolvedFilter(e.target.value)}
                  className="px-2.5 py-1.5 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                >
                  <option value="all">All Statuses</option>
                  <option value="false">Open Only</option>
                  <option value="true">Resolved Only</option>
                </select>
              </div>
            </div>

            {/* Error List */}
            <div className="space-y-3">
              {errors.map((err) => (
                <div
                  key={err._id}
                  className={`p-4 rounded-xl bg-slate-900 border transition-all ${
                    err.resolved
                      ? "border-slate-800/60 opacity-60"
                      : err.severity === "critical"
                      ? "border-rose-500/50 shadow-md shadow-rose-950/20"
                      : "border-amber-500/40"
                  }`}
                >
                  <div className="flex items-start justify-between gap-3">
                    <div className="space-y-1">
                      <div className="flex items-center gap-2 flex-wrap">
                        <span
                          className={`px-2 py-0.5 rounded text-[10px] font-bold uppercase tracking-wider ${
                            err.severity === "critical"
                              ? "bg-rose-500/20 text-rose-300 border border-rose-500/40"
                              : "bg-amber-500/20 text-amber-300 border border-amber-500/40"
                          }`}
                        >
                          {err.severity}
                        </span>
                        <span className="font-mono text-xs font-semibold text-slate-300">{err.type}</span>
                        {err.deviceId && (
                          <span className="font-mono text-[11px] px-1.5 py-0.5 rounded bg-slate-800 text-slate-300 border border-slate-700">
                            Device: {err.deviceId}
                          </span>
                        )}
                        <span className="text-[10px] text-slate-500 font-mono">
                          {new Date(err.createdAt).toLocaleString()}
                        </span>
                      </div>

                      <p className="text-xs font-medium text-white pt-1">{err.message}</p>
                      {err.details && (
                        <pre className="text-[11px] font-mono text-slate-400 bg-slate-950/80 p-2 rounded-lg border border-slate-800 overflow-x-auto max-h-32">
                          {err.details}
                        </pre>
                      )}
                    </div>

                    <button
                      onClick={() => handleToggleResolveError(err._id)}
                      className={`px-3 py-1.5 rounded-lg text-xs font-semibold shrink-0 border transition-all ${
                        err.resolved
                          ? "bg-slate-800 text-slate-400 border-slate-700"
                          : "bg-emerald-600 hover:bg-emerald-500 text-white border-emerald-500/40"
                      }`}
                    >
                      {err.resolved ? "Reopen" : "Mark Resolved"}
                    </button>
                  </div>
                </div>
              ))}

              {errors.length === 0 && (
                <div className="p-12 rounded-2xl bg-slate-900 border border-dashed border-slate-800 text-center">
                  <CheckCircle2 className="w-8 h-8 text-emerald-400 mx-auto mb-2" />
                  <h4 className="text-sm font-semibold text-white">No Reported Bugs or Diagnostic Errors</h4>
                  <p className="text-xs text-slate-400 mt-1">
                    All tables and system services are operating within normal telemetry parameters.
                  </p>
                </div>
              )}
            </div>
          </div>
        )}
      </main>

      {/* ============================================================ */}
      {/* MODAL 1: EDIT USER ROLE & SUBSCRIPTION */}
      {/* ============================================================ */}
      {editUserModal && (
        <div className="fixed inset-0 z-50 bg-black/75 backdrop-blur-sm flex items-center justify-center p-4">
          <div className="w-full max-w-md rounded-2xl bg-slate-900 border border-slate-800 p-6 shadow-2xl relative">
            <button
              onClick={() => setEditUserModal(null)}
              className="absolute top-4 right-4 p-1.5 rounded-lg text-slate-400 hover:text-white hover:bg-slate-800"
            >
              <X className="w-4 h-4" />
            </button>

            <h3 className="text-base font-bold text-white mb-1">Edit Customer Privileges</h3>
            <p className="text-xs text-slate-400 mb-4">{editUserModal.email}</p>

            <form onSubmit={handleSaveUser} className="space-y-4">
              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">Account Role</label>
                <select
                  value={editRole}
                  onChange={(e) => setEditRole(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                >
                  <option value="owner">Venue Owner (Full Venue Control)</option>
                  <option value="manager">Manager (Shift Reconciliation & Tables)</option>
                  <option value="attendant">Attendant (View Only)</option>
                  <option value="admin">Administrator (Global Access)</option>
                </select>
              </div>

              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">Subscription Plan</label>
                <select
                  value={editPlan}
                  onChange={(e) => setEditPlan(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                >
                  <option value="starter">Starter (1 - 2 Tables)</option>
                  <option value="growth">Growth (3 - 8 Tables)</option>
                  <option value="enterprise">Enterprise (Unlimited Tables)</option>
                </select>
              </div>

              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">Subscription Status</label>
                <select
                  value={editStatus}
                  onChange={(e) => setEditStatus(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                >
                  <option value="active">Active (Paid)</option>
                  <option value="trial">Trial (14-Day Free Evaluation)</option>
                  <option value="past_due">Past Due / Suspended</option>
                </select>
              </div>

              <div className="pt-2 flex items-center justify-end gap-2">
                <button
                  type="button"
                  onClick={() => setEditUserModal(null)}
                  className="px-4 py-2 rounded-lg bg-slate-800 text-slate-300 text-xs"
                >
                  Cancel
                </button>
                <button
                  type="submit"
                  className="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-500 text-white text-xs font-semibold"
                >
                  Save Changes
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* ============================================================ */}
      {/* MODAL 2: FACTORY PROVISION HARDWARE NODE */}
      {/* ============================================================ */}
      {provisionModal && (
        <div className="fixed inset-0 z-50 bg-black/75 backdrop-blur-sm flex items-center justify-center p-4">
          <div className="w-full max-w-md rounded-2xl bg-slate-900 border border-slate-800 p-6 shadow-2xl relative">
            <button
              onClick={() => {
                setProvisionModal(false);
                setProvisionMsg(null);
              }}
              className="absolute top-4 right-4 p-1.5 rounded-lg text-slate-400 hover:text-white hover:bg-slate-800"
            >
              <X className="w-4 h-4" />
            </button>

            <h3 className="text-base font-bold text-white mb-1">Factory Node Provisioning</h3>
            <p className="text-xs text-slate-400 mb-4">
              Generate official Serial Number & 6-Digit Claim PIN before shipping ESP32 hardware to a customer
            </p>

            {provisionMsg ? (
              <div className="p-4 rounded-xl bg-emerald-950/60 border border-emerald-500/40 text-center space-y-3">
                <CheckCircle2 className="w-8 h-8 text-emerald-400 mx-auto" />
                <h4 className="text-sm font-bold text-white">Hardware Successfully Registered</h4>
                <div className="space-y-1 bg-slate-900 p-3 rounded-lg border border-slate-800 font-mono">
                  <div className="text-xs text-slate-400">
                    Serial: <strong className="text-emerald-400 text-sm">{provisionMsg.serial}</strong>
                  </div>
                  <div className="text-xs text-slate-400">
                    Claim PIN: <strong className="text-amber-300 text-base">{provisionMsg.pin}</strong>
                  </div>
                </div>
                <p className="text-[11px] text-slate-400">
                  Print this serial and 6-digit PIN on the device label. The customer will use them to claim the table.
                </p>
                <button
                  type="button"
                  onClick={() => {
                    setProvisionModal(false);
                    setProvisionMsg(null);
                  }}
                  className="w-full py-2 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold"
                >
                  Done
                </button>
              </div>
            ) : (
              <form onSubmit={handleProvisionDevice} className="space-y-3">
                <div>
                  <label className="block text-xs font-medium text-slate-300 mb-1">Default Name</label>
                  <input
                    type="text"
                    required
                    value={provisionName}
                    onChange={(e) => setProvisionName(e.target.value)}
                    className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                  />
                </div>

                <div>
                  <label className="block text-xs font-medium text-slate-300 mb-1">Rack Price (₦)</label>
                  <input
                    type="number"
                    required
                    value={provisionPrice}
                    onChange={(e) => setProvisionPrice(Number(e.target.value))}
                    className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                  />
                </div>

                <div className="pt-2 flex items-center justify-end gap-2">
                  <button
                    type="button"
                    onClick={() => setProvisionModal(false)}
                    className="px-4 py-2 rounded-lg bg-slate-800 text-slate-300 text-xs"
                  >
                    Cancel
                  </button>
                  <button
                    type="submit"
                    className="px-4 py-2 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold shadow-md"
                  >
                    Generate Serial & PIN
                  </button>
                </div>
              </form>
            )}
          </div>
        </div>
      )}

      {/* ============================================================ */}
      {/* MODAL 3: UPLOAD FIRMWARE */}
      {/* ============================================================ */}
      {uploadModal && (
        <div className="fixed inset-0 z-50 bg-black/75 backdrop-blur-sm flex items-center justify-center p-4">
          <div className="w-full max-w-md rounded-2xl bg-slate-900 border border-slate-800 p-6 shadow-2xl relative">
            <button
              onClick={() => setUploadModal(false)}
              className="absolute top-4 right-4 p-1.5 rounded-lg text-slate-400 hover:text-white hover:bg-slate-800"
            >
              <X className="w-4 h-4" />
            </button>

            <h3 className="text-base font-bold text-white mb-1">Upload Firmware Release</h3>
            <p className="text-xs text-slate-400 mb-4">
              Upload compiled ESP32 binary (.bin) to make it available for instant over-the-air updates
            </p>

            <form onSubmit={handleUploadFirmware} className="space-y-3">
              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">Version Tag</label>
                <input
                  type="text"
                  required
                  placeholder="v2.1.0-esp32"
                  value={fwVersion}
                  onChange={(e) => setFwVersion(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white font-mono"
                />
              </div>

              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">Binary File (.bin)</label>
                <input
                  type="file"
                  accept=".bin"
                  required
                  onChange={(e) => setFwFile(e.target.files?.[0] || null)}
                  className="w-full text-xs text-slate-400 file:mr-2 file:py-1.5 file:px-3 file:rounded-md file:border-0 file:text-xs file:font-semibold file:bg-blue-600 file:text-white hover:file:bg-blue-500 file:cursor-pointer"
                />
              </div>

              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">Release Notes</label>
                <textarea
                  rows={2}
                  placeholder="Bug fixes, dual-core sync improvements, etc."
                  value={fwNotes}
                  onChange={(e) => setFwNotes(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                />
              </div>

              <div className="flex items-center gap-2 pt-1">
                <input
                  type="checkbox"
                  id="fwDefault"
                  checked={fwIsDefault}
                  onChange={(e) => setFwIsDefault(e.target.checked)}
                  className="rounded border-slate-700 bg-slate-800 text-blue-600 focus:ring-0"
                />
                <label htmlFor="fwDefault" className="text-xs text-slate-300 cursor-pointer">
                  Designate as Default Release for new devices
                </label>
              </div>

              <div className="pt-2 flex items-center justify-end gap-2">
                <button
                  type="button"
                  onClick={() => setUploadModal(false)}
                  className="px-4 py-2 rounded-lg bg-slate-800 text-slate-300 text-xs"
                >
                  Cancel
                </button>
                <button
                  type="submit"
                  disabled={fwUploading || !fwFile}
                  className="flex items-center gap-1.5 px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-500 text-white text-xs font-semibold disabled:opacity-50"
                >
                  <UploadCloud className={`w-3.5 h-3.5 ${fwUploading ? "animate-spin" : ""}`} />
                  <span>{fwUploading ? "Uploading..." : "Upload & Register"}</span>
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* ============================================================ */}
      {/* MODAL 4: FLEET ROLLOUT */}
      {/* ============================================================ */}
      {rolloutModal && (
        <div className="fixed inset-0 z-50 bg-black/75 backdrop-blur-sm flex items-center justify-center p-4">
          <div className="w-full max-w-md rounded-2xl bg-slate-900 border border-slate-800 p-6 shadow-2xl relative">
            <button
              onClick={() => setRolloutModal(null)}
              className="absolute top-4 right-4 p-1.5 rounded-lg text-slate-400 hover:text-white hover:bg-slate-800"
            >
              <X className="w-4 h-4" />
            </button>

            <h3 className="text-base font-bold text-white mb-1">Deploy Firmware OTA</h3>
            <p className="text-xs text-slate-400 mb-4">
              Roll out <strong className="text-purple-300 font-mono">{rolloutModal.version}</strong> over the internet
            </p>

            <form onSubmit={handleFleetRollout} className="space-y-4">
              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">Target Tables</label>
                <select
                  value={rolloutTargetDevice}
                  onChange={(e) => setRolloutTargetDevice(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                >
                  <option value="*">All Online Tables in Fleet ({devices.length} nodes)</option>
                  {devices.map((d) => (
                    <option key={d.deviceId} value={d.deviceId}>
                      {d.name} ({d.serialNumber}) - {d.firmwareVersion || "v2.0.0"}
                    </option>
                  ))}
                </select>
              </div>

              <div className="p-3 rounded-lg bg-blue-950/40 border border-blue-900/50 text-xs text-blue-300 space-y-1">
                <p className="font-semibold text-blue-200">Zero-Bricking Dual-Partition Safety:</p>
                <p className="text-[11px] text-slate-300 leading-relaxed">
                  Target nodes will receive the update URL on their next heartbeat and flash the inactive partition in the background. If Wi-Fi fails mid-stream, the existing partition continues to run without interruption.
                </p>
              </div>

              <div className="pt-2 flex items-center justify-end gap-2">
                <button
                  type="button"
                  onClick={() => setRolloutModal(null)}
                  className="px-4 py-2 rounded-lg bg-slate-800 text-slate-300 text-xs"
                >
                  Cancel
                </button>
                <button
                  type="submit"
                  className="flex items-center gap-1.5 px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-500 text-white text-xs font-semibold shadow-md"
                >
                  <UploadCloud className="w-3.5 h-3.5" />
                  <span>Start Remote OTA Rollout</span>
                </button>
              </div>
            </form>
          </div>
        </div>
      )}
    </div>
  );
}

