"use client";

import React, { useState, useEffect, useRef, useTransition } from "react";
import Image from "next/image";
import Link from "next/link";
import { useRouter, usePathname } from "next/navigation";
import { useSession, signOut } from "next-auth/react";
import {
  LayoutDashboard,
  Server,
  CreditCard,
  Receipt,
  RefreshCw,
  Wifi,
  WifiOff,
  Radio,
  ShieldAlert,
  ShieldCheck,
  Shield,
  CheckCircle2,
  Clock,
  Coins,
  TrendingUp,
  AlertTriangle,
  Play,
  UserCheck,
  Ban,
  Plus,
  Trash2,
  Lock,
  Unlock,
  ChevronLeft,
  ChevronRight,
  Menu,
  X,
  ExternalLink,
  Sliders,
  DollarSign,
  Activity,
  Layers,
  CircleDot,
  Download,
  Search,
  Filter,
  Sparkles,
  KeyRound,
  Check,
  Calendar,
  User,
  Building,
  LogOut,
  MapPin,
  Settings,
  UploadCloud,
  Cpu,
  Upload,
} from "lucide-react";
import { OnboardingModal } from "@/components/OnboardingModal";
import { ZoneSwitcher, ALL_ZONES, type ZoneSummary } from "@/components/ZoneSwitcher";
import { SettingsPanel } from "@/components/SettingsPanel";

interface TableData {
  _id?: string;
  deviceId: string;
  serialNumber?: string;
  name: string;
  venueName: string;
  location: string;
  deviceToken: string;
  pricePerGame: number;
  currency: string;
  subscriptionTier: string;
  subscriptionStatus: string;
  firmwareVersion: string;
  lastHeartbeat: string | null;
  rssi: number;
  localIp: string;
  chipId: string;
  freeHeap?: number;
  resetReason?: string;
  rc522Ok?: boolean;
  offlineQueueCount?: number;
  bootCount?: number;
  totalRacksLifetime: number;
  maintenanceLocked?: boolean;
  isOnline: boolean;
  secondsSinceHeartbeat: number | null;
  todayRacks: number;
  todayRevenue: number;
  isArmed: boolean;
  armedRemainingSec: number;
  otaCmd?: {
    pending: boolean;
    targetVersion: string;
    firmwareUrl: string;
    status: "idle" | "pending" | "downloading" | "success" | "failed";
    errorMessage?: string;
  };
}

interface CardData {
  _id?: string;
  cardUid: string;
  staffName: string;
  staffId: string;
  role: "admin" | "manager" | "staff" | "vip_member";
  status: "active" | "blocked";
  assignedTables: string[];
  dailyLimit: number;
  totalGamesUnlocked: number;
  totalRevenueGenerated: number;
  lastUsedAt: string | null;
  lastUsedDevice: string;
  notes: string;
}

interface EventData {
  _id: string;
  deviceId: string;
  tableName: string;
  event: string;
  trigger: string;
  cardUid: string;
  staffName: string;
  staffId: string;
  priceCharged: number;
  currency: string;
  syncedFromOffline: boolean;
  createdAt: string;
  rssi?: number;
  localIp?: string;
}

interface ReconciliationData {
  _id?: string;
  shiftDate: string;
  shiftLabel: string;
  managerName: string;
  expectedCashFromCoinBox: number;
  expectedRevenueFromStaffRfid: number;
  actualCashCounted: number;
  variance: number;
  notes: string;
  status: "balanced" | "shortage" | "overage";
  createdAt: string;
}

interface ChartItem {
  date: string;
  label: string;
  racks: number;
  rfidRacks: number;
  buttonRacks: number;
  revenue: number;
  alerts: number;
}

interface SummaryData {
  todayRevenue: number;
  todayRacks: number;
  todayRfidRacks: number;
  todayButtonRacks: number;
  todayRfidRevenue: number;
  todayCoinBoxRevenue: number;
  securityAlertsToday: number;
  sevenDayRevenue: number;
  sevenDayRacks: number;
  onlineTablesCount: number;
  totalTablesCount: number;
  activeCardsCount: number;
  blockedCardsCount: number;
}

export type DashboardTab = "overview" | "tables" | "cards" | "financials" | "settings";

