"use client";

import React, { useEffect, useRef, useState } from "react";
import { Building, Check, ChevronDown, Layers, Settings2 } from "lucide-react";

export interface ZoneSummary {
  name: string;
  tableCount: number;
  onlineCount: number;
  todayRacks: number;
  todayRevenue: number;
}

export const ALL_ZONES = "all";

interface ZoneSwitcherProps {
  zones: ZoneSummary[];
  selected: string;
  onSelect: (zone: string) => void;
  onManage: () => void;
  busy?: boolean;
  className?: string;
}

const formatNaira = (n: number) => `₦${n.toLocaleString()}`;

/** Dropdown to switch the dashboard between zones (or all zones). */
export function ZoneSwitcher({ zones, selected, onSelect, onManage, busy, className = "" }: ZoneSwitcherProps) {
  const [open, setOpen] = useState(false);
  const ref = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!open) return;
    const onClick = (e: MouseEvent) => {
      if (ref.current && !ref.current.contains(e.target as Node)) setOpen(false);
    };
    const onKey = (e: KeyboardEvent) => e.key === "Escape" && setOpen(false);
    document.addEventListener("mousedown", onClick);
    document.addEventListener("keydown", onKey);
    return () => {
      document.removeEventListener("mousedown", onClick);
      document.removeEventListener("keydown", onKey);
    };
  }, [open]);

  const totals = zones.reduce(
    (acc, z) => ({
      tableCount: acc.tableCount + z.tableCount,
      onlineCount: acc.onlineCount + z.onlineCount,
      todayRevenue: acc.todayRevenue + z.todayRevenue,
    }),
    { tableCount: 0, onlineCount: 0, todayRevenue: 0 }
  );

  const isAll = selected === ALL_ZONES;
  const label = isAll ? "All Zones" : selected;

  const choose = (zone: string) => {
    setOpen(false);
    if (zone !== selected) onSelect(zone);
  };

  const Option = ({
    value,
    title,
    tables,
    online,
    revenue,
    icon,
  }: {
    value: string;
    title: string;
    tables: number;
    online: number;
    revenue: number;
    icon: React.ReactNode;
  }) => {
    const active = value === selected;
    return (
      <button
        type="button"
        role="option"
        aria-selected={active}
        onClick={() => choose(value)}
        className={`w-full flex items-center gap-2.5 px-2.5 py-2 rounded-lg text-left transition-colors ${
          active ? "bg-emerald-600/15 text-white" : "text-slate-300 hover:bg-slate-800"
        }`}
      >
        <span
          className={`w-7 h-7 rounded-lg flex items-center justify-center flex-shrink-0 border ${
            active
              ? "bg-emerald-500/15 border-emerald-500/40 text-emerald-300"
              : "bg-slate-800 border-slate-700 text-slate-400"
          }`}
        >
          {icon}
        </span>
        <span className="min-w-0 flex-1">
          <span className="block text-xs font-semibold truncate">{title}</span>
          <span className="block text-[10px] text-slate-500 font-mono">
            {online}/{tables} online · {formatNaira(revenue)} today
          </span>
        </span>
        {active && <Check className="w-3.5 h-3.5 text-emerald-400 flex-shrink-0" />}
      </button>
    );
  };

  return (
    <div ref={ref} className={`relative ${className}`}>
      <button
        type="button"
        onClick={() => setOpen((o) => !o)}
        aria-haspopup="listbox"
        aria-expanded={open}
        className="w-full flex items-center justify-between gap-2 px-2.5 py-2 rounded-lg bg-slate-900/90 border border-slate-800 hover:border-emerald-500/40 text-[11px] text-slate-300 transition-colors"
      >
        <span className="flex items-center gap-1.5 text-slate-400 flex-shrink-0">
          {isAll ? (
            <Layers className="w-3.5 h-3.5 text-emerald-400" />
          ) : (
            <Building className="w-3.5 h-3.5 text-emerald-400" />
          )}
          <span>Zone:</span>
        </span>
        <span className="flex items-center gap-1 min-w-0">
          <span className={`font-semibold text-slate-100 truncate ${busy ? "opacity-60" : ""}`}>{label}</span>
          <ChevronDown
            className={`w-3.5 h-3.5 text-slate-500 flex-shrink-0 transition-transform ${open ? "rotate-180" : ""}`}
          />
        </span>
      </button>

      {open && (
        <div
          role="listbox"
          className="absolute z-50 left-0 right-0 mt-1.5 p-1.5 rounded-xl bg-slate-900 border border-slate-700 shadow-2xl shadow-black/50 max-h-80 overflow-y-auto"
        >
          <Option
            value={ALL_ZONES}
            title="All Zones"
            tables={totals.tableCount}
            online={totals.onlineCount}
            revenue={totals.todayRevenue}
            icon={<Layers className="w-3.5 h-3.5" />}
          />
          {zones.length > 0 && <div className="my-1 border-t border-slate-800" />}
          {zones.map((z) => (
            <Option
              key={z.name}
              value={z.name}
              title={z.name}
              tables={z.tableCount}
              online={z.onlineCount}
              revenue={z.todayRevenue}
              icon={<Building className="w-3.5 h-3.5" />}
            />
          ))}
          <div className="my-1 border-t border-slate-800" />
          <button
            type="button"
            onClick={() => {
              setOpen(false);
              onManage();
            }}
            className="w-full flex items-center gap-2 px-2.5 py-2 rounded-lg text-xs text-emerald-400 hover:bg-slate-800 font-medium"
          >
            <Settings2 className="w-3.5 h-3.5" />
            <span>Manage zones</span>
          </button>
        </div>
      )}
    </div>
  );
}