export default function DashboardPage() {
  const router = useRouter();
  const pathname = usePathname();
  const { data: session, status } = useSession();
  const userRole = (session?.user as unknown as { role?: string })?.role;
  const isAdmin = userRole === "admin" || userRole === "superadmin";

  useEffect(() => {
    if (status === "unauthenticated") {
      router.push("/auth/signin");
    }
  }, [status, router]);

  const [activeTab, setActiveTab] = useState<DashboardTab>(() => {
    if (typeof window !== "undefined") {
      const p = window.location.pathname;
      if (p.includes("/tables")) return "tables";
      if (p.includes("/cards")) return "cards";
      if (p.includes("/financials")) return "financials";
      if (p.includes("/settings")) return "settings";
    }
    return "overview";
  });

  useEffect(() => {
    if (pathname) {
      if (pathname.includes("/tables")) setActiveTab("tables");
      else if (pathname.includes("/cards")) setActiveTab("cards");
      else if (pathname.includes("/financials")) setActiveTab("financials");
      else if (pathname.includes("/settings")) setActiveTab("settings");
      else if (pathname === "/" || pathname === "") setActiveTab("overview");
    }
  }, [pathname]);

  const changeTab = (tab: DashboardTab) => {
    setActiveTab(tab);
    const targetUrl = tab === "overview" ? "/" : `/${tab}`;
    if (pathname !== targetUrl) {
      router.push(targetUrl);
    }
  };
  const [mobileMenuOpen, setMobileMenuOpen] = useState(false);

  // Zone (venue) scoping: null until the server tells us the saved zone
  const [zones, setZones] = useState<ZoneSummary[]>([]);
  const [zone, setZone] = useState<string>(ALL_ZONES);
  const [zoneSwitching, setZoneSwitching] = useState(false);
  const zoneRef = useRef<string | null>(null);

  const [lastSyncTime, setLastSyncTime] = useState<string>("Just now");

  const [loading, setLoading] = useState(true);
  const [errorMsg, setErrorMsg] = useState<string | null>(null);
  const [isPending, startTransition] = useTransition();

  const [summary, setSummary] = useState<SummaryData | null>(null);
  const [tables, setTables] = useState<TableData[]>([]);
  const [cards, setCards] = useState<CardData[]>([]);
  const [events, setEvents] = useState<EventData[]>([]);
  const [chart, setChart] = useState<ChartItem[]>([]);
  const [reconciliations, setReconciliations] = useState<ReconciliationData[]>([]);

  // Action feedback banner
  const [actionNotification, setActionNotification] = useState<{
    text: string;
    type: "success" | "error";
  } | null>(null);

  // Table action pending states
  const [unlockingDeviceId, setUnlockingDeviceId] = useState<string | null>(null);
  const [maintenanceTogglingId, setMaintenanceTogglingId] = useState<string | null>(null);

  // Hardware Claim Modal State
  const [claimModalOpen, setClaimModalOpen] = useState(false);
  const [claimSerial, setClaimSerial] = useState("");
  const [claimPin, setClaimPin] = useState("");
  const [claimTableName, setClaimTableName] = useState("");
  const [claimVenue, setClaimVenue] = useState("");
  const [claimPrice, setClaimPrice] = useState(500);
  const [claimBusy, setClaimBusy] = useState(false);
  const [claimMsg, setClaimMsg] = useState<{ type: "success" | "error"; text: string } | null>(null);

  // Shift Cash Reconciliation Modal State
  const [recModalOpen, setRecModalOpen] = useState(false);
  const [recShiftLabel, setRecShiftLabel] = useState("Evening Shift");
  const [recManagerName, setRecManagerName] = useState("");
  const [recActualCash, setRecActualCash] = useState<number | "">("");
  const [recNotes, setRecNotes] = useState("");
  const [recBusy, setRecBusy] = useState(false);
  const [recMsg, setRecMsg] = useState<{ type: "success" | "error"; text: string } | null>(null);

  // Event Log Filter & Pagination States
  const [filterTable, setFilterTable] = useState("all");
  const [filterTrigger, setFilterTrigger] = useState("all");
  const [filterEvent, setFilterEvent] = useState("all");
  const [searchEventText, setSearchEventText] = useState("");
  const [overviewPage, setOverviewPage] = useState(1);
  const OVERVIEW_PAGE_SIZE = 8;
  const [auditPage, setAuditPage] = useState(1);
  const AUDIT_PAGE_SIZE = 15;
  const [auditEvents, setAuditEvents] = useState<EventData[]>([]);
  const [auditTotal, setAuditTotal] = useState(0);
  const [auditTotalPages, setAuditTotalPages] = useState(1);
  const [auditLoading, setAuditLoading] = useState(false);

  const [recPage, setRecPage] = useState(1);
  const REC_PAGE_SIZE = 8;
  const [pagedRecs, setPagedRecs] = useState<ReconciliationData[]>([]);
  const [recTotal, setRecTotal] = useState(0);
  const [recTotalPages, setRecTotalPages] = useState(1);

  useEffect(() => {
    setAuditPage(1);
  }, [filterTable, filterTrigger, filterEvent, searchEventText, zone]);

  useEffect(() => {
    setRecPage(1);
  }, [zone]);

  useEffect(() => {
    let cancelled = false;
    const timer = setTimeout(async () => {
      setAuditLoading(true);
      try {
        const params = new URLSearchParams({
          page: String(auditPage),
          limit: String(AUDIT_PAGE_SIZE),
          zone,
          table: filterTable,
          trigger: filterTrigger,
          event: filterEvent,
          search: searchEventText,
        });
        const res = await fetch(`/api/events?${params.toString()}`);
        const json = await res.json();
        if (!cancelled && json.ok) {
          setAuditEvents(json.events || []);
          setAuditTotal(json.total || 0);
          setAuditTotalPages(json.totalPages || 1);
        }
      } catch {
        // Fallback to in-memory events if offline
      } finally {
        if (!cancelled) setAuditLoading(false);
      }
    }, searchEventText ? 200 : 0);
    return () => {
      cancelled = true;
      clearTimeout(timer);
    };
  }, [auditPage, zone, filterTable, filterTrigger, filterEvent, searchEventText, events.length]);

  useEffect(() => {
    let cancelled = false;
    (async () => {
      try {
        const params = new URLSearchParams({
          page: String(recPage),
          limit: String(REC_PAGE_SIZE),
          zone,
        });
        const res = await fetch(`/api/reconciliations?${params.toString()}`);
        const json = await res.json();
        if (!cancelled && json.ok) {
          setPagedRecs(json.reconciliations || []);
          setRecTotal(json.total || 0);
          setRecTotalPages(json.totalPages || 1);
        }
      } catch {
        // Fallback to initial reconciliations
      }
    })();
    return () => {
      cancelled = true;
    };
  }, [recPage, zone, reconciliations.length]);

  // Card Writer Arming Modal State
  const [armModalOpen, setArmModalOpen] = useState(false);
  const [armTargetDevice, setArmTargetDevice] = useState("");
  const [armStaffName, setArmStaffName] = useState("");
  const [armStaffId, setArmStaffId] = useState("");
  const [armRole, setArmRole] = useState("staff");
  const [armStatusMsg, setArmStatusMsg] = useState("");

  // New Card Modal State
  const [cardModalOpen, setCardModalOpen] = useState(false);
  const [newCardUid, setNewCardUid] = useState("");
  const [newCardStaffName, setNewCardStaffName] = useState("");
  const [newCardStaffId, setNewCardStaffId] = useState("");
  const [newCardRole, setNewCardRole] = useState("staff");
  const [newCardLimit, setNewCardLimit] = useState(200);

  // New Table Modal State
  const [tableModalOpen, setTableModalOpen] = useState(false);
  const [tableFormId, setTableFormId] = useState("");
  const [tableFormName, setTableFormName] = useState("");
  const [tableFormLocation, setTableFormLocation] = useState("");
  const [tableFormPrice, setTableFormPrice] = useState(500);

  // Firmware Remote Cloud OTA Modal State
  const [otaModalOpen, setOtaModalOpen] = useState(false);
  const [otaTargetDevice, setOtaTargetDevice] = useState("*"); // "*" for all tables
  const [otaTargetVersion, setOtaTargetVersion] = useState("v2.1.0-esp32");
  const [otaFirmwareUrl, setOtaFirmwareUrl] = useState("");
  const [otaUploading, setOtaUploading] = useState(false);
  const [otaMsg, setOtaMsg] = useState<{ type: "success" | "error"; text: string } | null>(null);
  const [otaFileSelected, setOtaFileSelected] = useState<string | null>(null);

  // Auto-select first active table for actions when loaded
  useEffect(() => {
    if (tables.length > 0) {
      if (!armTargetDevice || !tables.some((t) => t.deviceId === armTargetDevice)) {
        setArmTargetDevice(tables[0].deviceId);
      }
    }
  }, [tables, armTargetDevice]);

  const showNotification = (text: string, type: "success" | "error" = "success") => {
    setActionNotification({ text, type });
    setTimeout(() => {
      setActionNotification(null);
    }, 4000);
  };

  const fetchDashboardData = async () => {
    const requestedZone = zoneRef.current;
    let stale = false;
    try {
      // First load omits the zone so the server applies the user's last selected zone
      const url =
        requestedZone === null
          ? "/api/dashboard"
          : `/api/dashboard?zone=${encodeURIComponent(requestedZone)}`;
      const res = await fetch(url, { cache: "no-store" });
      const data = await res.json();
      // Ignore stale responses for a zone the user has since switched away from
      if (requestedZone !== null && zoneRef.current !== requestedZone) {
        stale = true;
        return;
      }
      if (data.ok) {
        if (data.zone) {
          zoneRef.current = data.zone;
          setZone(data.zone);
        }
        setSummary(data.summary);
        setTables(data.tables || []);
        setCards(data.cards || []);
        setEvents(data.recentEvents || []);
        setChart(data.dailyChart || []);
        setReconciliations(data.reconciliations || []);
        setLastSyncTime(
          new Date().toLocaleTimeString([], {
            hour: "2-digit",
            minute: "2-digit",
          })
        );
        setErrorMsg(null);
      } else {
        setErrorMsg(data.error || "Failed to load telemetry data");
      }
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : "Network error";
      setErrorMsg(message);
    } finally {
      if (!stale) {
        setLoading(false);
        setZoneSwitching(false);
      }
    }
  };

  const fetchZones = async () => {
    try {
      const res = await fetch("/api/zones", { cache: "no-store" });
      const data = await res.json();
      if (data.ok) setZones(data.zones || []);
    } catch {
      // Zone stats are non-critical; keep the last known list
    }
  };

  const handleSelectZone = (nextZone: string) => {
    zoneRef.current = nextZone;
    setZone(nextZone);
    setZoneSwitching(true);
    fetchDashboardData();
    // Persist the selection so it survives reloads/devices
    fetch("/api/zones", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ action: "select", name: nextZone }),
    }).catch(() => {});
  };

  const handleViewZoneReport = (zoneName: string) => {
    handleSelectZone(zoneName);
    changeTab("overview");
    setMobileMenuOpen(false);
  };

  const handleZonesChanged = (next: ZoneSummary[]) => {
    setZones(next);
    // If the zone being viewed was renamed/deleted, fall back to all zones
    const current = zoneRef.current;
    if (current && current !== ALL_ZONES && !next.some((z) => z.name === current)) {
      handleSelectZone(ALL_ZONES);
    } else {
      fetchDashboardData();
    }
  };

  // Default zone for new table pairing: the zone being viewed, else the first zone
  const defaultTableZone = zone !== ALL_ZONES ? zone : zones[0]?.name || "Main Floor";
  const zoneLabel = zone === ALL_ZONES ? "All Zones" : zone;

  useEffect(() => {
    fetchDashboardData();
    fetchZones();
    const interval = setInterval(fetchDashboardData, 8000);
    const zoneInterval = setInterval(fetchZones, 30000);
    return () => {
      clearInterval(interval);
      clearInterval(zoneInterval);
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const handleArmWriter = async (e: React.FormEvent) => {
    e.preventDefault();
    setArmStatusMsg("Transmitting arming command to table node...");
    try {
      const res = await fetch("/api/dashboard", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          action: "arm_card_writer",
          deviceId: armTargetDevice,
          staffName: armStaffName,
          staffId: armStaffId,
          role: armRole,
        }),
      });
      const data = await res.json();
      if (data.ok) {
        setArmStatusMsg(data.message);
        setTimeout(() => {
          setArmModalOpen(false);
          setArmStatusMsg("");
          setArmStaffName("");
          setArmStaffId("");
          fetchDashboardData();
        }, 1800);
      } else {
        setArmStatusMsg(`Error: ${data.error}`);
      }
    } catch (err: unknown) {
      const message = err instanceof Error ? err.message : "Failed";
      setArmStatusMsg(`Failed: ${message}`);
    }
  };

  const handleDisarmWriter = async (deviceId: string) => {
    await fetch("/api/dashboard", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        action: "cancel_card_writer",
        deviceId,
      }),
    });
    fetchDashboardData();
  };

  const handleToggleCardStatus = async (
    cardUid: string,
    currentStatus: "active" | "blocked"
  ) => {
    const nextStatus = currentStatus === "active" ? "blocked" : "active";
    await fetch("/api/dashboard", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        action: "toggle_card_status",
        cardUid,
        status: nextStatus,
      }),
    });
    fetchDashboardData();
  };

  const handleDeleteCard = async (cardUid: string) => {
    if (confirm(`Remove RFID card ${cardUid}?`)) {
      await fetch("/api/dashboard", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          action: "delete_card",
          cardUid,
        }),
      });
      fetchDashboardData();
    }
  };

  const handleAddManualCard = async (e: React.FormEvent) => {
    e.preventDefault();
    await fetch("/api/dashboard", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        action: "upsert_card",
        cardUid: newCardUid,
        staffName: newCardStaffName,
        staffId: newCardStaffId,
        role: newCardRole,
        dailyLimit: newCardLimit,
        status: "active",
      }),
    });
    setCardModalOpen(false);
    setNewCardUid("");
    setNewCardStaffName("");
    setNewCardStaffId("");
    fetchDashboardData();
  };

  const handleSaveTable = async (e: React.FormEvent) => {
    e.preventDefault();
    await fetch("/api/dashboard", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        action: "upsert_table",
        deviceId: tableFormId,
        name: tableFormName,
        venueName: tableFormLocation || defaultTableZone,
        location: tableFormLocation || "Main Floor",
        pricePerGame: tableFormPrice,
      }),
    });
    setTableModalOpen(false);
    setTableFormId("");
    setTableFormName("");
    setTableFormLocation("");
    fetchDashboardData();
  };

  const handleDeleteTable = async (deviceId: string, tableName: string) => {
    if (
      !confirm(
        `Are you sure you want to delete "${tableName}" (${deviceId})?\n\nThis will unpair the device and remove it from your venue dashboard.`
      )
    ) {
      return;
    }

    try {
      const res = await fetch("/api/dashboard", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          action: "delete_table",
          deviceId,
        }),
      });
      const data = await res.json();
      if (data.ok) {
        showNotification(data.message || `Table "${tableName}" removed successfully.`);
        fetchDashboardData();
      } else {
        alert(data.error || "Failed to remove table.");
      }
    } catch {
      alert("Network error while removing table.");
    }
  };

  const handleClaimDevice = async (e: React.FormEvent) => {
    e.preventDefault();
    setClaimBusy(true);
    setClaimMsg(null);
    try {
      const res = await fetch("/api/devices/claim", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          serialNumber: claimSerial,
          claimCode: claimPin,
          tableName: claimTableName,
          venueName: claimVenue || defaultTableZone,
          pricePerGame: claimPrice,
        }),
      });
      const data = await res.json();
      if (data.ok) {
        setClaimMsg({
          type: "success",
          text: data.message || "Hardware table paired successfully!",
        });
        showNotification(data.message || "Hardware table paired successfully!");
        setTimeout(() => {
          setClaimModalOpen(false);
          setClaimSerial("");
          setClaimPin("");
          setClaimTableName("");
          setClaimMsg(null);
          fetchDashboardData();
        }, 1500);
      } else {
        setClaimMsg({
          type: "error",
          text: data.error || "Failed to pair device.",
        });
      }
    } catch (err: unknown) {
      const msg = err instanceof Error ? err.message : "Pairing network error";
      setClaimMsg({ type: "error", text: msg });
    } finally {
      setClaimBusy(false);
    }
  };

  const handleRemoteUnlock = async (deviceId: string, tableName: string) => {
    if (!confirm(`Trigger remote solenoid rack unlock for ${tableName}?`)) return;
    setUnlockingDeviceId(deviceId);
    try {
      const res = await fetch("/api/dashboard", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action: "remote_unlock", deviceId }),
      });
      const data = await res.json();
      if (data.ok) {
        showNotification(`Remote unlock dispatched for ${tableName}!`, "success");
        fetchDashboardData();
      } else {
        showNotification(`Unlock error: ${data.error}`, "error");
      }
    } catch (err: unknown) {
      const msg = err instanceof Error ? err.message : "Unlock request failed";
      showNotification(`Unlock error: ${msg}`, "error");
    } finally {
      setUnlockingDeviceId(null);
    }
  };

  const handleToggleMaintenance = async (
    deviceId: string,
    currentlyLocked: boolean
  ) => {
    setMaintenanceTogglingId(deviceId);
    try {
      const res = await fetch("/api/dashboard", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          action: "toggle_maintenance",
          deviceId,
          locked: !currentlyLocked,
        }),
      });
      const data = await res.json();
      if (data.ok) {
        showNotification(
          `Maintenance mode ${!currentlyLocked ? "ENABLED" : "DISABLED"} for ${deviceId}.`,
          "success"
        );
        fetchDashboardData();
      } else {
        showNotification(`Maintenance error: ${data.error}`, "error");
      }
    } catch (err: unknown) {
      const msg = err instanceof Error ? err.message : "Maintenance toggle failed";
      showNotification(msg, "error");
    } finally {
      setMaintenanceTogglingId(null);
    }
  };

  const handleRecordReconciliation = async (e: React.FormEvent) => {
    e.preventDefault();
    setRecBusy(true);
    setRecMsg(null);
    const expectedCoin = summary?.todayCoinBoxRevenue || 0;
    const expectedRfid = summary?.todayRfidRevenue || 0;
    const actual = Number(recActualCash) || 0;
    try {
      const res = await fetch("/api/dashboard", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          action: "record_reconciliation",
          venueName: zone !== ALL_ZONES ? zone : (tables[0]?.venueName || ""),
          shiftDate: new Date().toISOString().slice(0, 10),
          shiftLabel: recShiftLabel,
          managerName: recManagerName || "Duty Attendant",
          expectedCashFromCoinBox: expectedCoin,
          expectedRevenueFromStaffRfid: expectedRfid,
          actualCashCounted: actual,
          notes: recNotes,
        }),
      });
      const data = await res.json();
      if (data.ok) {
        setRecMsg({
          type: "success",
          text: "Shift reconciliation registered successfully!",
        });
        showNotification(
          "Shift reconciliation registered successfully!",
          "success"
        );
        setTimeout(() => {
          setRecModalOpen(false);
          setRecActualCash("");
          setRecNotes("");
          setRecMsg(null);
          fetchDashboardData();
        }, 1400);
      } else {
        setRecMsg({
          type: "error",
          text: data.error || "Failed to record reconciliation.",
        });
      }
    } catch (err: unknown) {
      const msg = err instanceof Error ? err.message : "Reconciliation request error";
      setRecMsg({ type: "error", text: msg });
    } finally {
      setRecBusy(false);
    }
  };

  const handleUploadFirmware = async (file: File) => {
    setOtaUploading(true);
    setOtaMsg(null);
    try {
      const formData = new FormData();
      formData.append("file", file);
      formData.append("version", otaTargetVersion);

      const res = await fetch("/api/firmware", {
        method: "POST",
        body: formData,
      });
      const data = await res.json();
      if (data.ok) {
        setOtaFirmwareUrl(data.downloadUrl);
        setOtaFileSelected(data.fileName);
        setOtaMsg({
          type: "success",
          text: `Binary uploaded (${(data.sizeBytes / 1024).toFixed(1)} KB). Ready to deploy.`,
        });
      } else {
        setOtaMsg({ type: "error", text: data.error || "Upload failed" });
      }
    } catch (err: unknown) {
      const msg = err instanceof Error ? err.message : "Upload error";
      setOtaMsg({ type: "error", text: msg });
    } finally {
      setOtaUploading(false);
    }
  };

  const handleTriggerOta = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!otaFirmwareUrl.trim()) {
      setOtaMsg({ type: "error", text: "Please provide a valid firmware URL or upload a .bin file." });
      return;
    }
    setOtaUploading(true);
    try {
      const res = await fetch("/api/firmware", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          action: "trigger_ota",
          deviceId: otaTargetDevice,
          targetVersion: otaTargetVersion,
          firmwareUrl: otaFirmwareUrl,
        }),
      });
      const data = await res.json();
      if (data.ok) {
        setOtaMsg({ type: "success", text: data.message });
        showNotification(data.message);
        setTimeout(() => {
          setOtaModalOpen(false);
          setOtaMsg(null);
          fetchDashboardData();
        }, 1800);
      } else {
        setOtaMsg({ type: "error", text: data.error || "Failed to trigger OTA" });
      }
    } catch (err: unknown) {
      const msg = err instanceof Error ? err.message : "OTA trigger error";
      setOtaMsg({ type: "error", text: msg });
    } finally {
      setOtaUploading(false);
    }
  };

  const handleCancelOta = async (deviceId: string) => {
    try {
      const res = await fetch("/api/firmware", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action: "cancel_ota", deviceId }),
      });
      const data = await res.json();
      if (data.ok) {
        showNotification("OTA update cancelled.");
        fetchDashboardData();
      }
    } catch (err) {
      console.error(err);
    }
  };

  if (status === "loading") {
    return (
      <div className="min-h-screen bg-slate-950 flex flex-col items-center justify-center p-4">
        <div className="relative mb-6">
          <div className="w-16 h-16 rounded-2xl bg-emerald-500/10 border border-emerald-500/30 flex items-center justify-center animate-pulse shadow-lg shadow-emerald-950/40">
            <Radio className="w-8 h-8 text-emerald-400 animate-spin" style={{ animationDuration: "3s" }} />
          </div>
        </div>
        <p className="text-sm font-semibold text-white tracking-wide">Smarthive Pool Systems</p>
        <p className="text-xs text-slate-500 mt-1 font-mono">Verifying secure credentials...</p>
      </div>
    );
  }

  if (status === "unauthenticated") {
    return null;
  }

  // Google (OAuth) users must complete their profile before accessing the dashboard
  if (session?.user?.needsOnboarding) {
    return <OnboardingModal />;
  }

  return (
    <div className="flex min-h-screen bg-slate-950 text-slate-100">
      {/* Sidebar for Desktop */}
      <aside className="hidden lg:flex w-64 flex-shrink-0 flex-col sticky top-0 h-screen h-[100dvh] border-r border-slate-800/80 bg-slate-900/70 backdrop-blur-xl p-4 shadow-xl">
        {/* Brand & Venue Header */}
        <div className="flex-shrink-0 p-3 rounded-2xl bg-slate-900 border border-slate-800 mb-5 space-y-3">
          <div className="flex items-center gap-3">
            <div className="relative w-10 h-10 flex-shrink-0 bg-slate-800 rounded-xl overflow-hidden p-1.5 border border-slate-700/80 shadow-sm">
              <Image
                src="/logo.png"
                alt="Smarthive Pool"
                width={32}
                height={32}
                className="object-contain"
              />
            </div>
            <div className="min-w-0 flex-1">
              <div className="font-bold text-sm tracking-tight text-white truncate">
                {session?.user?.companyName || "SmartHive Pool"}
              </div>
              <div className="flex items-center gap-1.5 text-xs text-slate-400">
                <span className="w-2 h-2 rounded-full bg-emerald-400" />
                <span>Club Manager</span>
              </div>
            </div>
          </div>

          <ZoneSwitcher
            zones={zones}
            selected={zone}
            onSelect={handleSelectZone}
            onManage={() => changeTab("settings")}
            busy={zoneSwitching}
          />
        </div>

        {/* Navigation */}
        <nav className="flex-1 min-h-0 overflow-y-auto space-y-1 text-xs font-medium -mx-1 px-1">
          <button
            onClick={() => changeTab("overview")}
            className={`w-full flex items-center justify-between px-3 py-2.5 rounded-xl transition-all ${
              activeTab === "overview"
                ? "bg-emerald-600 text-white font-semibold shadow-sm"
                : "text-slate-400 hover:text-white hover:bg-slate-800/60"
            }`}
          >
            <div className="flex items-center gap-2.5">
              <LayoutDashboard className="w-4 h-4" />
              <span>Overview</span>
            </div>
            {summary && summary.onlineTablesCount > 0 && (
              <span className="w-2 h-2 rounded-full bg-emerald-400" />
            )}
          </button>

          <button
            onClick={() => changeTab("tables")}
            className={`w-full flex items-center justify-between px-3 py-2.5 rounded-xl transition-all ${
              activeTab === "tables"
                ? "bg-emerald-600 text-white font-semibold shadow-sm"
                : "text-slate-400 hover:text-white hover:bg-slate-800/60"
            }`}
          >
            <div className="flex items-center gap-2.5">
              <Server className="w-4 h-4" />
              <span>Tables</span>
            </div>
            <span className="text-[11px] px-2 py-0.5 rounded-full bg-slate-800 text-slate-300 font-mono font-medium">
              {tables.length}
            </span>
          </button>

          <button
            onClick={() => changeTab("cards")}
            className={`w-full flex items-center justify-between px-3 py-2.5 rounded-xl transition-all ${
              activeTab === "cards"
                ? "bg-emerald-600 text-white font-semibold shadow-sm"
                : "text-slate-400 hover:text-white hover:bg-slate-800/60"
            }`}
          >
            <div className="flex items-center gap-2.5">
              <CreditCard className="w-4 h-4" />
              <span>Staff Cards</span>
            </div>
            <span className="text-[11px] px-2 py-0.5 rounded-full bg-slate-800 text-slate-300 font-mono font-medium">
              {cards.length}
            </span>
          </button>

          <button
            onClick={() => changeTab("financials")}
            className={`w-full flex items-center justify-between px-3 py-2.5 rounded-xl transition-all ${
              activeTab === "financials"
                ? "bg-emerald-600 text-white font-semibold shadow-sm"
                : "text-slate-400 hover:text-white hover:bg-slate-800/60"
            }`}
          >
            <div className="flex items-center gap-2.5">
              <Receipt className="w-4 h-4" />
              <span>Reconcile</span>
            </div>
            {summary && summary.securityAlertsToday > 0 && (
              <span className="text-[10px] px-1.5 py-0.5 rounded bg-rose-500/20 text-rose-300 font-mono font-semibold">
                {summary.securityAlertsToday}
              </span>
            )}
          </button>

          <button
            onClick={() => changeTab("settings")}
            className={`w-full flex items-center justify-between px-3 py-2.5 rounded-xl transition-all ${
              activeTab === "settings"
                ? "bg-emerald-600 text-white font-semibold shadow-sm"
                : "text-slate-400 hover:text-white hover:bg-slate-800/60"
            }`}
          >
            <div className="flex items-center gap-2.5">
              <Settings className="w-4 h-4" />
              <span>Settings</span>
            </div>
            <span className="text-[11px] px-2 py-0.5 rounded-full bg-slate-800 text-slate-400 font-mono">
              {zones.length}
            </span>
          </button>

          {/* Admin Console Switcher (Visible ONLY to Admin accounts) */}
          {isAdmin && (
            <Link
              href="/admin"
              className="w-full mt-3 flex items-center justify-between px-3 py-2 rounded-xl transition-all bg-blue-950/60 hover:bg-blue-900/80 text-blue-300 border border-blue-500/40 text-xs font-semibold shadow-sm"
              title="Switch to Central Administrative Console"
            >
              <div className="flex items-center gap-2">
                <Shield className="w-3.5 h-3.5 text-blue-400" />
                <span>Admin Console</span>
              </div>
              <span className="text-[9px] px-1.5 py-0.5 rounded uppercase font-bold bg-blue-500/30 text-blue-200">
                Switch
              </span>
            </Link>
          )}
        </nav>

        {/* Quick Venue Status Widget */}
        <div className="flex-shrink-0 my-3 p-3 rounded-2xl bg-slate-900 border border-slate-800 text-xs space-y-2">
          <div className="flex items-center justify-between text-slate-400">
            <span className="flex items-center gap-1.5">
              <Radio className="w-3.5 h-3.5 text-emerald-400" />
              <span>Tables Online</span>
            </span>
            <span className="text-emerald-400 font-mono font-semibold">
              {summary ? `${summary.onlineTablesCount}/${summary.totalTablesCount}` : "..."}
            </span>
          </div>
          <div className="flex items-center justify-between text-slate-400">
            <span>Today&apos;s Games</span>
            <span className="font-mono text-white font-semibold">
              {summary ? summary.todayRacks : 0} racks
            </span>
          </div>
        </div>

        {/* User Profile Footer with Sign Out */}
        <div className="flex-shrink-0 pt-3 border-t border-slate-800 flex items-center justify-between gap-2">
          <div className="flex items-center gap-2.5 min-w-0">
            <div className="w-8 h-8 rounded-xl bg-slate-800 border border-slate-700 text-slate-200 flex items-center justify-center font-bold text-xs uppercase flex-shrink-0">
              {session?.user?.name
                ? session.user.name
                    .split(" ")
                    .map((n) => n[0])
                    .join("")
                    .slice(0, 2)
                : "SO"}
            </div>
            <div className="min-w-0">
              <div className="text-xs font-semibold text-white truncate">
                {session?.user?.name || "Manager"}
              </div>
              <div className="text-[10px] text-slate-400 truncate">
                {(session?.user as unknown as { role?: string })?.role || "Admin"}
              </div>
            </div>
          </div>
          <button
            onClick={() => signOut({ callbackUrl: "/auth/signin" })}
            className="p-1.5 rounded-lg text-slate-400 hover:text-rose-400 hover:bg-slate-800 transition-colors"
            title="Sign Out"
          >
            <LogOut className="w-4 h-4" />
          </button>
        </div>
      </aside>

      {/* Main Container */}
      <div className="flex-1 flex flex-col min-w-0">
        {/* Top Navbar */}
        <header className="sticky top-0 z-30 flex items-center justify-between px-4 sm:px-6 py-3 bg-slate-900/80 backdrop-blur-xl border-b border-slate-800/80 shadow-sm">
          <div className="flex items-center gap-3">
            {/* Mobile Hamburger Button */}
            <button
              onClick={() => setMobileMenuOpen(!mobileMenuOpen)}
              className="lg:hidden p-2 rounded-xl bg-slate-800 text-slate-300 hover:text-white border border-slate-700/60"
            >
              {mobileMenuOpen ? <X className="w-5 h-5" /> : <Menu className="w-5 h-5" />}
            </button>

            {/* Mobile Logo Branding */}
            <div className="flex items-center gap-2 lg:hidden">
              <div className="w-8 h-8 rounded-xl bg-slate-800 border border-emerald-500/40 p-1 flex items-center justify-center">
                <Image src="/logo.png" alt="Logo" width={24} height={24} />
              </div>
              <span className="font-extrabold text-xs tracking-wider text-white">
                SMARTHIVE
              </span>
            </div>

            {/* Desktop Venue & Zone Scope */}
            <div className="hidden sm:flex items-center gap-2.5 text-xs">
              <div className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-slate-800/80 border border-slate-700/80 text-slate-200 font-medium">
                <Building className="w-3.5 h-3.5 text-emerald-400" />
                <span>{session?.user?.companyName || "Smarthive Arena"}</span>
              </div>

              <div className="flex items-center gap-1.5 px-2.5 py-1.5 rounded-xl bg-emerald-500/10 border border-emerald-500/30 text-emerald-300 text-[11px] font-medium">
                <MapPin className="w-3.5 h-3.5 text-emerald-400" />
                <span>Zone: <strong className="text-white font-semibold">{zoneLabel}</strong></span>
              </div>
            </div>
          </div>

          <div className="flex items-center gap-2 sm:gap-3">
            {/* Unified Refresh & Sync Status Control */}
            <button
              onClick={() => {
                startTransition(() => {
                  fetchDashboardData();
                });
              }}
              disabled={isPending}
              className="flex items-center gap-2 px-3 py-1.5 rounded-xl bg-slate-800/80 hover:bg-slate-700/80 text-slate-300 hover:text-white text-xs font-medium border border-slate-700 transition-all shadow-sm"
              title="Click to refresh telemetry immediately"
            >
              <RefreshCw
                className={`w-3.5 h-3.5 ${
                  isPending ? "animate-spin text-emerald-400" : "text-slate-400"
                }`}
              />
              <span className="flex items-center gap-1.5 text-[11px] font-mono">
                <span className="w-1.5 h-1.5 rounded-full bg-emerald-400 animate-pulse" />
                <span className="hidden md:inline text-slate-400">Synced</span>
                <span className="text-emerald-400 font-semibold">{lastSyncTime}</span>
              </span>
            </button>

            {/* Quick Pair Node Button */}
            <button
              onClick={() => {
                setClaimSerial("");
                setClaimPin("");
                setClaimTableName(`Table 0${tables.length + 1}`);
                setClaimVenue(defaultTableZone);
                setClaimPrice(500);
                setClaimMsg(null);
                setClaimModalOpen(true);
              }}
              className="hidden sm:flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-semibold border border-slate-700 shadow-sm transition-all"
            >
              <KeyRound className="w-3.5 h-3.5 text-emerald-400" />
              <span>Pair Node</span>
            </button>

            {/* Program RFID Card Button */}
            <button
              onClick={() => setArmModalOpen(true)}
              className="flex items-center gap-1.5 px-3.5 py-1.5 rounded-xl bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-bold shadow-md shadow-emerald-950/60 transition-all"
            >
              <Radio className="w-3.5 h-3.5" />
              <span>Program Card</span>
            </button>

            {/* Switch to Admin Console (Visible only to Admins) */}
            {isAdmin && (
              <Link
                href="/admin"
                className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl bg-blue-700/90 hover:bg-blue-600 text-white text-xs font-semibold shadow-md shadow-blue-950/60 border border-blue-500/40 transition-all"
                title="Switch to Central Administrative Console"
              >
                <Shield className="w-3.5 h-3.5" />
                <span className="hidden sm:inline">Admin Console</span>
              </Link>
            )}

            {/* Sign Out (Desktop header action) */}
            <button
              onClick={() => signOut({ callbackUrl: "/auth/signin" })}
              className="hidden sm:flex p-2 rounded-xl text-slate-400 hover:text-rose-400 hover:bg-rose-950/40 border border-transparent hover:border-rose-900/40 transition-all"
              title="Sign Out"
            >
              <LogOut className="w-4 h-4" />
            </button>
          </div>
        </header>

        {/* Mobile Navigation Drawer */}
        {mobileMenuOpen && (
          <div className="lg:hidden bg-slate-900/95 border-b border-slate-800 px-4 py-4 space-y-3 text-xs font-medium backdrop-blur-xl animate-in slide-in-from-top-2">
            {/* User Info on Mobile */}
            <div className="flex items-center justify-between pb-3 border-b border-slate-800">
              <div className="flex items-center gap-2.5">
                <div className="w-8 h-8 rounded-xl bg-emerald-600/20 border border-emerald-500/40 text-emerald-300 flex items-center justify-center font-bold text-xs uppercase">
                  {session?.user?.name
                    ? session.user.name
                        .split(" ")
                        .map((n) => n[0])
                        .join("")
                        .slice(0, 2)
                    : "SO"}
                </div>
                <div>
                  <div className="text-xs font-bold text-white">
                    {session?.user?.name || "Venue Owner"}
                  </div>
                  <div className="text-[10px] text-slate-400">
                    {session?.user?.companyName || "Smarthive Arena"}
                  </div>
                </div>
              </div>
              <button
                onClick={() => signOut({ callbackUrl: "/auth/signin" })}
                className="flex items-center gap-1 px-2.5 py-1 rounded-lg bg-rose-950/40 border border-rose-800 text-rose-300 text-[11px]"
              >
                <LogOut className="w-3 h-3" />
                <span>Sign Out</span>
              </button>
            </div>

            {/* Mobile Zone Switcher */}
            <div className="pt-1 pb-1">
              <ZoneSwitcher
                zones={zones}
                selected={zone}
                onSelect={(z) => {
                  handleSelectZone(z);
                  setMobileMenuOpen(false);
                }}
                onManage={() => {
                  changeTab("settings");
                  setMobileMenuOpen(false);
                }}
                busy={zoneSwitching}
              />
            </div>

            {/* Navigation Buttons */}
            <div className="space-y-1">
              <button
                onClick={() => {
                  changeTab("overview");
                  setMobileMenuOpen(false);
                }}
                className={`w-full flex items-center gap-3 px-3.5 py-2.5 rounded-xl ${
                  activeTab === "overview"
                    ? "bg-emerald-600 text-white font-bold"
                    : "text-slate-300 hover:bg-slate-800"
                }`}
              >
                <LayoutDashboard className="w-4 h-4" />
                <span>Live Overview</span>
              </button>
              <button
                onClick={() => {
                  changeTab("tables");
                  setMobileMenuOpen(false);
                }}
                className={`w-full flex items-center justify-between px-3.5 py-2.5 rounded-xl ${
                  activeTab === "tables"
                    ? "bg-emerald-600 text-white font-bold"
                    : "text-slate-300 hover:bg-slate-800"
                }`}
              >
                <div className="flex items-center gap-3">
                  <Server className="w-4 h-4" />
                  <span>Table Fleet</span>
                </div>
                <span className="text-[11px] px-2 py-0.5 rounded-full bg-slate-800 text-emerald-400 font-mono">
                  {tables.length}
                </span>
              </button>
              <button
                onClick={() => {
                  changeTab("cards");
                  setMobileMenuOpen(false);
                }}
                className={`w-full flex items-center justify-between px-3.5 py-2.5 rounded-xl ${
                  activeTab === "cards"
                    ? "bg-emerald-600 text-white font-bold"
                    : "text-slate-300 hover:bg-slate-800"
                }`}
              >
                <div className="flex items-center gap-3">
                  <CreditCard className="w-4 h-4" />
                  <span>RFID Access & Staff</span>
                </div>
                <span className="text-[11px] px-2 py-0.5 rounded-full bg-slate-800 text-emerald-400 font-mono">
                  {cards.length}
                </span>
              </button>
              <button
                onClick={() => {
                  changeTab("financials");
                  setMobileMenuOpen(false);
                }}
                className={`w-full flex items-center justify-between px-3.5 py-2.5 rounded-xl ${
                  activeTab === "financials"
                    ? "bg-emerald-600 text-white font-bold"
                    : "text-slate-300 hover:bg-slate-800"
                }`}
              >
                <div className="flex items-center gap-3">
                  <Receipt className="w-4 h-4" />
                  <span>Reconciliations & Audit</span>
                </div>
                {summary && summary.securityAlertsToday > 0 && (
                  <span className="text-[10px] px-1.5 py-0.5 rounded bg-rose-500/20 text-rose-300 font-mono font-semibold">
                    {summary.securityAlertsToday} alerts
                  </span>
                )}
              </button>
              <button
                onClick={() => {
                  changeTab("settings");
                  setMobileMenuOpen(false);
                }}
                className={`w-full flex items-center justify-between px-3.5 py-2.5 rounded-xl ${
                  activeTab === "settings"
                    ? "bg-emerald-600 text-white font-bold"
                    : "text-slate-300 hover:bg-slate-800"
                }`}
              >
                <div className="flex items-center gap-3">
                  <Settings className="w-4 h-4" />
                  <span>Profile &amp; Zones</span>
                </div>
                <span className="text-[11px] px-2 py-0.5 rounded-full bg-slate-800 text-slate-300 font-mono">
                  {zones.length}
                </span>
              </button>

              {/* Mobile Admin Console Switcher */}
              {isAdmin && (
                <Link
                  href="/admin"
                  onClick={() => setMobileMenuOpen(false)}
                  className="w-full flex items-center justify-between px-3.5 py-2.5 rounded-xl bg-blue-950/70 border border-blue-500/40 text-blue-300 font-bold"
                >
                  <div className="flex items-center gap-3">
                    <Shield className="w-4 h-4 text-blue-400" />
                    <span>Admin Console</span>
                  </div>
                  <span className="text-[10px] px-2 py-0.5 rounded uppercase font-bold bg-blue-500/30 text-blue-200">
                    Switch
                  </span>
                </Link>
              )}
            </div>

            {/* Quick Actions on Mobile */}
            <div className="pt-2 flex items-center gap-2">
              <button
                onClick={() => {
                  setMobileMenuOpen(false);
                  setClaimSerial("");
                  setClaimPin("");
                  setClaimTableName(`Table 0${tables.length + 1}`);
                  setClaimVenue(defaultTableZone);
                  setClaimPrice(500);
                  setClaimMsg(null);
                  setClaimModalOpen(true);
                }}
                className="flex-1 flex items-center justify-center gap-1.5 py-2 rounded-xl bg-slate-800 text-slate-200 border border-slate-700"
              >
                <KeyRound className="w-3.5 h-3.5 text-emerald-400" />
                <span>Pair Node</span>
              </button>
              <button
                onClick={() => {
                  setMobileMenuOpen(false);
                  setArmModalOpen(true);
                }}
                className="flex-1 flex items-center justify-center gap-1.5 py-2 rounded-xl bg-emerald-600 text-white font-semibold"
              >
                <Radio className="w-3.5 h-3.5" />
                <span>Program Card</span>
              </button>
            </div>
          </div>
        )}

        {/* Content Area */}
        <main className="flex-1 p-3.5 sm:p-6 lg:p-8 overflow-y-auto max-w-7xl w-full mx-auto space-y-6 pb-24 lg:pb-8">
          {actionNotification && (
            <div
              className={`flex items-center gap-2.5 p-3.5 rounded-xl border text-xs sm:text-sm font-medium shadow-lg transition-all ${
                actionNotification.type === "success"
                  ? "bg-emerald-950/70 border-emerald-700/80 text-emerald-300"
                  : "bg-rose-950/70 border-rose-700/80 text-rose-300"
              }`}
            >
              {actionNotification.type === "success" ? (
                <CheckCircle2 className="w-4 h-4 flex-shrink-0 text-emerald-400" />
              ) : (
                <AlertTriangle className="w-4 h-4 flex-shrink-0 text-rose-400" />
              )}
              <span>{actionNotification.text}</span>
            </div>
          )}

          {errorMsg && (
            <div className="flex items-center gap-3 p-4 rounded-xl bg-rose-950/40 border border-rose-800 text-rose-300 text-sm">
              <AlertTriangle className="w-5 h-5 flex-shrink-0 text-rose-400" />
              <span>{errorMsg}</span>
            </div>
          )}

          {/* TAB 1: OVERVIEW */}
          {activeTab === "overview" && (
            <div className="space-y-6">
              {/* Active Zone Filter Header Banner */}
              <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 p-4 rounded-2xl bg-slate-900/90 border border-slate-800 backdrop-blur-xl">
                <div className="flex items-center gap-3">
                  <div className="w-10 h-10 rounded-xl bg-emerald-500/10 border border-emerald-500/30 flex items-center justify-center text-emerald-400">
                    {zone === ALL_ZONES ? <Layers className="w-5 h-5" /> : <Building className="w-5 h-5" />}
                  </div>
                  <div>
                    <div className="flex items-center gap-2 flex-wrap">
                      <h2 className="text-sm sm:text-base font-bold text-white tracking-tight">
                        {zone === ALL_ZONES ? "All Arenas & Zones Report" : `${zone} Telematics & Revenue`}
                      </h2>
                      <span className="text-[10px] px-2 py-0.5 rounded-full bg-emerald-500/10 border border-emerald-500/30 text-emerald-300 font-semibold uppercase">
                        {zone === ALL_ZONES ? "Aggregated Fleet" : "Zone Scoped"}
                      </span>
                    </div>
                    <p className="text-xs text-slate-400 mt-0.5">
                      {zone === ALL_ZONES
                        ? "Showing combined metrics and logs across all branches and floors"
                        : `Viewing filtered reports, active tables, and live logs strictly for ${zone}`}
                    </p>
                  </div>
                </div>

                <div className="flex items-center gap-2 self-start sm:self-auto flex-shrink-0">
                  {zone !== ALL_ZONES && (
                    <button
                      type="button"
                      onClick={() => handleSelectZone(ALL_ZONES)}
                      className="px-3 py-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-300 text-xs font-medium border border-slate-700 transition-colors"
                    >
                      Show All Zones
                    </button>
                  )}
                  <button
                    type="button"
                    onClick={() => changeTab("settings")}
                    className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-emerald-600/15 hover:bg-emerald-600/25 border border-emerald-500/30 text-emerald-300 text-xs font-semibold transition-colors"
                  >
                    <Settings className="w-3.5 h-3.5" />
                    <span>Manage Zones</span>
                  </button>
                </div>
              </div>

              {/* Metric Cards Grid */}
              <div className="grid grid-cols-2 lg:grid-cols-4 gap-3 sm:gap-4">
                {/* Metric 1: Today's Revenue */}
                <div className="p-3.5 sm:p-4 rounded-2xl bg-slate-900 border border-slate-800 shadow-sm relative overflow-hidden flex flex-col justify-between">
                  <div>
                    <div className="flex items-center justify-between text-xs text-slate-400 mb-2">
                      <span className="font-medium truncate">Today&apos;s Revenue</span>
                      <span className="p-1.5 rounded-lg bg-emerald-500/10 text-emerald-400 flex-shrink-0">
                        <Coins className="w-4 h-4" />
                      </span>
                    </div>
                    <div className="text-xl sm:text-2xl font-bold tracking-tight text-white">
                      ₦{summary ? summary.todayRevenue.toLocaleString() : "..."}
                    </div>
                  </div>
                  <div className="mt-3 flex items-center gap-1.5 text-[11px] sm:text-xs text-slate-400 flex-wrap">
                    <span className="text-emerald-400 font-semibold">
                      ₦{summary ? summary.todayRfidRevenue.toLocaleString() : 0}
                    </span>
                    <span className="text-slate-500">Card</span>
                    <span className="text-slate-700">•</span>
                    <span className="text-slate-300 font-semibold">
                      ₦{summary ? summary.todayCoinBoxRevenue.toLocaleString() : 0}
                    </span>
                    <span className="text-slate-500">Coin</span>
                  </div>
                </div>

                {/* Metric 2: Today's Games */}
                <div className="p-3.5 sm:p-4 rounded-2xl bg-slate-900 border border-slate-800 shadow-sm relative overflow-hidden flex flex-col justify-between">
                  <div>
                    <div className="flex items-center justify-between text-xs text-slate-400 mb-2">
                      <span className="font-medium truncate">Games Played</span>
                      <span className="p-1.5 rounded-lg bg-blue-500/10 text-blue-400 flex-shrink-0">
                        <Activity className="w-4 h-4" />
                      </span>
                    </div>
                    <div className="text-xl sm:text-2xl font-bold tracking-tight text-white">
                      {summary ? summary.todayRacks : "..."}
                      <span className="text-xs font-normal text-slate-400 ml-1">racks</span>
                    </div>
                  </div>
                  <div className="mt-3 flex items-center gap-1.5 text-[11px] sm:text-xs text-slate-400 flex-wrap">
                    <span className="text-blue-400 font-semibold">{summary ? summary.todayRfidRacks : 0}</span>
                    <span className="text-slate-500">Staff</span>
                    <span className="text-slate-700">•</span>
                    <span className="text-slate-300 font-semibold">{summary ? summary.todayButtonRacks : 0}</span>
                    <span className="text-slate-500">Manual</span>
                  </div>
                </div>

                {/* Metric 3: Tables Online */}
                <div className="p-3.5 sm:p-4 rounded-2xl bg-slate-900 border border-slate-800 shadow-sm relative overflow-hidden flex flex-col justify-between">
                  <div>
                    <div className="flex items-center justify-between text-xs text-slate-400 mb-2">
                      <span className="font-medium truncate">Tables Online</span>
                      <span className="p-1.5 rounded-lg bg-emerald-500/10 text-emerald-400 flex-shrink-0">
                        <Wifi className="w-4 h-4" />
                      </span>
                    </div>
                    <div className="text-xl sm:text-2xl font-bold tracking-tight text-white flex items-center gap-1.5">
                      <span>{summary ? summary.onlineTablesCount : 0}</span>
                      <span className="text-slate-600 text-base">/</span>
                      <span className="text-slate-400 text-base">{summary ? summary.totalTablesCount : 0}</span>
                    </div>
                  </div>
                  <div className="mt-3 text-[11px] sm:text-xs">
                    {summary && summary.totalTablesCount === 0 ? (
                      <span className="text-slate-500">No tables paired</span>
                    ) : summary && summary.onlineTablesCount === summary.totalTablesCount ? (
                      <span className="text-emerald-400 flex items-center gap-1 font-medium">
                        <CheckCircle2 className="w-3.5 h-3.5" /> All online
                      </span>
                    ) : (
                      <span className="text-amber-400 flex items-center gap-1 font-medium">
                        <AlertTriangle className="w-3.5 h-3.5" /> Check offline
                      </span>
                    )}
                  </div>
                </div>

                {/* Metric 4: Security Alerts */}
                <div className="p-3.5 sm:p-4 rounded-2xl bg-slate-900 border border-slate-800 shadow-sm relative overflow-hidden flex flex-col justify-between">
                  <div>
                    <div className="flex items-center justify-between text-xs text-slate-400 mb-2">
                      <span className="font-medium truncate">Security Status</span>
                      <span className="p-1.5 rounded-lg bg-rose-500/10 text-rose-400 flex-shrink-0">
                        <ShieldAlert className="w-4 h-4" />
                      </span>
                    </div>
                    <div className="text-xl sm:text-2xl font-bold tracking-tight text-white">
                      {summary ? summary.securityAlertsToday : 0}
                      <span className="text-xs font-normal text-slate-400 ml-1">alerts</span>
                    </div>
                  </div>
                  <div className="mt-3 text-[11px] sm:text-xs text-slate-400">
                    <span className="text-slate-300 font-semibold">{summary ? summary.blockedCardsCount : 0}</span>
                    <span> cards blocked</span>
                  </div>
                </div>
              </div>

              {/* 7-Day Revenue & Games Trend */}
              <div className="p-4 sm:p-5 rounded-2xl bg-slate-900 border border-slate-800 shadow-sm">
                <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 mb-5">
                  <div>
                    <h3 className="text-sm font-bold text-white flex items-center gap-2">
                      <TrendingUp className="w-4 h-4 text-emerald-400" />
                      7-Day Revenue &amp; Games
                    </h3>
                    <p className="text-xs text-slate-400">
                      Daily revenue breakdown across all active tables
                    </p>
                  </div>
                  <div className="flex items-center gap-3 text-xs">
                    <div className="flex items-center gap-1.5">
                      <span className="w-2.5 h-2.5 rounded-full bg-emerald-500"></span>
                      <span className="text-slate-300">Revenue (₦)</span>
                    </div>
                    <div className="flex items-center gap-1.5">
                      <span className="w-2.5 h-2.5 rounded-full bg-slate-700"></span>
                      <span className="text-slate-400">Manual Games</span>
                    </div>
                  </div>
                </div>

                {/* CSS Bar Chart */}
                <div className="grid grid-cols-7 gap-1.5 sm:gap-4 items-end h-44 sm:h-48 pt-4 border-b border-slate-800">
                  {chart.map((item, idx) => {
                    const maxRev = Math.max(...chart.map((c) => c.revenue), 1000);
                    const heightPct = Math.max(12, Math.round((item.revenue / maxRev) * 100));
                    return (
                      <div key={idx} className="flex flex-col items-center h-full justify-end group">
                        <div className="text-[10px] text-slate-400 mb-1 opacity-0 group-hover:opacity-100 transition-opacity font-mono">
                          ₦{item.revenue}
                        </div>
                        <div
                          style={{ height: `${heightPct}%` }}
                          className="w-full max-w-[36px] sm:max-w-[42px] bg-emerald-500 rounded-t-md relative flex flex-col justify-end overflow-hidden transition-all group-hover:bg-emerald-400"
                        >
                          {item.buttonRacks > 0 && (
                            <div
                              style={{
                                height: `${Math.round(
                                  (item.buttonRacks / Math.max(item.racks, 1)) * 100
                                )}%`,
                              }}
                              className="w-full bg-slate-700/80"
                            />
                          )}
                        </div>
                        <span className="text-[10px] sm:text-[11px] text-slate-300 mt-2 font-medium truncate max-w-full text-center">
                          {item.label}
                        </span>
                        <span className="text-[9px] sm:text-[10px] text-slate-500 font-mono">
                          {item.racks}g
                        </span>
                      </div>
                    );
                  })}
                </div>
              </div>

              {/* Real-time Fleet Status & Activity Stream */}
              <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
                {/* Active Tables List */}
                <div className="lg:col-span-1 p-5 rounded-2xl bg-slate-900/80 border border-slate-800/90 flex flex-col">
                  <div className="flex items-center justify-between mb-4">
                    <h3 className="text-sm font-semibold text-white flex items-center gap-2">
                      <Server className="w-4 h-4 text-emerald-400" />
                      Table Fleet Status
                    </h3>
                    <button
                      onClick={() => changeTab("tables")}
                      className="text-xs text-emerald-400 hover:text-emerald-300 flex items-center gap-1"
                    >
                      Manage <ChevronRight className="w-3.5 h-3.5" />
                    </button>
                  </div>

                  <div className="space-y-3 flex-1">
                    {tables.length === 0 ? (
                      <div className="p-6 rounded-xl bg-slate-800/30 border border-dashed border-slate-700 flex flex-col items-center justify-center text-center">
                        <Server className="w-8 h-8 text-slate-600 mb-2" />
                        <p className="text-xs font-semibold text-slate-300">No tables connected</p>
                        <p className="text-[11px] text-slate-500 mt-1 max-w-[200px]">
                          Pair a controller device to start receiving automated telemetry.
                        </p>
                        <button
                          onClick={() => {
                            setClaimSerial("");
                            setClaimPin("");
                            setClaimTableName(`Table 0${tables.length + 1}`);
                            setClaimVenue(defaultTableZone);
                            setClaimPrice(500);
                            setClaimMsg(null);
                            setClaimModalOpen(true);
                          }}
                          className="mt-3 px-3 py-1.5 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold transition-all shadow-md"
                        >
                          Pair Table
                        </button>
                      </div>
                    ) : (
                      tables.map((t) => (
                        <div
                          key={t.deviceId}
                          className="p-3.5 rounded-xl bg-slate-800/50 border border-slate-700/60 flex flex-col gap-2"
                        >
                          <div className="flex items-center justify-between">
                            <div className="flex items-center gap-2">
                              <span
                                className={`w-2.5 h-2.5 rounded-full ${
                                  t.isOnline
                                    ? "bg-emerald-500 shadow-sm shadow-emerald-500"
                                    : "bg-slate-600"
                                }`}
                              />
                              <span className="font-semibold text-xs text-white">
                                {t.name}
                              </span>
                            </div>
                            <span
                              className={`text-[10px] px-2 py-0.5 rounded font-mono ${
                                t.isOnline
                                  ? "bg-emerald-500/10 text-emerald-400"
                                  : "bg-slate-800 text-slate-400"
                              }`}
                            >
                              {t.isOnline ? "LIVE ONLINE" : "OFFLINE"}
                            </span>
                          </div>

                          <div className="grid grid-cols-2 text-[11px] text-slate-400 pt-1 border-t border-slate-700/40">
                            <div>
                              Today: <span className="text-white font-mono">{t.todayRacks} racks</span>
                            </div>
                            <div className="text-right">
                              Rev: <span className="text-emerald-400 font-mono">₦{t.todayRevenue}</span>
                            </div>
                          </div>

                          {t.isArmed && (
                            <div className="p-2 rounded-lg bg-amber-950/40 border border-amber-800/60 text-[11px] text-amber-300 flex items-center justify-between">
                              <span className="flex items-center gap-1.5">
                                <Radio className="w-3.5 h-3.5 animate-pulse text-amber-400" />
                                Writer Armed ({t.armedRemainingSec}s)
                              </span>
                              <button
                                onClick={() => handleDisarmWriter(t.deviceId)}
                                className="text-[10px] text-amber-400 hover:underline"
                              >
                                Cancel
                              </button>
                            </div>
                          )}
                        </div>
                      ))
                    )}
                  </div>
                </div>

                {/* Live Real-time Activity Feed / Recent Table Unlocks */}
                <div className="lg:col-span-2 p-4 sm:p-5 rounded-2xl bg-slate-900 border border-slate-800 space-y-4 shadow-sm">
                  <div className="flex items-center justify-between">
                    <h3 className="text-sm font-bold text-white flex items-center gap-2">
                      <Clock className="w-4 h-4 text-emerald-400" />
                      Recent Activity &amp; Releases
                    </h3>
                    <span className="text-[11px] text-slate-400 font-mono">
                      auto-syncing
                    </span>
                  </div>

                  {(() => {
                    const totalOverviewPages = Math.ceil(events.length / OVERVIEW_PAGE_SIZE) || 1;
                    const currentOverviewPage = Math.min(overviewPage, totalOverviewPages);
                    const pagedOverviewEvents = events.slice(
                      (currentOverviewPage - 1) * OVERVIEW_PAGE_SIZE,
                      currentOverviewPage * OVERVIEW_PAGE_SIZE
                    );

                    return (
                      <>
                        {/* Mobile Stacked Card List (< sm) */}
                        <div className="sm:hidden space-y-2.5">
                          {events.length === 0 ? (
                            <div className="py-8 text-center text-slate-500">
                              <Activity className="w-6 h-6 text-slate-600 mx-auto mb-1.5" />
                              <p className="text-xs text-slate-400">No activity recorded yet.</p>
                            </div>
                          ) : (
                            pagedOverviewEvents.map((ev) => {
                              const isAlert =
                                ev.event.includes("attempt") || ev.event.includes("blocked");
                              const isSuccess = ev.event === "button_press" || ev.event === "remote_unlock";

                              return (
                                <div
                                  key={ev._id}
                                  className="p-3 rounded-xl bg-slate-800/40 border border-slate-800 flex items-center justify-between gap-3"
                                >
                                  <div className="min-w-0 flex-1 space-y-1">
                                    <div className="flex items-center gap-2">
                                      <span className="font-semibold text-xs text-white truncate">
                                        {ev.tableName || ev.deviceId}
                                      </span>
                                      <span className="text-[10px] text-slate-500 font-mono">
                                        {new Date(ev.createdAt).toLocaleTimeString([], {
                                          hour: "2-digit",
                                          minute: "2-digit",
                                        })}
                                      </span>
                                    </div>
                                    <div className="flex items-center gap-1.5 flex-wrap">
                                      {isSuccess ? (
                                        <span className="inline-flex items-center gap-1 text-[10px] px-2 py-0.5 rounded-full bg-emerald-500/10 text-emerald-400 font-medium">
                                          <CheckCircle2 className="w-3 h-3" /> Released
                                        </span>
                                      ) : isAlert ? (
                                        <span className="inline-flex items-center gap-1 text-[10px] px-2 py-0.5 rounded-full bg-rose-500/10 text-rose-400 font-medium">
                                          <ShieldAlert className="w-3 h-3" /> Rejected
                                        </span>
                                      ) : (
                                        <span className="inline-flex items-center gap-1 text-[10px] px-2 py-0.5 rounded-full bg-blue-500/10 text-blue-400 font-medium">
                                          <Radio className="w-3 h-3" /> Card Programmed
                                        </span>
                                      )}
                                      <span className="text-[11px] text-slate-400 truncate">
                                        {ev.staffName || "Coin Drop"}
                                      </span>
                                    </div>
                                  </div>
                                  <div className="text-right font-mono font-bold text-xs text-emerald-400 flex-shrink-0">
                                    {ev.priceCharged > 0 ? `₦${ev.priceCharged.toLocaleString()}` : "-"}
                                  </div>
                                </div>
                              );
                            })
                          )}
                        </div>

                        {/* Desktop Table View (>= sm) */}
                        <div className="hidden sm:block overflow-x-auto">
                          <table className="w-full text-left text-xs">
                            <thead>
                              <tr className="border-b border-slate-800 text-slate-400">
                                <th className="pb-2.5 font-medium">Time</th>
                                <th className="pb-2.5 font-medium">Table</th>
                                <th className="pb-2.5 font-medium">Event</th>
                                <th className="pb-2.5 font-medium">Staff / Trigger</th>
                                <th className="pb-2.5 font-medium text-right">Amount</th>
                              </tr>
                            </thead>
                            <tbody className="divide-y divide-slate-800/60 font-normal">
                              {events.length === 0 ? (
                                <tr>
                                  <td colSpan={5} className="py-8 text-center text-slate-500">
                                    <div className="flex flex-col items-center justify-center gap-2">
                                      <Activity className="w-6 h-6 text-slate-600" />
                                      <p className="text-xs text-slate-400">No activity recorded yet.</p>
                                      <p className="text-[11px] text-slate-600">
                                        Ball releases and card scans will appear here in real time.
                                      </p>
                                    </div>
                                  </td>
                                </tr>
                              ) : (
                                pagedOverviewEvents.map((ev) => {
                                  const isAlert =
                                    ev.event.includes("attempt") || ev.event.includes("blocked");
                                  const isSuccess = ev.event === "button_press" || ev.event === "remote_unlock";

                                  return (
                                    <tr key={ev._id} className="hover:bg-slate-800/40">
                                      <td className="py-2.5 text-slate-400 font-mono text-[11px]">
                                        {new Date(ev.createdAt).toLocaleTimeString([], {
                                          hour: "2-digit",
                                          minute: "2-digit",
                                          second: "2-digit",
                                        })}
                                      </td>
                                      <td className="py-2.5 text-slate-200 font-medium">
                                        {ev.tableName || ev.deviceId}
                                      </td>
                                      <td className="py-2.5">
                                        {isSuccess ? (
                                          <span className="inline-flex items-center gap-1 text-[11px] px-2 py-0.5 rounded-full bg-emerald-500/10 text-emerald-400 font-medium">
                                            <CheckCircle2 className="w-3 h-3" /> Ball Released
                                          </span>
                                        ) : isAlert ? (
                                          <span className="inline-flex items-center gap-1 text-[11px] px-2 py-0.5 rounded-full bg-rose-500/10 text-rose-400 font-medium">
                                            <ShieldAlert className="w-3 h-3" /> Security Reject
                                          </span>
                                        ) : (
                                          <span className="inline-flex items-center gap-1 text-[11px] px-2 py-0.5 rounded-full bg-blue-500/10 text-blue-400 font-medium">
                                            <Radio className="w-3 h-3" /> Card Programmed
                                          </span>
                                        )}
                                        {ev.syncedFromOffline && (
                                          <span className="ml-1.5 inline-flex items-center gap-1 text-[10px] px-1.5 py-0.5 rounded-full bg-amber-500/10 text-amber-400 border border-amber-500/30 font-medium">
                                            Offline Synced
                                          </span>
                                        )}
                                      </td>
                                      <td className="py-2.5 text-slate-300">
                                        <div>{ev.staffName || "Coin Drop"}</div>
                                        {ev.cardUid && (
                                          <div className="text-[10px] text-slate-500 font-mono">
                                            UID: {ev.cardUid}
                                          </div>
                                        )}
                                      </td>
                                      <td className="py-2.5 text-right font-mono font-medium text-emerald-400">
                                        {ev.priceCharged > 0 ? `₦${ev.priceCharged.toLocaleString()}` : "-"}
                                      </td>
                                    </tr>
                                  );
                                })
                              )}
                            </tbody>
                          </table>
                        </div>

                        {totalOverviewPages > 1 && (
                          <div className="flex flex-col sm:flex-row items-center justify-between gap-3 pt-3 border-t border-slate-800/80 text-xs">
                            <span className="text-slate-400 text-[11px] font-mono">
                              Showing {(currentOverviewPage - 1) * OVERVIEW_PAGE_SIZE + 1}–{Math.min(currentOverviewPage * OVERVIEW_PAGE_SIZE, events.length)} of {events.length} unlocks
                            </span>
                            <div className="flex items-center gap-1.5">
                              <button
                                type="button"
                                onClick={() => setOverviewPage((p) => Math.max(p - 1, 1))}
                                disabled={currentOverviewPage === 1}
                                className="px-2.5 py-1.5 rounded-lg border border-slate-800 bg-slate-800/60 hover:bg-slate-700/80 disabled:opacity-30 disabled:cursor-not-allowed text-slate-300 hover:text-white transition-all flex items-center gap-1"
                                aria-label="Previous page"
                              >
                                <ChevronLeft className="w-3.5 h-3.5" />
                                <span className="hidden sm:inline">Prev</span>
                              </button>

                              <span className="px-3 py-1 rounded-lg bg-slate-950/60 border border-slate-800 text-slate-300 font-mono text-[11px]">
                                {currentOverviewPage} / {totalOverviewPages}
                              </span>

                              <button
                                type="button"
                                onClick={() => setOverviewPage((p) => Math.min(p + 1, totalOverviewPages))}
                                disabled={currentOverviewPage === totalOverviewPages}
                                className="px-2.5 py-1.5 rounded-lg border border-slate-800 bg-slate-800/60 hover:bg-slate-700/80 disabled:opacity-30 disabled:cursor-not-allowed text-slate-300 hover:text-white transition-all flex items-center gap-1"
                                aria-label="Next page"
                              >
                                <span className="hidden sm:inline">Next</span>
                                <ChevronRight className="w-3.5 h-3.5" />
                              </button>
                            </div>
                          </div>
                        )}
                      </>
                    );
                  })()}
                </div>
              </div>
            </div>
          )}

          {/* TAB 2: TABLE FLEET */}
          {activeTab === "tables" && (
            <div className="space-y-6">
              <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
                <div>
                  <h2 className="text-lg font-bold text-white flex items-center gap-2">
                    <Server className="w-5 h-5 text-emerald-400" />
                    Multi-Table Fleet & Device Management
                  </h2>
                  <p className="text-xs text-slate-400">
                    Pair hardware nodes with 6-digit Claim PINs, toggle maintenance mode, unlock solenoids, and audit RSSI health
                  </p>
                </div>
                <div className="flex items-center gap-2 flex-wrap">
                  <button
                    onClick={() => {
                      setOtaTargetDevice("*");
                      setOtaMsg(null);
                      setOtaModalOpen(true);
                    }}
                    className="flex items-center gap-1.5 px-3 py-2 rounded-lg bg-blue-700/80 hover:bg-blue-600 text-white text-xs font-semibold shadow-md transition-all border border-blue-500/40"
                    title="Remotely flash firmware over Wi-Fi without physical cables"
                  >
                    <UploadCloud className="w-4 h-4" />
                    <span>Deploy Firmware (OTA)</span>
                  </button>
                  <button
                    onClick={() => {
                      setClaimSerial("");
                      setClaimPin("");
                      setClaimTableName(`Table 0${tables.length + 1}`);
                      setClaimVenue(defaultTableZone);
                      setClaimPrice(500);
                      setClaimMsg(null);
                      setClaimModalOpen(true);
                    }}
                    className="flex items-center gap-1.5 px-3 py-2 rounded-lg bg-emerald-700/80 hover:bg-emerald-600 text-white text-xs font-semibold shadow-md transition-all border border-emerald-500/40"
                  >
                    <KeyRound className="w-4 h-4" />
                    <span>Pair Hardware Device</span>
                  </button>
                  <button
                    onClick={() => {
                      setTableFormId(`snooker_table_${tables.length + 1}`);
                      setTableFormName(`Table 0${tables.length + 1}`);
                      setTableFormLocation(defaultTableZone);
                      setTableFormPrice(500);
                      setTableModalOpen(true);
                    }}
                    className="flex items-center gap-1.5 px-3 py-2 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-semibold shadow-md transition-all border border-slate-700"
                  >
                    <Plus className="w-4 h-4" />
                    <span>Add Manual Node</span>
                  </button>
                </div>
              </div>

              {/* Table Cards Grid */}
              {tables.length === 0 ? (
                <div className="p-12 rounded-2xl bg-slate-900/60 border border-dashed border-slate-700/80 flex flex-col items-center justify-center text-center">
                  <div className="w-12 h-12 rounded-2xl bg-emerald-500/10 border border-emerald-500/30 flex items-center justify-center mb-3">
                    <Server className="w-6 h-6 text-emerald-400" />
                  </div>
                  <h3 className="text-sm font-semibold text-white">No Table Controllers Connected</h3>
                  <p className="text-xs text-slate-400 mt-1 max-w-md">
                    Connect your controller devices using the hardware serial number & claim PIN, or add a manual device definition.
                  </p>
                  <div className="flex flex-wrap items-center justify-center gap-3 mt-4">
                    <button
                      onClick={() => {
                        setClaimSerial("");
                        setClaimPin("");
                        setClaimTableName(`Table 0${tables.length + 1}`);
                        setClaimVenue(defaultTableZone);
                        setClaimPrice(500);
                        setClaimMsg(null);
                        setClaimModalOpen(true);
                      }}
                      className="flex items-center gap-1.5 px-3.5 py-2 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold transition-all shadow-md"
                    >
                      <KeyRound className="w-4 h-4" />
                      <span>Pair Table Node</span>
                    </button>
                    <button
                      onClick={() => {
                        setTableFormId(`snooker_table_${tables.length + 1}`);
                        setTableFormName(`Table 0${tables.length + 1}`);
                        setTableFormLocation(defaultTableZone);
                        setTableFormPrice(500);
                        setTableModalOpen(true);
                      }}
                      className="flex items-center gap-1.5 px-3.5 py-2 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-semibold border border-slate-700"
                    >
                      <Plus className="w-4 h-4" />
                      <span>Add Manual Node</span>
                    </button>
                  </div>
                </div>
              ) : (
                <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-5">
                  {tables.map((t) => (
                    <div
                      key={t.deviceId}
                    className={`p-5 rounded-2xl bg-slate-900/80 border flex flex-col justify-between transition-all ${
                      t.maintenanceLocked
                        ? "border-amber-700/70 shadow-lg shadow-amber-950/20"
                        : "border-slate-800/90"
                    }`}
                  >
                    <div>
                      {/* Header */}
                      <div className="flex items-start justify-between mb-3">
                        <div>
                          <div className="flex items-center gap-2">
                            <span
                              className={`w-2.5 h-2.5 rounded-full ${
                                t.isOnline
                                  ? "bg-emerald-500 shadow-sm shadow-emerald-500"
                                  : "bg-slate-600"
                              }`}
                            />
                            <h3 className="font-bold text-sm text-white">{t.name}</h3>
                          </div>
                          <p className="text-xs text-slate-400 mt-0.5">
                            {t.location && t.location !== "Unassigned"
                              ? t.location
                              : t.venueName && t.venueName !== "Unassigned"
                              ? t.venueName
                              : "Main Floor"}
                          </p>
                        </div>
                        <div className="flex flex-col items-end gap-1">
                          <span className="text-[10px] px-2 py-0.5 rounded font-mono bg-slate-800 text-slate-300 border border-slate-700">
                            {t.deviceId}
                          </span>
                          {t.maintenanceLocked ? (
                            <span className="text-[10px] px-2 py-0.5 rounded font-semibold bg-amber-500/15 text-amber-400 border border-amber-500/40 flex items-center gap-1">
                              <Lock className="w-2.5 h-2.5" /> Maintenance Lock
                            </span>
                          ) : (
                            <span className="text-[10px] px-2 py-0.5 rounded font-semibold bg-emerald-500/15 text-emerald-400 border border-emerald-500/40 flex items-center gap-1">
                              <Check className="w-2.5 h-2.5" /> Active Service
                            </span>
                          )}
                        </div>
                      </div>

                      {/* Hardware Telemetry Specs */}
                      <div className="space-y-2 py-3 border-y border-slate-800 text-xs">
                        {t.serialNumber && (
                          <div className="flex justify-between">
                            <span className="text-slate-400">Serial Number:</span>
                            <span className="font-mono text-emerald-400 font-semibold">{t.serialNumber}</span>
                          </div>
                        )}
                        <div className="flex justify-between">
                          <span className="text-slate-400">Wi-Fi RSSI:</span>
                          <span className="font-mono text-slate-200">{t.rssi} dBm (Good)</span>
                        </div>
                        <div className="flex justify-between">
                          <span className="text-slate-400">LAN IP Address:</span>
                          <span className="font-mono text-slate-200">{t.localIp}</span>
                        </div>
                        <div className="flex justify-between">
                          <span className="text-slate-400">Device Hardware ID:</span>
                          <span className="font-mono text-slate-200">{t.chipId}</span>
                        </div>
                        <div className="flex justify-between">
                          <span className="text-slate-400">Rack Price:</span>
                          <span className="font-mono text-emerald-400 font-semibold">
                            ₦{t.pricePerGame} / rack
                          </span>
                        </div>
                        <div className="flex justify-between">
                          <span className="text-slate-400">Firmware:</span>
                          <span className="font-mono text-slate-200">
                            {t.firmwareVersion || "v2.0.0-esp32"}
                          </span>
                        </div>
                        <div className="flex justify-between">
                          <span className="text-slate-400">Lifetime Games:</span>
                          <span className="font-mono text-white">{t.totalRacksLifetime}</span>
                        </div>
                        {typeof t.rc522Ok === "boolean" && (
                          <div className="flex justify-between">
                            <span className="text-slate-400">RFID Reader:</span>
                            <span className={`font-mono font-semibold ${t.rc522Ok ? "text-emerald-400" : "text-rose-400"}`}>
                              {t.rc522Ok ? "Healthy" : "Check Wiring"}
                            </span>
                          </div>
                        )}
                        {typeof t.freeHeap === "number" && t.freeHeap > 0 && (
                          <div className="flex justify-between">
                            <span className="text-slate-400">Free Heap / Boots:</span>
                            <span className="font-mono text-slate-200">
                              {(t.freeHeap / 1024).toFixed(1)} KB{typeof t.bootCount === "number" && t.bootCount > 0 ? ` (${t.bootCount} boots)` : ""}
                            </span>
                          </div>
                        )}
                        {t.resetReason && (
                          <div className="flex justify-between">
                            <span className="text-slate-400">Last Reset:</span>
                            <span className="font-mono text-slate-300 truncate max-w-[160px]" title={t.resetReason}>
                              {t.resetReason}
                            </span>
                          </div>
                        )}
                        {typeof t.offlineQueueCount === "number" && t.offlineQueueCount > 0 && (
                          <div className="flex justify-between">
                            <span className="text-amber-400">Offline Buffer:</span>
                            <span className="font-mono text-amber-300 font-semibold">
                              {t.offlineQueueCount} pending
                            </span>
                          </div>
                        )}
                      </div>

                      {/* OTA Pending Status Banner */}
                      {t.otaCmd?.pending && (
                        <div className="mt-2.5 p-2 rounded-lg bg-blue-950/70 border border-blue-500/40 text-xs flex items-center justify-between gap-2">
                          <div className="flex items-center gap-1.5 text-blue-300">
                            <UploadCloud className="w-3.5 h-3.5 animate-pulse text-blue-400" />
                            <span className="font-medium">
                              OTA Queued: <span className="font-mono text-white">{t.otaCmd.targetVersion}</span>
                            </span>
                          </div>
                          <button
                            type="button"
                            onClick={() => handleCancelOta(t.deviceId)}
                            className="text-[10px] px-2 py-0.5 rounded bg-blue-900/80 hover:bg-rose-900/80 text-blue-200 hover:text-rose-200 border border-blue-700/60 transition-colors"
                          >
                            Cancel
                          </button>
                        </div>
                      )}
                    </div>

                    {/* Quick Remote Actions */}
                    <div className="mt-4 pt-3 space-y-2">
                      <div className="flex items-center gap-2">
                        {/* Remote Solenoid Unlock Button */}
                        <button
                          onClick={() => handleRemoteUnlock(t.deviceId, t.name)}
                          disabled={unlockingDeviceId === t.deviceId || Boolean(t.maintenanceLocked)}
                          className={`flex-1 flex items-center justify-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-semibold shadow-sm transition-all ${
                            t.maintenanceLocked
                              ? "bg-slate-800 text-slate-500 cursor-not-allowed border border-slate-700/50"
                              : "bg-emerald-600 hover:bg-emerald-500 text-white"
                          }`}
                          title={
                            t.maintenanceLocked
                              ? "Unlock disabled while table is in maintenance lock"
                              : "Remotely open ball solenoid"
                          }
                        >
                          <Unlock
                            className={`w-3.5 h-3.5 ${
                              unlockingDeviceId === t.deviceId ? "animate-spin" : ""
                            }`}
                          />
                          <span>{unlockingDeviceId === t.deviceId ? "Unlocking..." : "Remote Unlock"}</span>
                        </button>

                        {/* Maintenance Lock Toggle */}
                        <button
                          onClick={() => handleToggleMaintenance(t.deviceId, Boolean(t.maintenanceLocked))}
                          disabled={maintenanceTogglingId === t.deviceId}
                          className={`px-3 py-1.5 rounded-lg text-xs font-medium border flex items-center gap-1 transition-all ${
                            t.maintenanceLocked
                              ? "bg-amber-950/60 border-amber-600 text-amber-300 hover:bg-amber-900/80"
                              : "bg-slate-800 hover:bg-slate-700 text-slate-300 border-slate-700"
                          }`}
                          title={t.maintenanceLocked ? "Deactivate Maintenance Lock" : "Activate Maintenance Lock"}
                        >
                          <Lock className="w-3.5 h-3.5" />
                          <span className="hidden sm:inline">
                            {t.maintenanceLocked ? "Locked" : "Lock"}
                          </span>
                        </button>
                      </div>

                      <div className="flex items-center justify-between gap-2">
                        <button
                          onClick={() => {
                            setArmTargetDevice(t.deviceId);
                            setArmModalOpen(true);
                          }}
                          className="flex-1 flex items-center justify-center gap-1.5 px-3 py-1.5 rounded-lg bg-slate-800/90 hover:bg-slate-700 text-emerald-400 text-xs font-medium border border-slate-700"
                        >
                          <Radio className="w-3.5 h-3.5" />
                          <span>Arm RFID Writer</span>
                        </button>

                        <button
                          onClick={() => {
                            setOtaTargetDevice(t.deviceId);
                            setOtaMsg(null);
                            setOtaModalOpen(true);
                          }}
                          className="p-2 rounded-lg bg-slate-800 hover:bg-slate-700 text-blue-400 border border-slate-700 hover:border-blue-500/50 transition-colors"
                          title="Push Remote OTA Firmware to this device"
                        >
                          <UploadCloud className="w-3.5 h-3.5" />
                        </button>

                        <button
                          onClick={() => {
                            setTableFormId(t.deviceId);
                            setTableFormName(t.name);
                            setTableFormLocation(t.location);
                            setTableFormPrice(t.pricePerGame);
                            setTableModalOpen(true);
                          }}
                          className="p-2 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-300 border border-slate-700"
                          title="Configure Table Settings"
                        >
                          <Sliders className="w-3.5 h-3.5" />
                        </button>

                        <button
                          onClick={() => handleDeleteTable(t.deviceId, t.name)}
                          className="p-2 rounded-lg bg-slate-800 hover:bg-rose-500/20 text-slate-400 hover:text-rose-400 border border-slate-700 hover:border-rose-500/40 transition-colors"
                          title="Delete / Unpair Table"
                        >
                          <Trash2 className="w-3.5 h-3.5" />
                        </button>
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            )}
            </div>
          )}

          {/* TAB 3: RFID ACCESS & STAFF MANAGEMENT */}
          {activeTab === "cards" && (
            <div className="space-y-6">
              <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
                <div>
                  <h2 className="text-lg font-bold text-white flex items-center gap-2">
                    <CreditCard className="w-5 h-5 text-emerald-400" />
                    RFID Card Whitelist & Staff Accountability
                  </h2>
                  <p className="text-xs text-slate-400">
                    Remotely enroll cards over-the-air, enforce shift limits, and audit games unlocked per staff
                  </p>
                </div>
                <div className="flex items-center gap-2">
                  <button
                    onClick={() => setCardModalOpen(true)}
                    className="flex items-center gap-1.5 px-3.5 py-2 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-medium border border-slate-700"
                  >
                    <Plus className="w-3.5 h-3.5" />
                    <span>Manual Add Card</span>
                  </button>
                  <button
                    onClick={() => setArmModalOpen(true)}
                    className="flex items-center gap-1.5 px-3.5 py-2 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold shadow-md"
                  >
                    <Radio className="w-3.5 h-3.5" />
                    <span>Over-The-Air Program</span>
                  </button>
                </div>
              </div>

              {/* Cards Section */}
              <div className="p-4 sm:p-5 rounded-2xl bg-slate-900 border border-slate-800 shadow-sm">
                {/* Mobile Cards Stack (< sm) */}
                <div className="sm:hidden space-y-3">
                  {cards.length === 0 ? (
                    <div className="py-8 text-center text-slate-500">
                      <CreditCard className="w-8 h-8 text-slate-600 mx-auto mb-2" />
                      <p className="text-xs text-slate-400 font-medium">No staff cards enrolled</p>
                    </div>
                  ) : (
                    cards.map((c) => (
                      <div
                        key={c.cardUid}
                        className="p-3.5 rounded-xl bg-slate-800/40 border border-slate-800 space-y-3"
                      >
                        <div className="flex items-start justify-between gap-2">
                          <div>
                            <div className="font-bold text-sm text-white">{c.staffName}</div>
                            <div className="text-xs text-slate-400 font-mono mt-0.5">
                              ID: {c.staffId} · {c.cardUid}
                            </div>
                          </div>
                          <span className="text-[10px] px-2 py-0.5 rounded font-mono uppercase bg-slate-800 text-slate-300 border border-slate-700">
                            {c.role}
                          </span>
                        </div>

                        <div className="grid grid-cols-2 gap-2 text-xs py-2 border-y border-slate-800/80">
                          <div>
                            <span className="text-slate-400">Games: </span>
                            <span className="font-semibold text-white">{c.totalGamesUnlocked}</span>
                          </div>
                          <div className="text-right">
                            <span className="text-slate-400">Revenue: </span>
                            <span className="font-semibold text-emerald-400 font-mono">
                              ₦{c.totalRevenueGenerated.toLocaleString()}
                            </span>
                          </div>
                        </div>

                        <div className="flex items-center justify-between gap-2 pt-1">
                          <button
                            onClick={() => handleToggleCardStatus(c.cardUid, c.status)}
                            className={`flex-1 py-2 px-3 rounded-xl text-xs font-semibold flex items-center justify-center gap-1.5 transition-all ${
                              c.status === "active"
                                ? "bg-emerald-500/15 text-emerald-400 border border-emerald-500/30"
                                : "bg-rose-500/15 text-rose-400 border border-rose-500/30"
                            }`}
                          >
                            {c.status === "active" ? (
                              <>
                                <CheckCircle2 className="w-3.5 h-3.5" />
                                <span>Active</span>
                              </>
                            ) : (
                              <>
                                <Ban className="w-3.5 h-3.5" />
                                <span>Blocked</span>
                              </>
                            )}
                          </button>
                          <button
                            onClick={() => handleDeleteCard(c.cardUid)}
                            className="p-2 rounded-xl bg-slate-800 text-slate-400 hover:text-rose-400 hover:bg-rose-950/40 border border-slate-700"
                            title="Remove Card"
                          >
                            <Trash2 className="w-4 h-4" />
                          </button>
                        </div>
                      </div>
                    ))
                  )}
                </div>

                {/* Desktop Cards Table (>= sm) */}
                <div className="hidden sm:block overflow-x-auto">
                  <table className="w-full text-left text-xs">
                    <thead>
                      <tr className="border-b border-slate-800 text-slate-400">
                        <th className="pb-3 font-medium">Card UID</th>
                        <th className="pb-3 font-medium">Staff Member</th>
                        <th className="pb-3 font-medium">Role</th>
                        <th className="pb-3 font-medium">Status</th>
                        <th className="pb-3 font-medium">Games Unlocked</th>
                        <th className="pb-3 font-medium">Revenue Accounted</th>
                        <th className="pb-3 font-medium">Last Active</th>
                        <th className="pb-3 font-medium text-right">Actions</th>
                      </tr>
                    </thead>
                    <tbody className="divide-y divide-slate-800/60">
                      {cards.length === 0 ? (
                        <tr>
                          <td colSpan={8} className="py-12 text-center text-slate-500">
                            <div className="flex flex-col items-center justify-center gap-2">
                              <CreditCard className="w-8 h-8 text-slate-600" />
                              <p className="text-sm font-semibold text-slate-300">No RFID Cards Enrolled</p>
                              <p className="text-xs text-slate-500 max-w-sm">
                                Enroll manager or staff RFID cards wirelessly by arming a table reader, or enter an existing card UID manually.
                              </p>
                              <div className="flex flex-wrap items-center justify-center gap-2 mt-3">
                                <button
                                  onClick={() => setArmModalOpen(true)}
                                  className="px-3 py-1.5 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold shadow-md"
                                >
                                  Over-The-Air Program
                                </button>
                                <button
                                  onClick={() => setCardModalOpen(true)}
                                  className="px-3 py-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-300 text-xs font-medium border border-slate-700"
                                >
                                  Manual Add Card
                                </button>
                              </div>
                            </div>
                          </td>
                        </tr>
                      ) : (
                        cards.map((c) => (
                          <tr key={c.cardUid} className="hover:bg-slate-800/40">
                            <td className="py-3 font-mono text-emerald-400 font-semibold">
                              {c.cardUid}
                            </td>
                            <td className="py-3">
                              <div className="font-semibold text-white">{c.staffName}</div>
                              <div className="text-[10px] text-slate-400 font-mono">
                                ID: {c.staffId}
                              </div>
                            </td>
                            <td className="py-3">
                              <span className="text-[10px] px-2 py-0.5 rounded font-mono uppercase bg-slate-800 text-slate-300">
                                {c.role}
                              </span>
                            </td>
                            <td className="py-3">
                              <button
                                onClick={() => handleToggleCardStatus(c.cardUid, c.status)}
                                className={`inline-flex items-center gap-1 text-[11px] px-2.5 py-0.5 rounded-full font-medium transition-colors ${
                                  c.status === "active"
                                    ? "bg-emerald-500/10 text-emerald-400 hover:bg-rose-500/10 hover:text-rose-400"
                                    : "bg-rose-500/10 text-rose-400 hover:bg-emerald-500/10 hover:text-emerald-400"
                                }`}
                              >
                                {c.status === "active" ? (
                                  <>
                                    <CheckCircle2 className="w-3 h-3" /> ACTIVE
                                  </>
                                ) : (
                                  <>
                                    <Ban className="w-3 h-3" /> BLOCKED
                                  </>
                                )}
                              </button>
                            </td>
                            <td className="py-3 font-mono text-slate-200">
                              {c.totalGamesUnlocked} racks
                            </td>
                            <td className="py-3 font-mono text-emerald-400 font-medium">
                              ₦{c.totalRevenueGenerated.toLocaleString()}
                            </td>
                            <td className="py-3 text-slate-400 font-mono text-[11px]">
                              {c.lastUsedAt ? new Date(c.lastUsedAt).toLocaleTimeString() : "Never"}
                            </td>
                            <td className="py-3 text-right">
                              <button
                                onClick={() => handleDeleteCard(c.cardUid)}
                                className="p-1.5 rounded-lg bg-slate-800/80 hover:bg-rose-900/60 text-slate-400 hover:text-rose-300"
                              >
                                <Trash2 className="w-3.5 h-3.5" />
                              </button>
                            </td>
                          </tr>
                        ))
                      )}
                    </tbody>
                  </table>
                </div>
              </div>
            </div>
          )}

          {/* TAB 4: RECONCILIATION & LOGS */}
          {activeTab === "financials" && (
            <div className="space-y-6">
              <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
                <div>
                  <h2 className="text-lg font-bold text-white flex items-center gap-2">
                    <Receipt className="w-5 h-5 text-emerald-400" />
                    Shift Reconciliation & Anti-Pilferage Auditing
                  </h2>
                  <p className="text-xs text-slate-400">
                    Audit cash collections, match coin-box triggers with staff RFID unlocks, and record manager sign-offs
                  </p>
                </div>
                <button
                  onClick={() => {
                    setRecActualCash("");
                    setRecNotes("");
                    setRecMsg(null);
                    setRecModalOpen(true);
                  }}
                  className="flex items-center gap-1.5 px-3.5 py-2 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold shadow-md transition-all self-start sm:self-auto"
                >
                  <Plus className="w-4 h-4" />
                  <span>Record Shift Cash Count</span>
                </button>
              </div>

              {/* Financial Breakdown Cards */}
              <div className="grid grid-cols-1 md:grid-cols-3 gap-5">
                <div className="p-5 rounded-2xl bg-slate-900/80 border border-slate-800/90 shadow-sm">
                  <span className="text-xs text-slate-400">Expected Coin Box Cash (Push Button)</span>
                  <div className="text-2xl font-bold font-mono text-white mt-1">
                    ₦{summary ? summary.todayCoinBoxRevenue.toLocaleString() : 0}
                  </div>
                  <p className="text-[11px] text-slate-500 mt-2">
                    Direct physical cash dropped by players into mechanical coin slots
                  </p>
                </div>

                <div className="p-5 rounded-2xl bg-slate-900/80 border border-slate-800/90 shadow-sm">
                  <span className="text-xs text-slate-400">Expected Staff RFID Collected Cash</span>
                  <div className="text-2xl font-bold font-mono text-emerald-400 mt-1">
                    ₦{summary ? summary.todayRfidRevenue.toLocaleString() : 0}
                  </div>
                  <p className="text-[11px] text-slate-500 mt-2">
                    Cash collected by table attendants using registered MIFARE RFID cards
                  </p>
                </div>

                <div className="p-5 rounded-2xl bg-slate-900/80 border border-slate-800/90 shadow-sm">
                  <span className="text-xs text-slate-400">7-Day Gross Venue Turnover</span>
                  <div className="text-2xl font-bold font-mono text-blue-400 mt-1">
                    ₦{summary ? summary.sevenDayRevenue.toLocaleString() : 0}
                  </div>
                  <p className="text-[11px] text-slate-500 mt-2">
                    Across {summary ? summary.sevenDayRacks : 0} total game unlocking cycles
                  </p>
                </div>
              </div>

              {/* Shift Reconciliations Ledger */}
              <div className="p-4 sm:p-5 rounded-2xl bg-slate-900 border border-slate-800 space-y-4 shadow-sm">
                <div className="flex items-center justify-between">
                  <h3 className="text-sm font-bold text-white flex items-center gap-2">
                    <Receipt className="w-4 h-4 text-emerald-400" />
                    Shift Cash Count Reconciliations
                  </h3>
                  <span className="text-xs text-slate-400 font-mono">
                    {reconciliations.length} shift logs
                  </span>
                </div>

                {reconciliations.length === 0 ? (
                  <div className="py-10 text-center text-xs text-slate-400 border border-dashed border-slate-800 rounded-xl flex flex-col items-center justify-center gap-2">
                    <Receipt className="w-8 h-8 text-slate-600 mb-1" />
                    <p className="font-semibold text-slate-300">No Shift Reconciliation Records Logged</p>
                    <p className="text-slate-500 max-w-sm">
                      Audit shift coin box collections against automated ball drops and staff RFID logs to eliminate revenue skimming.
                    </p>
                    <button
                      onClick={() => {
                        setRecActualCash("");
                        setRecNotes("");
                        setRecMsg(null);
                        setRecModalOpen(true);
                      }}
                      className="mt-2 px-3 py-1.5 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold shadow-md"
                    >
                      Record Shift Cash Count
                    </button>
                  </div>
                ) : (
                  <>
                    {/* Mobile Shift Cards (< sm) */}
                    <div className="sm:hidden space-y-3">
                      {reconciliations.map((rec) => {
                        const totalExpected =
                          (rec.expectedCashFromCoinBox || 0) +
                          (rec.expectedRevenueFromStaffRfid || 0);
                        const isBalanced = rec.variance === 0;
                        const isShort = rec.variance < 0;

                        return (
                          <div
                            key={rec._id}
                            className="p-3.5 rounded-xl bg-slate-800/40 border border-slate-800 space-y-2.5"
                          >
                            <div className="flex items-start justify-between gap-2">
                              <div>
                                <div className="font-bold text-sm text-white">{rec.shiftLabel}</div>
                                <div className="text-xs text-slate-400 font-mono mt-0.5">
                                  {rec.shiftDate} · {rec.managerName}
                                </div>
                              </div>
                              <span
                                className={`inline-flex items-center gap-1 text-[10px] px-2 py-0.5 rounded-full font-semibold uppercase ${
                                  isBalanced
                                    ? "bg-emerald-500/15 text-emerald-400 border border-emerald-500/30"
                                    : isShort
                                    ? "bg-rose-500/15 text-rose-400 border border-rose-500/30"
                                    : "bg-amber-500/15 text-amber-400 border border-amber-500/30"
                                }`}
                              >
                                {isBalanced ? (
                                  <CheckCircle2 className="w-2.5 h-2.5" />
                                ) : (
                                  <AlertTriangle className="w-2.5 h-2.5" />
                                )}
                                {rec.status}
                              </span>
                            </div>

                            <div className="grid grid-cols-3 gap-2 text-xs py-2 border-y border-slate-800/80">
                              <div>
                                <span className="text-[10px] text-slate-500 block">Expected</span>
                                <span className="font-mono text-slate-300">₦{totalExpected.toLocaleString()}</span>
                              </div>
                              <div>
                                <span className="text-[10px] text-slate-500 block">Actual</span>
                                <span className="font-mono font-bold text-white">₦{rec.actualCashCounted.toLocaleString()}</span>
                              </div>
                              <div className="text-right">
                                <span className="text-[10px] text-slate-500 block">Variance</span>
                                <span
                                  className={`font-mono font-bold ${
                                    isBalanced
                                      ? "text-emerald-400"
                                      : isShort
                                      ? "text-rose-400"
                                      : "text-amber-400"
                                  }`}
                                >
                                  {rec.variance > 0
                                    ? `+₦${rec.variance.toLocaleString()}`
                                    : rec.variance < 0
                                    ? `-₦${Math.abs(rec.variance).toLocaleString()}`
                                    : "₦0"}
                                </span>
                              </div>
                            </div>

                            {rec.notes && (
                              <p className="text-[11px] text-slate-400 italic">
                                {rec.notes}
                              </p>
                            )}
                          </div>
                        );
                      })}
                    </div>

                    {/* Desktop Shift Table (>= sm) */}
                    <div className="hidden sm:block overflow-x-auto">
                      <table className="w-full text-left text-xs">
                        <thead>
                          <tr className="border-b border-slate-800 text-slate-400">
                            <th className="pb-3 font-medium">Shift Date</th>
                            <th className="pb-3 font-medium">Shift Label</th>
                            <th className="pb-3 font-medium">Duty Manager</th>
                            <th className="pb-3 font-medium text-right">Expected (Coin+RFID)</th>
                            <th className="pb-3 font-medium text-right">Actual Counted</th>
                            <th className="pb-3 font-medium text-right">Variance</th>
                            <th className="pb-3 font-medium text-center">Status</th>
                            <th className="pb-3 font-medium">Audit Remarks</th>
                          </tr>
                        </thead>
                        <tbody className="divide-y divide-slate-800/60 font-normal">
                          {(pagedRecs.length > 0 ? pagedRecs : reconciliations).map((rec) => {
                            const totalExpected =
                              (rec.expectedCashFromCoinBox || 0) +
                              (rec.expectedRevenueFromStaffRfid || 0);
                            const isBalanced = rec.variance === 0;
                            const isShort = rec.variance < 0;

                            return (
                              <tr key={rec._id} className="hover:bg-slate-800/40">
                                <td className="py-2.5 font-mono text-slate-300">
                                  {rec.shiftDate}
                                </td>
                                <td className="py-2.5 font-medium text-white">
                                  {rec.shiftLabel}
                                </td>
                                <td className="py-2.5 text-slate-300">
                                  {rec.managerName}
                                </td>
                                <td className="py-2.5 text-right font-mono text-slate-300">
                                  ₦{totalExpected.toLocaleString()}
                                </td>
                                <td className="py-2.5 text-right font-mono font-bold text-white">
                                  ₦{rec.actualCashCounted.toLocaleString()}
                                </td>
                                <td
                                  className={`py-2.5 text-right font-mono font-bold ${
                                    isBalanced
                                      ? "text-emerald-400"
                                      : isShort
                                      ? "text-rose-400"
                                      : "text-amber-400"
                                  }`}
                                >
                                  {rec.variance > 0
                                    ? `+₦${rec.variance.toLocaleString()}`
                                    : rec.variance < 0
                                    ? `-₦${Math.abs(rec.variance).toLocaleString()}`
                                    : "₦0 (Exact)"}
                                </td>
                                <td className="py-2.5 text-center">
                                  <span
                                    className={`inline-flex items-center gap-1 text-[10px] px-2 py-0.5 rounded-full font-semibold uppercase ${
                                      isBalanced
                                        ? "bg-emerald-500/15 text-emerald-400 border border-emerald-500/30"
                                        : isShort
                                        ? "bg-rose-500/15 text-rose-400 border border-rose-500/30"
                                        : "bg-amber-500/15 text-amber-400 border border-amber-500/30"
                                    }`}
                                  >
                                    {isBalanced ? (
                                      <CheckCircle2 className="w-2.5 h-2.5" />
                                    ) : (
                                      <AlertTriangle className="w-2.5 h-2.5" />
                                    )}
                                    {rec.status}
                                  </span>
                                </td>
                                <td className="py-2.5 text-slate-400 max-w-xs truncate">
                                  {rec.notes || "-"}
                                </td>
                              </tr>
                            );
                          })}
                        </tbody>
                      </table>
                    </div>
                    {recTotalPages > 1 && (
                      <div className="flex flex-col sm:flex-row items-center justify-between gap-3 pt-4 border-t border-slate-800 text-xs">
                        <span className="text-slate-400 text-[11px] font-mono">
                          Showing {(recPage - 1) * REC_PAGE_SIZE + 1}–{Math.min(recPage * REC_PAGE_SIZE, recTotal)} of {recTotal} shift audits
                        </span>
                        <div className="flex items-center gap-1.5">
                          <button
                            type="button"
                            onClick={() => setRecPage((p) => Math.max(p - 1, 1))}
                            disabled={recPage === 1}
                            className="px-2.5 py-1.5 rounded-lg border border-slate-800 bg-slate-800/60 hover:bg-slate-700/80 disabled:opacity-30 disabled:cursor-not-allowed text-slate-300 hover:text-white transition-all flex items-center gap-1"
                            aria-label="Previous shift audit page"
                          >
                            <ChevronLeft className="w-3.5 h-3.5" />
                            <span className="hidden sm:inline">Prev</span>
                          </button>
                          <span className="px-2.5 py-1 font-mono text-slate-300 text-[11px]">
                            Page {recPage} / {recTotalPages}
                          </span>
                          <button
                            type="button"
                            onClick={() => setRecPage((p) => Math.min(p + 1, recTotalPages))}
                            disabled={recPage >= recTotalPages}
                            className="px-2.5 py-1.5 rounded-lg border border-slate-800 bg-slate-800/60 hover:bg-slate-700/80 disabled:opacity-30 disabled:cursor-not-allowed text-slate-300 hover:text-white transition-all flex items-center gap-1"
                            aria-label="Next shift audit page"
                          >
                            <span className="hidden sm:inline">Next</span>
                            <ChevronRight className="w-3.5 h-3.5" />
                          </button>
                        </div>
                      </div>
                    )}
                  </>
                )}
              </div>

              {/* Complete Audit Log with Filters */}
              <div className="p-5 rounded-2xl bg-slate-900/80 border border-slate-800/90 space-y-4">
                <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
                  <h3 className="text-sm font-semibold text-white flex items-center gap-2">
                    <Activity className="w-4 h-4 text-emerald-400" />
                    Immutable Solenoid Event Ledger
                  </h3>
                  <a
                    href={`/api/reports/export?format=csv&days=7${
                      zone !== ALL_ZONES ? `&zone=${encodeURIComponent(zone)}` : ""
                    }`}
                    className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-emerald-400 text-xs font-medium border border-slate-700 transition-all self-start sm:self-auto"
                    download
                  >
                    <Download className="w-3.5 h-3.5" />
                    <span>Export {zone === ALL_ZONES ? "Audit CSV" : `${zone} CSV`}</span>
                  </a>
                </div>

                {/* Filter Toolbar */}
                <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-2.5 pt-2">
                  <div className="relative">
                    <Search className="w-3.5 h-3.5 text-slate-400 absolute left-3 top-1/2 -translate-y-1/2" />
                    <input
                      type="text"
                      placeholder="Search staff, UID, table..."
                      value={searchEventText}
                      onChange={(e) => setSearchEventText(e.target.value)}
                      className="w-full pl-9 pr-3 py-1.5 rounded-lg bg-slate-800/80 border border-slate-700 text-xs text-white placeholder-slate-500"
                    />
                  </div>

                  <select
                    value={filterTable}
                    onChange={(e) => setFilterTable(e.target.value)}
                    className="px-3 py-1.5 rounded-lg bg-slate-800/80 border border-slate-700 text-xs text-white"
                  >
                    <option value="all">All Tables</option>
                    {tables.map((t) => (
                      <option key={t.deviceId} value={t.deviceId}>
                        {t.name}
                      </option>
                    ))}
                  </select>

                  <select
                    value={filterTrigger}
                    onChange={(e) => setFilterTrigger(e.target.value)}
                    className="px-3 py-1.5 rounded-lg bg-slate-800/80 border border-slate-700 text-xs text-white"
                  >
                    <option value="all">All Trigger Sources</option>
                    <option value="rfid">Staff RFID</option>
                    <option value="button">Coin Box Button</option>
                    <option value="cloud">Cloud Remote</option>
                  </select>

                  <select
                    value={filterEvent}
                    onChange={(e) => setFilterEvent(e.target.value)}
                    className="px-3 py-1.5 rounded-lg bg-slate-800/80 border border-slate-700 text-xs text-white"
                  >
                    <option value="all">All Event Types</option>
                    <option value="billable">Rack Unlocked (Billable)</option>
                    <option value="alerts">Security Alerts / Rejects</option>
                    <option value="rfid_program">Card Programming</option>
                  </select>
                </div>

                {(() => {
                  const currentAuditPage = Math.min(auditPage, auditTotalPages);
                  const pagedAuditEvents = auditEvents;
                  const totalFilteredCount = auditTotal;

                  return (
                    <>
                      <div className="sm:hidden space-y-2.5 pt-2">
                        {auditLoading ? (
                          <div className="py-8 text-center text-slate-400 text-xs">Loading audit logs...</div>
                        ) : pagedAuditEvents.length === 0 ? (
                          <div className="py-8 text-center text-slate-500">
                            <Activity className="w-5 h-5 text-slate-600 mx-auto mb-1.5" />
                            <p className="text-xs">No audit events match current criteria</p>
                          </div>
                        ) : (
                          pagedAuditEvents.map((ev) => (
                            <div
                              key={ev._id}
                              className="p-3 rounded-xl bg-slate-800/40 border border-slate-800 space-y-1.5"
                            >
                              <div className="flex items-center justify-between text-xs">
                                <span className="font-semibold text-white">{ev.tableName || ev.deviceId}</span>
                                <span className="font-mono text-emerald-400 font-bold">
                                  {ev.priceCharged > 0 ? `₦${ev.priceCharged.toLocaleString()}` : "-"}
                                </span>
                              </div>
                              <div className="flex items-center justify-between text-[11px] text-slate-400">
                                <span>{ev.staffName || "Attendant"} {ev.cardUid ? `(${ev.cardUid})` : ""}</span>
                                <span className="font-mono text-[10px] text-slate-500 uppercase">{ev.trigger}</span>
                              </div>
                              <div className="flex items-center justify-between text-[10px] text-slate-500 pt-1 border-t border-slate-800">
                                <span className="font-mono">{ev.event}</span>
                                <span className="font-mono">{new Date(ev.createdAt).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" })}</span>
                              </div>
                            </div>
                          ))
                        )}
                      </div>

                      <div className="hidden sm:block overflow-x-auto pt-2">
                        <table className="w-full text-left text-xs">
                          <thead>
                            <tr className="border-b border-slate-800 text-slate-400">
                              <th className="pb-3 font-medium">Timestamp</th>
                              <th className="pb-3 font-medium">Table Device</th>
                              <th className="pb-3 font-medium">Action Event</th>
                              <th className="pb-3 font-medium">Trigger Source</th>
                              <th className="pb-3 font-medium">Staff / Card ID</th>
                              <th className="pb-3 font-medium text-right">Amount</th>
                            </tr>
                          </thead>
                          <tbody className="divide-y divide-slate-800/60 font-normal">
                            {auditLoading ? (
                              <tr>
                                <td colSpan={6} className="py-8 text-center text-slate-400 text-xs">
                                  Loading audit logs...
                                </td>
                              </tr>
                            ) : pagedAuditEvents.length === 0 ? (
                              <tr>
                                <td colSpan={6} className="py-8 text-center text-slate-500">
                                  <Activity className="w-5 h-5 text-slate-600 mx-auto mb-1.5" />
                                  <p className="text-xs">No audit events match current criteria</p>
                                </td>
                              </tr>
                            ) : (
                              pagedAuditEvents.map((ev) => (
                                <tr key={ev._id} className="hover:bg-slate-800/40">
                                  <td className="py-2.5 font-mono text-slate-400 text-[11px]">
                                    {new Date(ev.createdAt).toLocaleString()}
                                  </td>
                                  <td className="py-2.5 text-slate-200">
                                    {ev.tableName || ev.deviceId}
                                  </td>
                                  <td className="py-2.5 font-mono text-[11px]">
                                    <span>{ev.event}</span>
                                    {ev.syncedFromOffline && (
                                      <span className="ml-1.5 inline-flex items-center px-1.5 py-0.5 rounded-full bg-amber-500/10 text-amber-400 border border-amber-500/30 font-sans text-[10px]">
                                        Offline
                                      </span>
                                    )}
                                  </td>
                                  <td className="py-2.5 uppercase font-mono text-[10px] text-slate-400">
                                    {ev.trigger}
                                  </td>
                                  <td className="py-2.5 text-slate-300">
                                    {ev.staffName} {ev.cardUid ? `(${ev.cardUid})` : ""}
                                  </td>
                                  <td className="py-2.5 text-right font-mono text-emerald-400">
                                    {ev.priceCharged > 0 ? `₦${ev.priceCharged}` : "-"}
                                  </td>
                                </tr>
                              ))
                            )}
                          </tbody>
                        </table>
                      </div>

                      {auditTotalPages > 1 && (
                        <div className="flex flex-col sm:flex-row items-center justify-between gap-3 pt-4 border-t border-slate-800 text-xs">
                          <span className="text-slate-400 text-[11px] font-mono">
                            Showing {(currentAuditPage - 1) * AUDIT_PAGE_SIZE + 1}–{Math.min(currentAuditPage * AUDIT_PAGE_SIZE, totalFilteredCount)} of {totalFilteredCount} audit entries
                          </span>
                          <div className="flex items-center gap-1.5">
                            <button
                              type="button"
                              onClick={() => setAuditPage((p) => Math.max(p - 1, 1))}
                              disabled={currentAuditPage === 1}
                              className="px-2.5 py-1.5 rounded-lg border border-slate-800 bg-slate-800/60 hover:bg-slate-700/80 disabled:opacity-30 disabled:cursor-not-allowed text-slate-300 hover:text-white transition-all flex items-center gap-1"
                              aria-label="Previous page"
                            >
                              <ChevronLeft className="w-3.5 h-3.5" />
                              <span className="hidden sm:inline">Prev</span>
                            </button>

                            <span className="px-3 py-1 rounded-lg bg-slate-950/60 border border-slate-800 text-slate-300 font-mono text-[11px]">
                              {currentAuditPage} / {auditTotalPages}
                            </span>

                            <button
                              type="button"
                              onClick={() => setAuditPage((p) => Math.min(p + 1, auditTotalPages))}
                              disabled={currentAuditPage === auditTotalPages}
                              className="px-2.5 py-1.5 rounded-lg border border-slate-800 bg-slate-800/60 hover:bg-slate-700/80 disabled:opacity-30 disabled:cursor-not-allowed text-slate-300 hover:text-white transition-all flex items-center gap-1"
                              aria-label="Next page"
                            >
                              <span className="hidden sm:inline">Next</span>
                              <ChevronRight className="w-3.5 h-3.5" />
                            </button>
                          </div>
                        </div>
                      )}
                    </>
                  );
                })()}
              </div>
            </div>
          )}

          {/* TAB 5: PROFILE & ZONES SETTINGS */}
          {activeTab === "settings" && (
            <SettingsPanel
              zones={zones}
              selectedZone={zone}
              onZonesChanged={handleZonesChanged}
              onViewZone={handleViewZoneReport}
              notify={showNotification}
            />
          )}
        </main>
      </div>

      {/* MODAL 1: ARM CARD WRITER */}
      {armModalOpen && (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/75 backdrop-blur-sm">
          <div className="bg-slate-900 border border-slate-800 rounded-2xl max-w-md w-full p-6 shadow-2xl space-y-4">
            <div className="flex items-center justify-between border-b border-slate-800 pb-3">
              <div className="flex items-center gap-2">
                <Radio className="w-5 h-5 text-emerald-400 animate-pulse" />
                <h3 className="text-sm font-bold text-white">
                  Program RFID Card Over-The-Air
                </h3>
              </div>
              <button
                onClick={() => setArmModalOpen(false)}
                className="p-1 rounded-lg text-slate-400 hover:text-white"
              >
                <X className="w-4 h-4" />
              </button>
            </div>

            <p className="text-xs text-slate-400">
              Select an online table node. Once armed, hold any blank MIFARE 1K card or keyfob against the table&apos;s RFID Pod. The device will encode the staff credentials and upload the new UID to MongoDB.
            </p>

            <form onSubmit={handleArmWriter} className="space-y-3">
              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">
                  Target Table Node
                </label>
                <select
                  value={armTargetDevice}
                  onChange={(e) => setArmTargetDevice(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                >
                  {tables.length === 0 ? (
                    <option value="">No tables configured - Pair or Add a Table first</option>
                  ) : (
                    tables.map((t) => (
                      <option key={t.deviceId} value={t.deviceId}>
                        {t.name} ({t.deviceId})
                      </option>
                    ))
                  )}
                </select>
              </div>

              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">
                  Staff Member Name
                </label>
                <input
                  type="text"
                  required
                  placeholder="e.g. Ibrahim Musa"
                  value={armStaffName}
                  onChange={(e) => setArmStaffName(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                />
              </div>

              <div className="grid grid-cols-2 gap-3">
                <div>
                  <label className="block text-xs font-medium text-slate-300 mb-1">
                    Staff ID
                  </label>
                  <input
                    type="text"
                    required
                    placeholder="EMP-012"
                    value={armStaffId}
                    onChange={(e) => setArmStaffId(e.target.value)}
                    className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white font-mono uppercase"
                  />
                </div>
                <div>
                  <label className="block text-xs font-medium text-slate-300 mb-1">
                    Role
                  </label>
                  <select
                    value={armRole}
                    onChange={(e) => setArmRole(e.target.value)}
                    className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                  >
                    <option value="staff">Staff</option>
                    <option value="manager">Manager</option>
                    <option value="admin">Admin</option>
                    <option value="vip_member">VIP Member</option>
                  </select>
                </div>
              </div>

              {armStatusMsg && (
                <div className="p-3 rounded-lg bg-emerald-950/40 border border-emerald-800 text-emerald-300 text-xs font-mono">
                  {armStatusMsg}
                </div>
              )}

              <div className="pt-2 flex items-center justify-end gap-2">
                <button
                  type="button"
                  onClick={() => setArmModalOpen(false)}
                  className="px-4 py-2 rounded-lg bg-slate-800 text-slate-300 hover:text-white text-xs font-medium"
                >
                  Cancel
                </button>
                <button
                  type="submit"
                  className="px-4 py-2 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold shadow-md"
                >
                  Arm RFID Reader (60s)
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* MODAL 2: ADD MANUAL CARD */}
      {cardModalOpen && (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/75 backdrop-blur-sm">
          <div className="bg-slate-900 border border-slate-800 rounded-2xl max-w-md w-full p-6 shadow-2xl space-y-4">
            <div className="flex items-center justify-between border-b border-slate-800 pb-3">
              <h3 className="text-sm font-bold text-white flex items-center gap-2">
                <Plus className="w-4 h-4 text-emerald-400" />
                Add RFID Card Manually
              </h3>
              <button
                onClick={() => setCardModalOpen(false)}
                className="p-1 rounded-lg text-slate-400 hover:text-white"
              >
                <X className="w-4 h-4" />
              </button>
            </div>

            <form onSubmit={handleAddManualCard} className="space-y-3">
              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">
                  Card UID (Hex Format)
                </label>
                <input
                  type="text"
                  required
                  placeholder="e.g. 04:A2:3B:5C"
                  value={newCardUid}
                  onChange={(e) => setNewCardUid(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white font-mono uppercase"
                />
              </div>

              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">
                  Staff Member Name
                </label>
                <input
                  type="text"
                  required
                  placeholder="e.g. Samuel Ade"
                  value={newCardStaffName}
                  onChange={(e) => setNewCardStaffName(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                />
              </div>

              <div className="grid grid-cols-2 gap-3">
                <div>
                  <label className="block text-xs font-medium text-slate-300 mb-1">
                    Staff ID
                  </label>
                  <input
                    type="text"
                    required
                    placeholder="EMP-004"
                    value={newCardStaffId}
                    onChange={(e) => setNewCardStaffId(e.target.value)}
                    className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white font-mono uppercase"
                  />
                </div>
                <div>
                  <label className="block text-xs font-medium text-slate-300 mb-1">
                    Daily Limit (Racks)
                  </label>
                  <input
                    type="number"
                    value={newCardLimit}
                    onChange={(e) => setNewCardLimit(Number(e.target.value))}
                    className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white font-mono"
                  />
                </div>
              </div>

              <div className="pt-2 flex items-center justify-end gap-2">
                <button
                  type="button"
                  onClick={() => setCardModalOpen(false)}
                  className="px-4 py-2 rounded-lg bg-slate-800 text-slate-300 hover:text-white text-xs font-medium"
                >
                  Cancel
                </button>
                <button
                  type="submit"
                  className="px-4 py-2 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold shadow-md"
                >
                  Save Card to Whitelist
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* MODAL 3: ADD/EDIT TABLE */}
      {tableModalOpen && (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/75 backdrop-blur-sm">
          <div className="bg-slate-900 border border-slate-800 rounded-2xl max-w-md w-full p-6 shadow-2xl space-y-4">
            <div className="flex items-center justify-between border-b border-slate-800 pb-3">
              <h3 className="text-sm font-bold text-white flex items-center gap-2">
                <Server className="w-4 h-4 text-emerald-400" />
                Table Node Configuration
              </h3>
              <button
                onClick={() => setTableModalOpen(false)}
                className="p-1 rounded-lg text-slate-400 hover:text-white"
              >
                <X className="w-4 h-4" />
              </button>
            </div>

            <form onSubmit={handleSaveTable} className="space-y-3">
              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">
                  Device ID (Must match hardware device identifier)
                </label>
                <input
                  type="text"
                  required
                  placeholder="e.g. snooker_table_01"
                  value={tableFormId}
                  onChange={(e) => setTableFormId(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white font-mono"
                />
              </div>

              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">
                  Friendly Table Name
                </label>
                <input
                  type="text"
                  required
                  placeholder="Table 01 - Championship Pro"
                  value={tableFormName}
                  onChange={(e) => setTableFormName(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                />
              </div>

              <div className="grid grid-cols-2 gap-3">
                <div>
                  <label className="block text-xs font-medium text-slate-300 mb-1">
                    Venue / Zone
                  </label>
                  {zones.length > 0 ? (
                    <select
                      value={tableFormLocation || defaultTableZone}
                      onChange={(e) => setTableFormLocation(e.target.value)}
                      className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                    >
                      {zones.map((z) => (
                        <option key={z.name} value={z.name}>
                          {z.name}
                        </option>
                      ))}
                    </select>
                  ) : (
                    <input
                      type="text"
                      placeholder="Main Floor"
                      value={tableFormLocation}
                      onChange={(e) => setTableFormLocation(e.target.value)}
                      className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                    />
                  )}
                </div>
                <div>
                  <label className="block text-xs font-medium text-slate-300 mb-1">
                    Price Per Rack (₦)
                  </label>
                  <input
                    type="number"
                    value={tableFormPrice}
                    onChange={(e) => setTableFormPrice(Number(e.target.value))}
                    className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white font-mono"
                  />
                </div>
              </div>

              <div className="pt-2 flex items-center justify-between gap-2">
                {tableFormId ? (
                  <button
                    type="button"
                    onClick={() => {
                      setTableModalOpen(false);
                      handleDeleteTable(tableFormId, tableFormName);
                    }}
                    className="px-3 py-2 rounded-lg bg-rose-500/10 hover:bg-rose-500/20 text-rose-400 hover:text-rose-300 text-xs font-medium border border-rose-500/30 flex items-center gap-1.5 transition-colors"
                  >
                    <Trash2 className="w-3.5 h-3.5" />
                    <span>Delete Table</span>
                  </button>
                ) : (
                  <div />
                )}
                <div className="flex items-center gap-2">
                  <button
                    type="button"
                    onClick={() => setTableModalOpen(false)}
                    className="px-4 py-2 rounded-lg bg-slate-800 text-slate-300 hover:text-white text-xs font-medium"
                  >
                    Cancel
                  </button>
                  <button
                    type="submit"
                    className="px-4 py-2 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold shadow-md"
                  >
                    Save Table Node
                  </button>
                </div>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* MODAL 4: PAIR / CLAIM HARDWARE NODE */}
      {claimModalOpen && (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/75 backdrop-blur-sm animate-in fade-in">
          <div className="bg-slate-900 border border-slate-800 rounded-2xl max-w-md w-full p-6 shadow-2xl space-y-4">
            <div className="flex items-center justify-between border-b border-slate-800 pb-3">
              <div className="flex items-center gap-2">
                <KeyRound className="w-5 h-5 text-emerald-400" />
                <h3 className="text-sm font-bold text-white">
                  Pair Hardware Snooker Node
                </h3>
              </div>
              <button
                onClick={() => setClaimModalOpen(false)}
                className="p-1 rounded-lg text-slate-400 hover:text-white"
              >
                <X className="w-4 h-4" />
              </button>
            </div>

            <p className="text-xs text-slate-400">
              Enter the unique Serial Number and 6-digit Claim PIN printed on the bottom of the device controller enclosure to link it to your venue account.
            </p>

            {claimMsg && (
              <div
                className={`p-3 rounded-xl border text-xs font-medium flex items-center gap-2 ${
                  claimMsg.type === "success"
                    ? "bg-emerald-950/60 border-emerald-700 text-emerald-300"
                    : "bg-rose-950/60 border-rose-700 text-rose-300"
                }`}
              >
                {claimMsg.type === "success" ? (
                  <CheckCircle2 className="w-4 h-4 flex-shrink-0 text-emerald-400" />
                ) : (
                  <AlertTriangle className="w-4 h-4 flex-shrink-0 text-rose-400" />
                )}
                <span>{claimMsg.text}</span>
              </div>
            )}

            <form onSubmit={handleClaimDevice} className="space-y-3">
              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">
                  Hardware Serial Number
                </label>
                <input
                  type="text"
                  required
                  placeholder="e.g. ADR-SNK-8266-01"
                  value={claimSerial}
                  onChange={(e) => setClaimSerial(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white font-mono uppercase tracking-wider"
                />
              </div>

              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">
                  6-Digit Claim PIN
                </label>
                <input
                  type="password"
                  required
                  maxLength={8}
                  placeholder="6-digit PIN from enclosure label"
                  value={claimPin}
                  onChange={(e) => setClaimPin(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white font-mono tracking-widest"
                />
              </div>

              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">
                  Table Display Name
                </label>
                <input
                  type="text"
                  required
                  placeholder="e.g. Table 03 - Championship Gold"
                  value={claimTableName}
                  onChange={(e) => setClaimTableName(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                />
              </div>

              <div className="grid grid-cols-2 gap-3">
                <div>
                  <label className="block text-xs font-medium text-slate-300 mb-1">
                    Venue Area / Zone
                  </label>
                  {zones.length > 0 ? (
                    <select
                      value={claimVenue || defaultTableZone}
                      onChange={(e) => setClaimVenue(e.target.value)}
                      className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                    >
                      {zones.map((z) => (
                        <option key={z.name} value={z.name}>
                          {z.name}
                        </option>
                      ))}
                    </select>
                  ) : (
                    <input
                      type="text"
                      placeholder="Main Floor"
                      value={claimVenue}
                      onChange={(e) => setClaimVenue(e.target.value)}
                      className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                    />
                  )}
                </div>
                <div>
                  <label className="block text-xs font-medium text-slate-300 mb-1">
                    Price Per Rack (₦)
                  </label>
                  <input
                    type="number"
                    value={claimPrice}
                    onChange={(e) => setClaimPrice(Number(e.target.value))}
                    className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white font-mono"
                  />
                </div>
              </div>

              <div className="pt-2 flex items-center justify-end gap-2">
                <button
                  type="button"
                  onClick={() => setClaimModalOpen(false)}
                  className="px-4 py-2 rounded-lg bg-slate-800 text-slate-300 hover:text-white text-xs font-medium"
                >
                  Cancel
                </button>
                <button
                  type="submit"
                  disabled={claimBusy}
                  className="flex items-center gap-1.5 px-4 py-2 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold shadow-md transition-all disabled:opacity-50"
                >
                  <KeyRound className="w-3.5 h-3.5" />
                  <span>{claimBusy ? "Verifying..." : "Verify & Pair Node"}</span>
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* MODAL 5: RECORD SHIFT RECONCILIATION */}
      {recModalOpen && (
        <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/75 backdrop-blur-sm animate-in fade-in">
          <div className="bg-slate-900 border border-slate-800 rounded-2xl max-w-md w-full p-6 shadow-2xl space-y-4">
            <div className="flex items-center justify-between border-b border-slate-800 pb-3">
              <div className="flex items-center gap-2">
                <Receipt className="w-5 h-5 text-emerald-400" />
                <h3 className="text-sm font-bold text-white">
                  Record Shift Cash Count & Audit
                </h3>
              </div>
              <button
                onClick={() => setRecModalOpen(false)}
                className="p-1 rounded-lg text-slate-400 hover:text-white"
              >
                <X className="w-4 h-4" />
              </button>
            </div>

            {/* Shift Expected Summary Preview */}
            <div className="p-3.5 rounded-xl bg-slate-800/60 border border-slate-700/60 text-xs space-y-1.5">
              <div className="flex justify-between text-slate-400">
                <span>Expected Push Button Coin Cash:</span>
                <span className="font-mono text-white">
                  ₦{summary ? summary.todayCoinBoxRevenue.toLocaleString() : 0}
                </span>
              </div>
              <div className="flex justify-between text-slate-400">
                <span>Expected Staff RFID Collected Cash:</span>
                <span className="font-mono text-emerald-400 font-semibold">
                  ₦{summary ? summary.todayRfidRevenue.toLocaleString() : 0}
                </span>
              </div>
              <div className="flex justify-between pt-1.5 border-t border-slate-700 font-medium text-slate-200">
                <span>Total Expected Venue Shift Revenue:</span>
                <span className="font-mono text-emerald-400 font-bold">
                  ₦
                  {summary
                    ? (
                        summary.todayCoinBoxRevenue + summary.todayRfidRevenue
                      ).toLocaleString()
                    : 0}
                </span>
              </div>
            </div>

            {/* Real-time Variance Preview */}
            {recActualCash !== "" && (
              <div
                className={`p-3 rounded-xl border text-xs font-medium flex items-center justify-between ${
                  Number(recActualCash) -
                    ((summary?.todayCoinBoxRevenue || 0) +
                      (summary?.todayRfidRevenue || 0)) ===
                  0
                    ? "bg-emerald-950/60 border-emerald-600 text-emerald-300"
                    : Number(recActualCash) -
                        ((summary?.todayCoinBoxRevenue || 0) +
                          (summary?.todayRfidRevenue || 0)) <
                      0
                    ? "bg-rose-950/60 border-rose-600 text-rose-300"
                    : "bg-amber-950/60 border-amber-600 text-amber-300"
                }`}
              >
                <span>Calculated Variance:</span>
                <span className="font-mono font-bold text-sm">
                  {Number(recActualCash) -
                    ((summary?.todayCoinBoxRevenue || 0) +
                      (summary?.todayRfidRevenue || 0)) >
                  0
                    ? `+₦${(
                        Number(recActualCash) -
                        ((summary?.todayCoinBoxRevenue || 0) +
                          (summary?.todayRfidRevenue || 0))
                      ).toLocaleString()} (Overage)`
                    : Number(recActualCash) -
                        ((summary?.todayCoinBoxRevenue || 0) +
                          (summary?.todayRfidRevenue || 0)) <
                      0
                    ? `-₦${Math.abs(
                        Number(recActualCash) -
                          ((summary?.todayCoinBoxRevenue || 0) +
                            (summary?.todayRfidRevenue || 0))
                      ).toLocaleString()} (Shortage)`
                    : "₦0 (Balanced Exact Match)"}
                </span>
              </div>
            )}

            {recMsg && (
              <div
                className={`p-3 rounded-xl border text-xs font-medium flex items-center gap-2 ${
                  recMsg.type === "success"
                    ? "bg-emerald-950/60 border-emerald-700 text-emerald-300"
                    : "bg-rose-950/60 border-rose-700 text-rose-300"
                }`}
              >
                {recMsg.type === "success" ? (
                  <CheckCircle2 className="w-4 h-4 flex-shrink-0 text-emerald-400" />
                ) : (
                  <AlertTriangle className="w-4 h-4 flex-shrink-0 text-rose-400" />
                )}
                <span>{recMsg.text}</span>
              </div>
            )}

            <form onSubmit={handleRecordReconciliation} className="space-y-3">
              <div className="grid grid-cols-2 gap-3">
                <div>
                  <label className="block text-xs font-medium text-slate-300 mb-1">
                    Shift Label
                  </label>
                  <select
                    value={recShiftLabel}
                    onChange={(e) => setRecShiftLabel(e.target.value)}
                    className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                  >
                    <option value="Morning Shift">Morning Shift</option>
                    <option value="Afternoon Shift">Afternoon Shift</option>
                    <option value="Evening Shift">Evening Shift</option>
                    <option value="Night Shift">Night Shift</option>
                    <option value="Daily Closeout">Full Day Closeout</option>
                  </select>
                </div>
                <div>
                  <label className="block text-xs font-medium text-slate-300 mb-1">
                    Manager Name
                  </label>
                  <input
                    type="text"
                    required
                    placeholder="Duty Attendant"
                    value={recManagerName}
                    onChange={(e) => setRecManagerName(e.target.value)}
                    className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                  />
                </div>
              </div>

              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">
                  Actual Physical Cash Counted (₦)
                </label>
                <input
                  type="number"
                  required
                  placeholder="Total paper notes counted from coin box & staff float"
                  value={recActualCash}
                  onChange={(e) =>
                    setRecActualCash(
                      e.target.value === "" ? "" : Number(e.target.value)
                    )
                  }
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-sm text-white font-mono font-bold"
                />
              </div>

              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">
                  Audit Notes / Discrepancy Reason
                </label>
                <textarea
                  rows={2}
                  placeholder="Optional explanations for variances, damaged coins, tournament comps, etc."
                  value={recNotes}
                  onChange={(e) => setRecNotes(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                />
              </div>

              <div className="pt-2 flex items-center justify-end gap-2">
                <button
                  type="button"
                  onClick={() => setRecModalOpen(false)}
                  className="px-4 py-2 rounded-lg bg-slate-800 text-slate-300 hover:text-white text-xs font-medium"
                >
                  Cancel
                </button>
                <button
                  type="submit"
                  disabled={recBusy}
                  className="flex items-center gap-1.5 px-4 py-2 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold shadow-md transition-all disabled:opacity-50"
                >
                  <Receipt className="w-3.5 h-3.5" />
                  <span>
                    {recBusy ? "Logging Audit..." : "Sign & Register Shift Cash"}
                  </span>
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* Firmware Remote Cloud OTA Modal */}
      {otaModalOpen && (
        <div className="fixed inset-0 z-50 bg-black/75 backdrop-blur-sm flex items-center justify-center p-4">
          <div className="w-full max-w-lg rounded-2xl bg-slate-900 border border-slate-800 p-6 shadow-2xl relative">
            <button
              onClick={() => {
                setOtaModalOpen(false);
                setOtaMsg(null);
              }}
              className="absolute top-4 right-4 p-1.5 rounded-lg text-slate-400 hover:text-white hover:bg-slate-800"
            >
              <X className="w-4 h-4" />
            </button>

            <div className="flex items-center gap-3 mb-4">
              <div className="w-10 h-10 rounded-xl bg-blue-500/15 border border-blue-500/30 flex items-center justify-center">
                <UploadCloud className="w-5 h-5 text-blue-400" />
              </div>
              <div>
                <h3 className="text-base font-bold text-white">Remote Cloud Firmware OTA</h3>
                <p className="text-xs text-slate-400">Deploy compiled ESP32 binaries over Wi-Fi without USB cables</p>
              </div>
            </div>

            {otaMsg && (
              <div
                className={`mb-4 p-3 rounded-lg text-xs flex items-center gap-2 ${
                  otaMsg.type === "success"
                    ? "bg-emerald-950/70 border border-emerald-500/40 text-emerald-300"
                    : "bg-rose-950/70 border border-rose-500/40 text-rose-300"
                }`}
              >
                {otaMsg.type === "success" ? (
                  <CheckCircle2 className="w-4 h-4 shrink-0 text-emerald-400" />
                ) : (
                  <AlertTriangle className="w-4 h-4 shrink-0 text-rose-400" />
                )}
                <span>{otaMsg.text}</span>
              </div>
            )}

            <form onSubmit={handleTriggerOta} className="space-y-4">
              <div className="grid grid-cols-1 sm:grid-cols-2 gap-3">
                <div>
                  <label className="block text-xs font-medium text-slate-300 mb-1">
                    Target Table Device
                  </label>
                  <select
                    value={otaTargetDevice}
                    onChange={(e) => setOtaTargetDevice(e.target.value)}
                    className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white"
                  >
                    <option value="*">All Online Tables (Fleet Broadcast)</option>
                    {tables.map((tbl) => (
                      <option key={tbl.deviceId} value={tbl.deviceId}>
                        {tbl.name} ({tbl.deviceId}) - {tbl.firmwareVersion || "v2.0.0-esp32"}
                      </option>
                    ))}
                  </select>
                </div>

                <div>
                  <label className="block text-xs font-medium text-slate-300 mb-1">
                    Target Version Tag
                  </label>
                  <input
                    type="text"
                    required
                    placeholder="e.g. v2.1.0-esp32"
                    value={otaTargetVersion}
                    onChange={(e) => setOtaTargetVersion(e.target.value)}
                    className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white font-mono"
                  />
                </div>
              </div>

              {/* Upload Binary File */}
              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">
                  Firmware Binary (.bin)
                </label>
                <div className="p-3 rounded-lg bg-slate-800/60 border border-dashed border-slate-700 flex flex-col items-center justify-center text-center">
                  <Upload className="w-5 h-5 text-slate-400 mb-1" />
                  <p className="text-xs text-slate-300 font-medium">
                    {otaFileSelected ? otaFileSelected : "Choose or drag ESP32 compiled binary"}
                  </p>
                  <p className="text-[11px] text-slate-500 mt-0.5">
                    Exported from Arduino IDE: Sketch → Export Compiled Binary (.bin)
                  </p>
                  <input
                    type="file"
                    accept=".bin"
                    onChange={(e) => {
                      const file = e.target.files?.[0];
                      if (file) handleUploadFirmware(file);
                    }}
                    className="mt-2 text-xs text-slate-400 file:mr-2 file:py-1 file:px-2.5 file:rounded-md file:border-0 file:text-xs file:font-semibold file:bg-blue-600 file:text-white hover:file:bg-blue-500 file:cursor-pointer"
                  />
                </div>
              </div>

              {/* Or Direct URL */}
              <div>
                <label className="block text-xs font-medium text-slate-300 mb-1">
                  Firmware Binary URL
                </label>
                <input
                  type="url"
                  required
                  placeholder="https://example.com/firmware/v2.1.0-esp32.bin"
                  value={otaFirmwareUrl}
                  onChange={(e) => setOtaFirmwareUrl(e.target.value)}
                  className="w-full px-3 py-2 rounded-lg bg-slate-800 border border-slate-700 text-xs text-white font-mono"
                />
                <p className="text-[11px] text-slate-500 mt-1">
                  ESP32 pulls this URL on its next Wi-Fi heartbeat and verifies the dual-partition A/B flash before swapping.
                </p>
              </div>

              <div className="p-3 rounded-lg bg-blue-950/40 border border-blue-900/50 flex items-start gap-2 text-xs text-blue-300">
                <Cpu className="w-4 h-4 shrink-0 text-blue-400 mt-0.5" />
                <div className="space-y-0.5 text-[11px] text-slate-300">
                  <p className="font-semibold text-blue-300">Failsafe Dual-Partition Flash Protection:</p>
                  <p>ESP32 writes the image to its inactive partition (<span className="font-mono text-white">ota_1</span>). If network drops mid-stream, the existing firmware (<span className="font-mono text-white">ota_0</span>) stays untouched. Servo lock is blocked during active flashes.</p>
                </div>
              </div>

              <div className="pt-2 flex items-center justify-end gap-2">
                <button
                  type="button"
                  onClick={() => {
                    setOtaModalOpen(false);
                    setOtaMsg(null);
                  }}
                  className="px-4 py-2 rounded-lg bg-slate-800 text-slate-300 hover:text-white text-xs font-medium"
                >
                  Cancel
                </button>
                <button
                  type="submit"
                  disabled={otaUploading || !otaFirmwareUrl.trim()}
                  className="flex items-center gap-1.5 px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-500 text-white text-xs font-semibold shadow-md transition-all disabled:opacity-50"
                >
                  <UploadCloud className={`w-3.5 h-3.5 ${otaUploading ? "animate-spin" : ""}`} />
                  <span>{otaUploading ? "Deploying..." : "Push Remote OTA Update"}</span>
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* Persistent Mobile Bottom Navigation Bar (< lg) */}
      <nav className="lg:hidden fixed bottom-0 left-0 right-0 z-40 bg-slate-900/95 backdrop-blur-xl border-t border-slate-800 px-2 py-1.5 flex items-center justify-around shadow-2xl">
        <button
          onClick={() => changeTab("overview")}
          className={`flex flex-col items-center gap-1 py-1.5 px-3 rounded-xl transition-all ${
            activeTab === "overview"
              ? "text-emerald-400 font-bold"
              : "text-slate-400 hover:text-slate-200"
          }`}
        >
          <LayoutDashboard className="w-5 h-5" />
          <span className="text-[10px]">Overview</span>
        </button>

        <button
          onClick={() => changeTab("tables")}
          className={`flex flex-col items-center gap-1 py-1.5 px-3 rounded-xl relative transition-all ${
            activeTab === "tables"
              ? "text-emerald-400 font-bold"
              : "text-slate-400 hover:text-slate-200"
          }`}
        >
          <Server className="w-5 h-5" />
          <span className="text-[10px]">Tables</span>
          {tables.length > 0 && (
            <span className="absolute top-1 right-2 px-1 min-w-[14px] h-3.5 rounded-full bg-slate-800 text-[9px] font-mono text-emerald-400 flex items-center justify-center border border-slate-700">
              {tables.length}
            </span>
          )}
        </button>

        <button
          onClick={() => changeTab("cards")}
          className={`flex flex-col items-center gap-1 py-1.5 px-3 rounded-xl relative transition-all ${
            activeTab === "cards"
              ? "text-emerald-400 font-bold"
              : "text-slate-400 hover:text-slate-200"
          }`}
        >
          <CreditCard className="w-5 h-5" />
          <span className="text-[10px]">Cards</span>
          {cards.length > 0 && (
            <span className="absolute top-1 right-2 px-1 min-w-[14px] h-3.5 rounded-full bg-slate-800 text-[9px] font-mono text-emerald-400 flex items-center justify-center border border-slate-700">
              {cards.length}
            </span>
          )}
        </button>

        <button
          onClick={() => changeTab("financials")}
          className={`flex flex-col items-center gap-1 py-1.5 px-3 rounded-xl relative transition-all ${
            activeTab === "financials"
              ? "text-emerald-400 font-bold"
              : "text-slate-400 hover:text-slate-200"
          }`}
        >
          <Receipt className="w-5 h-5" />
          <span className="text-[10px]">Reconcile</span>
          {summary && summary.securityAlertsToday > 0 && (
            <span className="absolute top-1 right-2 px-1 min-w-[14px] h-3.5 rounded-full bg-rose-500 text-[9px] font-mono text-white flex items-center justify-center">
              {summary.securityAlertsToday}
            </span>
          )}
        </button>

        <button
          onClick={() => changeTab("settings")}
          className={`flex flex-col items-center gap-1 py-1.5 px-3 rounded-xl transition-all ${
            activeTab === "settings"
              ? "text-emerald-400 font-bold"
              : "text-slate-400 hover:text-slate-200"
          }`}
        >
          <Settings className="w-5 h-5" />
          <span className="text-[10px]">Settings</span>
        </button>

        {/* Mobile Admin Console Switcher */}
        {isAdmin && (
          <Link
            href="/admin"
            className="flex flex-col items-center gap-1 py-1.5 px-3 rounded-xl text-blue-400 hover:text-blue-300 transition-all"
            title="Switch to Admin Console"
          >
            <Shield className="w-5 h-5" />
            <span className="text-[10px] font-bold">Admin</span>
          </Link>
        )}
      </nav>
    </div>
  );
}

