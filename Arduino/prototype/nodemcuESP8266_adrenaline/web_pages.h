#pragma once
#include <Arduino.h>

// ==================== HTML PAGES STORED IN FLASH (PROGMEM) ====================
const char PROGMEM PAGE_DASHBOARD[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Adrenaline Snooker Hub</title>
  <style>
    :root {
      --bg-dark: #0b0e14;
      --card-bg: rgba(22, 27, 34, 0.9);
      --card-border: rgba(255, 255, 255, 0.08);
      --section-bg: rgba(255, 255, 255, 0.025);
      --text-main: #f0f6fc;
      --text-sub: #8b949e;
      --text-dim: #6e7681;
      --green: #10b981;
      --green-glow: rgba(16, 185, 129, 0.25);
      --green-dim: rgba(16, 185, 129, 0.12);
      --blue: #38bdf8;
      --blue-glow: rgba(56, 189, 248, 0.25);
      --blue-dim: rgba(56, 189, 248, 0.12);
      --red: #f85149;
      --red-dim: rgba(248, 81, 73, 0.15);
      --amber: #f59e0b;
      --radius-lg: 18px;
      --radius-md: 10px;
      --radius-sm: 6px;
    }
    * { margin: 0; padding: 0; box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background: radial-gradient(circle at 50% 0%, #161e2e 0%, #0b0e14 70%);
      color: var(--text-main);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 14px 12px 28px;
    }
    .app-shell {
      width: 100%;
      max-width: 500px;
      display: flex;
      flex-direction: column;
      gap: 12px;
    }
    /* Top Bar & Branding */
    .topbar {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border: 1px solid var(--card-border);
      border-radius: var(--radius-lg);
      padding: 12px 16px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      box-shadow: 0 10px 30px rgba(0,0,0,0.4);
    }
    .brand {
      display: flex;
      align-items: center;
      gap: 10px;
      text-decoration: none;
      color: var(--text-main);
    }
    .brand-icon {
      width: 36px;
      height: 36px;
      border-radius: 50%;
      background: linear-gradient(135deg, #10b981 0%, #047857 100%);
      display: flex;
      align-items: center;
      justify-content: center;
      box-shadow: 0 0 14px var(--green-glow);
      color: #ffffff;
      flex-shrink: 0;
    }
    .brand-info {
      display: flex;
      flex-direction: column;
    }
    .brand-title {
      font-size: 1.05rem;
      font-weight: 700;
      letter-spacing: -0.3px;
      display: flex;
      align-items: center;
      gap: 6px;
    }
    .brand-sub {
      font-size: 0.72rem;
      color: var(--text-sub);
    }
    .status-dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: #6e7681;
      display: inline-block;
      transition: background 0.3s;
    }
    .status-dot.online {
      background: var(--green);
      box-shadow: 0 0 8px var(--green);
    }
    .hamburger-btn {
      background: rgba(255, 255, 255, 0.05);
      border: 1px solid var(--card-border);
      border-radius: var(--radius-md);
      color: var(--text-main);
      width: 40px;
      height: 40px;
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      transition: background 0.2s;
    }
    .hamburger-btn:hover { background: rgba(255, 255, 255, 0.1); }
    /* Mobile App Side Overlay Drawer */
    .nav-backdrop {
      position: fixed;
      inset: 0;
      background: rgba(2, 6, 23, 0.65);
      backdrop-filter: blur(4px);
      -webkit-backdrop-filter: blur(4px);
      opacity: 0;
      pointer-events: none;
      transition: opacity 0.25s ease;
      z-index: 998;
    }
    .nav-backdrop.open {
      opacity: 1;
      pointer-events: auto;
    }
    .mobile-menu {
      position: fixed;
      top: 0;
      right: 0;
      bottom: 0;
      width: min(300px, 82vw);
      display: flex;
      flex-direction: column;
      gap: 6px;
      background: #111827;
      border-left: 1px solid var(--card-border);
      padding: 18px 14px 24px;
      box-shadow: -16px 0 40px rgba(0, 0, 0, 0.6);
      transform: translateX(100%);
      transition: transform 0.26s cubic-bezier(0.22, 1, 0.36, 1);
      z-index: 999;
      overflow-y: auto;
    }
    .mobile-menu.open {
      transform: translateX(0);
    }
    .drawer-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 4px 6px 14px;
      margin-bottom: 6px;
      border-bottom: 1px solid var(--card-border);
    }
    .drawer-title {
      font-size: 0.92rem;
      font-weight: 700;
      color: var(--text-main);
      letter-spacing: 0.02em;
    }
    .drawer-close {
      background: rgba(255, 255, 255, 0.06);
      border: 1px solid var(--card-border);
      color: var(--text-main);
      border-radius: 8px;
      width: 34px;
      height: 34px;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
    }
    .drawer-close:hover { background: rgba(255, 255, 255, 0.12); }
    .menu-item {
      display: flex;
      align-items: center;
      gap: 12px;
      padding: 12px 14px;
      border-radius: var(--radius-md);
      color: var(--text-main);
      text-decoration: none;
      font-size: 0.9rem;
      font-weight: 600;
      background: transparent;
      border: none;
      cursor: pointer;
      transition: background 0.15s, color 0.15s;
      width: 100%;
    }
    .menu-item:hover, .menu-item.active {
      background: var(--blue-dim);
      color: var(--blue);
    }
    /* Segmented Tab Bar */
    .tab-bar {
      display: flex;
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border: 1px solid var(--card-border);
      border-radius: var(--radius-lg);
      padding: 4px;
      gap: 4px;
      box-shadow: 0 6px 24px rgba(0,0,0,0.3);
    }
    .tab-btn {
      flex: 1;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 7px;
      padding: 10px 8px;
      border-radius: 12px;
      border: none;
      background: transparent;
      color: var(--text-sub);
      font-size: 0.84rem;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.2s;
      min-height: 42px;
    }
    .tab-btn:hover {
      color: var(--text-main);
      background: rgba(255, 255, 255, 0.04);
    }
    .tab-btn.active {
      color: #ffffff;
      background: var(--blue-dim);
      border: 1px solid rgba(56, 189, 248, 0.35);
      box-shadow: 0 2px 10px var(--blue-glow);
    }
    /* Tab Contents */
    .tab-pane {
      display: none;
      flex-direction: column;
      gap: 12px;
    }
    .tab-pane.active { display: flex; }
    /* Card Container */
    .card {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border: 1px solid var(--card-border);
      border-radius: var(--radius-lg);
      padding: 18px 16px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.35);
    }
    .card-title {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 0.74rem;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 1.1px;
      color: var(--blue);
      margin-bottom: 12px;
    }
    /* Counter Hero */
    .counter-hero {
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      padding: 22px 14px;
      background: linear-gradient(180deg, rgba(56, 189, 248, 0.1) 0%, rgba(56, 189, 248, 0.02) 100%);
      border: 1px solid rgba(56, 189, 248, 0.2);
      border-radius: 14px;
      text-align: center;
      margin-bottom: 12px;
    }
    .counter-value {
      font-size: 3.2rem;
      font-weight: 800;
      color: var(--blue);
      font-family: ui-monospace, SFMono-Regular, monospace;
      line-height: 1;
      text-shadow: 0 0 24px var(--blue-glow);
      transition: transform 0.15s cubic-bezier(0.175, 0.885, 0.32, 1.275);
    }
    .counter-value.bump {
      transform: scale(1.15);
      color: #7dd3fc;
    }
    .counter-sub {
      color: var(--text-sub);
      font-size: 0.74rem;
      text-transform: uppercase;
      letter-spacing: 1px;
      margin-top: 8px;
      font-weight: 600;
    }
    /* Buttons */
    .btn-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
      margin-top: 10px;
    }
    .btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      font-size: 0.86rem;
      font-weight: 600;
      padding: 11px 14px;
      border-radius: var(--radius-md);
      border: 1px solid var(--card-border);
      cursor: pointer;
      min-height: 44px;
      text-decoration: none;
      transition: all 0.2s;
      width: 100%;
    }
    .btn:active { transform: scale(0.98); }
    .btn-primary {
      background: #059669;
      color: #ffffff;
      border-color: rgba(255,255,255,0.15);
    }
    .btn-primary:hover { background: #10b981; }
    .btn-secondary {
      background: rgba(255, 255, 255, 0.05);
      color: var(--text-main);
      border-color: var(--card-border);
    }
    .btn-secondary:hover { background: rgba(255, 255, 255, 0.1); }
    .btn-danger {
      background: var(--red-dim);
      color: #ff7b72;
      border-color: rgba(248, 81, 73, 0.3);
    }
    .btn-danger:hover { background: rgba(248, 81, 73, 0.25); }
    .btn:disabled { opacity: 0.5; cursor: not-allowed; }
    /* Key-Value Data Rows */
    .data-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 8px 0;
      border-bottom: 1px solid rgba(255, 255, 255, 0.04);
    }
    .data-row:last-child { border-bottom: none; }
    .data-label { color: var(--text-sub); font-size: 0.85rem; }
    .data-val {
      color: var(--text-main);
      font-weight: 600;
      font-size: 0.85rem;
      font-family: ui-monospace, SFMono-Regular, monospace;
      text-align: right;
    }
    /* Badges */
    .badge {
      display: inline-flex;
      align-items: center;
      gap: 5px;
      padding: 3px 9px;
      border-radius: 20px;
      font-size: 0.72rem;
      font-weight: 700;
      letter-spacing: 0.5px;
    }
    .badge-online { background: var(--green-dim); color: #34d399; border: 1px solid rgba(16, 185, 129, 0.35); }
    .badge-offline { background: var(--red-dim); color: #f85149; border: 1px solid rgba(248, 81, 73, 0.35); }
    .badge-idle { background: rgba(110, 118, 129, 0.18); color: #8b949e; border: 1px solid rgba(110, 118, 129, 0.3); }
    .badge-sig { background: var(--blue-dim); color: var(--blue); border: 1px solid rgba(56, 189, 248, 0.3); }
    .sig-indicator { display: inline-flex; align-items: center; gap: 6px; }
    .sig-bars { display: flex; align-items: flex-end; gap: 2px; height: 12px; }
    .sig-bar { width: 3px; background: rgba(255,255,255,0.2); border-radius: 1px; }
    .sig-bar.active { background: #34d399; }
    .sig-bar:nth-child(1) { height: 3px; }
    .sig-bar:nth-child(2) { height: 6px; }
    .sig-bar:nth-child(3) { height: 9px; }
    .sig-bar:nth-child(4) { height: 12px; }
    /* Scanner */
    .scan-btn {
      width: 100%;
      background: var(--blue-dim);
      color: var(--blue);
      border: 1px solid rgba(56, 189, 248, 0.3);
    }
    .scan-btn:hover { background: rgba(56, 189, 248, 0.2); }
    .scan-results {
      margin-top: 10px;
      max-height: 180px;
      overflow-y: auto;
      display: none;
      border-radius: var(--radius-sm);
      border: 1px solid var(--card-border);
      background: rgba(0,0,0,0.3);
    }
    .scan-item {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 9px 12px;
      border-bottom: 1px solid rgba(255,255,255,0.05);
      cursor: pointer;
      transition: background 0.15s;
    }
    .scan-item:last-child { border-bottom: none; }
    .scan-item:hover { background: var(--blue-dim); }
    .scan-ssid {
      font-size: 0.85rem;
      font-weight: 600;
      color: #f0f6fc;
      display: flex;
      align-items: center;
      gap: 7px;
    }
    .scan-meta { font-size: 0.74rem; color: #8b949e; }
    /* Table for Reports */
    .table-wrap {
      width: 100%;
      overflow-x: auto;
      border-radius: var(--radius-md);
      border: 1px solid var(--card-border);
      background: rgba(0, 0, 0, 0.25);
      margin-top: 10px;
    }
    .report-table {
      width: 100%;
      border-collapse: collapse;
      text-align: left;
      font-size: 0.85rem;
      min-width: 300px;
    }
    .report-table th {
      padding: 10px 12px;
      background: rgba(255, 255, 255, 0.04);
      color: var(--text-sub);
      font-weight: 600;
      font-size: 0.72rem;
      text-transform: uppercase;
      letter-spacing: 0.8px;
      border-bottom: 1px solid var(--card-border);
    }
    .report-table td {
      padding: 10px 12px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.04);
      color: var(--text-main);
    }
    .report-table tr:last-child td { border-bottom: none; }
    .report-table tr:hover { background: rgba(255, 255, 255, 0.02); }
    .empty-state {
      text-align: center;
      padding: 24px 16px;
      color: var(--text-sub);
      font-size: 0.84rem;
      display: none;
    }
    /* Form */
    .form-group { margin-bottom: 13px; }
    label {
      display: block;
      color: var(--text-sub);
      font-size: 0.82rem;
      margin-bottom: 5px;
      font-weight: 500;
    }
    .input-wrap {
      position: relative;
      display: flex;
      align-items: center;
    }
    input[type="text"], input[type="password"] {
      width: 100%;
      min-height: 44px;
      padding: 10px 42px 10px 14px;
      border: 1px solid var(--card-border);
      border-radius: var(--radius-md);
      background: rgba(0, 0, 0, 0.3);
      color: #ffffff;
      font-size: 0.9rem;
      outline: none;
      transition: border-color 0.2s, box-shadow 0.2s;
    }
    input:focus {
      border-color: var(--blue);
      box-shadow: 0 0 0 3px rgba(56, 189, 248, 0.2);
    }
    .toggle-pwd {
      position: absolute;
      right: 10px;
      background: none;
      border: none;
      color: var(--text-sub);
      cursor: pointer;
      padding: 6px;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .toggle-pwd:hover { color: var(--blue); }
    .hint {
      color: var(--text-dim);
      font-size: 0.73rem;
      margin-top: 4px;
    }
    .divider {
      height: 1px;
      background: rgba(255, 255, 255, 0.08);
      margin: 16px 0;
    }
    /* Universal Icons */
    .icon {
      width: 16px;
      height: 16px;
      fill: none;
      stroke: currentColor;
      stroke-width: 2;
      stroke-linecap: round;
      stroke-linejoin: round;
      flex-shrink: 0;
    }
    .spin { animation: spin 1s linear infinite; }
    @keyframes spin { 100% { transform: rotate(360deg); } }
    .poll-note {
      text-align: center;
      color: var(--text-dim);
      font-size: 0.72rem;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
      margin-top: 10px;
    }
    #pane-settings .card.collapsible > .card-title {
      cursor: pointer;
      user-select: none;
      margin-bottom: 0;
      padding: 4px 0;
      justify-content: space-between;
    }
    #pane-settings .card.collapsible > .card-title .title-left {
      display: flex;
      align-items: center;
      gap: 8px;
    }
    #pane-settings .card.collapsible > .card-title .chevron {
      width: 16px;
      height: 16px;
      color: var(--text-sub);
      transition: transform 0.2s ease;
      flex-shrink: 0;
    }
    #pane-settings .card.collapsible.open > .card-title .chevron {
      transform: rotate(180deg);
      color: var(--accent);
    }
    #pane-settings .card.collapsible > .card-body {
      display: none;
    }
    #pane-settings .card.collapsible.open > .card-body {
      display: block;
      margin-top: 14px;
    }
    @media (max-width: 480px) {
      body { padding: 10px 8px 24px; }
      .counter-value { font-size: 2.6rem; }
      .btn-grid { grid-template-columns: 1fr; }
      .tab-btn span { font-size: 0.78rem; }
    }
  </style>
</head>
<body>
  <div class="app-shell">
    <!-- Topbar with Brand and Hamburger Toggle -->
    <div class="topbar">
      <a href="#snooker" class="brand" onclick="switchTab('snooker')">
        <div class="brand-icon">
          <svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"></circle><circle cx="12" cy="12" r="4"></circle><line x1="4.93" y1="4.93" x2="9.17" y2="9.17"></line><line x1="14.83" y1="14.83" x2="19.07" y2="19.07"></line><line x1="14.83" y1="9.17" x2="19.07" y2="4.93"></line><line x1="4.93" y1="19.07" x2="9.17" y2="14.83"></line></svg>
        </div>
        <div class="brand-info">
          <div class="brand-title">
            <span>Smarthive Pool</span>
            <span class="status-dot" id="hub-status-dot"></span>
          </div>
          <span class="brand-sub" id="brand-serial">Controller Node</span>
        </div>
      </a>
      <button type="button" class="hamburger-btn" id="nav-toggle" onclick="toggleNavMenu()" title="Toggle Menu">
        <svg class="icon" id="hamburger-icon" viewBox="0 0 24 24"><line x1="3" y1="12" x2="21" y2="12"></line><line x1="3" y1="6" x2="21" y2="6"></line><line x1="3" y1="18" x2="21" y2="18"></line></svg>
      </button>
    </div>

    <!-- Mobile Side Drawer Backdrop & Navigation Menu -->
    <div class="nav-backdrop" id="nav-backdrop" onclick="closeNavMenu()"></div>
    <div class="mobile-menu" id="mobile-menu">
      <div class="drawer-header">
        <span class="drawer-title">Menu</span>
        <button type="button" class="drawer-close" onclick="closeNavMenu()" aria-label="Close Menu">
          <svg class="icon" viewBox="0 0 24 24"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>
        </button>
      </div>
      <button type="button" class="menu-item active" data-tab="snooker" onclick="switchTab('snooker')">
        <svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"></circle><circle cx="12" cy="12" r="4"></circle></svg>
        <span>Snooker Hub</span>
      </button>
      <button type="button" class="menu-item" data-tab="reports" onclick="switchTab('reports')">
        <svg class="icon" viewBox="0 0 24 24"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path><polyline points="14 2 14 8 20 8"></polyline><line x1="16" y1="13" x2="8" y2="13"></line><line x1="16" y1="17" x2="8" y2="17"></line><polyline points="10 9 9 9 8 9"></polyline></svg>
        <span>Reports (48h Racks)</span>
      </button>
      <button type="button" class="menu-item" data-tab="connection" onclick="switchTab('connection')">
        <svg class="icon" viewBox="0 0 24 24"><path d="M5 12.55a11 11 0 0 1 14.08 0"></path><path d="M1.42 9a16 16 0 0 1 21.16 0"></path><path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path><line x1="12" y1="20" x2="12.01" y2="20"></line></svg>
        <span>Connection (AP &amp; STA)</span>
      </button>
      <button type="button" class="menu-item" data-tab="settings" onclick="switchTab('settings')">
        <svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="3"></circle><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"></path></svg>
        <span>Settings &amp; Diagnostics</span>
      </button>
      <a href="/update" class="menu-item">
        <svg class="icon" viewBox="0 0 24 24"><polyline points="16 16 12 12 8 16"></polyline><line x1="12" y1="12" x2="12" y2="21"></line><path d="M20.39 18.39A5 5 0 0 0 18 9h-1.26A8 8 0 1 0 3 16.3"></path><polyline points="16 16 12 12 8 16"></polyline></svg>
        <span>Firmware OTA Update</span>
      </a>
      <a href="/logout" class="menu-item" style="color:var(--red);">
        <svg class="icon" viewBox="0 0 24 24"><path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"></path><polyline points="16 17 21 12 16 7"></polyline><line x1="21" y1="12" x2="9" y2="12"></line></svg>
        <span>Sign Out</span>
      </a>
    </div>

    <!-- Segmented Tab Navigation Bar -->
    <div class="tab-bar">
      <button type="button" class="tab-btn active" data-tab="snooker" onclick="switchTab('snooker')">
        <svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"></circle><circle cx="12" cy="12" r="4"></circle></svg>
        <span>Snooker</span>
      </button>
      <button type="button" class="tab-btn" data-tab="reports" onclick="switchTab('reports')">
        <svg class="icon" viewBox="0 0 24 24"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path><polyline points="14 2 14 8 20 8"></polyline><line x1="16" y1="13" x2="8" y2="13"></line><line x1="16" y1="17" x2="8" y2="17"></line></svg>
        <span>Reports</span>
      </button>
      <button type="button" class="tab-btn" data-tab="connection" onclick="switchTab('connection')">
        <svg class="icon" viewBox="0 0 24 24"><path d="M5 12.55a11 11 0 0 1 14.08 0"></path><path d="M1.42 9a16 16 0 0 1 21.16 0"></path><path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path><line x1="12" y1="20" x2="12.01" y2="20"></line></svg>
        <span>Connection</span>
      </button>
      <button type="button" class="tab-btn" data-tab="settings" onclick="switchTab('settings')">
        <svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="3"></circle><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"></path></svg>
        <span>Settings</span>
      </button>
    </div>

    <!-- ==================== TAB 1: SNOOKER HUB ==================== -->
    <div class="tab-pane active" id="pane-snooker">
      <!-- Cloud Pairing Credentials Card -->
      <div class="card" style="border: 1px solid rgba(16, 185, 129, 0.45); background: rgba(16, 185, 129, 0.05); box-shadow: 0 0 16px rgba(16, 185, 129, 0.08);">
        <div class="card-title" style="color: var(--green);">
          <svg class="icon" viewBox="0 0 24 24"><path d="M12 2a10 10 0 1 0 10 10A10 10 0 0 0 12 2zm0 18a8 8 0 1 1 8-8 8 8 0 0 1-8 8z"></path><path d="M12 6a1 1 0 0 0-1 1v5.59l4.71 4.7a1 1 0 0 0 1.41-1.41L13 11.41V7a1 1 0 0 0-1-1z"></path></svg>
          <span>Cloud Dashboard Pairing</span>
        </div>
        <p style="font-size:0.78rem; color:var(--text-sub); margin-bottom:12px; line-height:1.4;">
          Pair this table on your <strong>Smarthive Pool Dashboard</strong> by entering this Serial Number and Claim PIN under <em>Pair Node</em>.
        </p>
        <div class="data-row">
          <span class="data-label">Hardware Serial</span>
          <span class="data-val" id="device-serial" style="font-family:monospace; font-weight:700; color:var(--blue); font-size:0.95rem;">SMP-LOADING</span>
        </div>
        <div class="data-row">
          <span class="data-label">6-Digit Claim PIN</span>
          <span class="data-val" id="device-claim-pin" style="font-family:monospace; font-weight:800; color:var(--green); font-size:1.2rem; letter-spacing:3px;">------</span>
        </div>
        <div class="data-row" style="border-bottom:none;">
          <span class="data-label">Device Node ID</span>
          <span class="data-val" id="device-id-label" style="font-family:monospace; font-size:0.8rem; color:var(--text-dim);">--</span>
        </div>
      </div>

      <!-- Game Counter Card -->
      <div class="card">
        <div class="card-title">
          <svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"></circle><circle cx="12" cy="12" r="4"></circle></svg>
          <span>Snooker Game Counter</span>
        </div>
        <div class="counter-hero">
          <div class="counter-value" id="btn-count">0</div>
          <div class="counter-sub" id="table-name-label">Total Games Unlocked (RFID &amp; Web)</div>
          <div class="counter-sub" id="session-rev-est" style="margin-top:4px;color:var(--accent);font-weight:600;"></div>
        </div>
        <div class="btn-grid">
          <button type="button" class="btn btn-primary" id="btn-trigger-servo" onclick="openServo()">
            <svg class="icon" viewBox="0 0 24 24"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 9.9-1"></path></svg>
            <span>Open Lock</span>
          </button>
          <button type="button" class="btn btn-danger" id="btn-reset-counter" onclick="resetCounter()">
            <svg class="icon" viewBox="0 0 24 24"><path d="M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8"></path><path d="M3 3v5h5"></path></svg>
            <span>Reset Count</span>
          </button>
        </div>
      </div>

      <!-- Mechanism & Activity Card (WITHOUT PIN INFO) -->
      <div class="card">
        <div class="card-title">
          <svg class="icon" viewBox="0 0 24 24"><rect x="2" y="3" width="20" height="14" rx="2" ry="2"></rect><line x1="8" y1="21" x2="16" y2="21"></line><line x1="12" y1="17" x2="12" y2="21"></line></svg>
          <span>Mechanism &amp; Activity</span>
        </div>
        <div class="data-row">
          <span class="data-label">Last Trigger</span>
          <span class="data-val" id="last-trigger">None</span>
        </div>
        <div class="data-row">
          <span class="data-label">Last Card UID</span>
          <span class="data-val" id="last-card-uid">None</span>
        </div>
        <div class="data-row">
          <span class="data-label">Staff Member</span>
          <span class="data-val" id="staff-name">None</span>
        </div>
        <div class="data-row">
          <span class="data-label">Staff ID</span>
          <span class="data-val" id="staff-id">None</span>
        </div>
        <div class="data-row">
          <span class="data-label">RC522 RFID Reader</span>
          <span id="rc522-status"><span class="badge badge-idle">PROBING...</span></span>
        </div>
        <div class="data-row">
          <span class="data-label">Servo Door Lock</span>
          <span id="servo-state"><span class="badge badge-idle"><svg class="icon" viewBox="0 0 24 24"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 10 0v4"></path></svg> LOCKED</span></span>
        </div>
        <div class="data-row">
          <span class="data-label">Pending Cloud Sync</span>
          <span id="offline-queue-status"><span class="badge badge-online"><svg class="icon" viewBox="0 0 24 24"><polyline points="20 6 9 17 4 12"></polyline></svg> 0 Queued</span></span>
        </div>
        <div class="data-row">
          <span class="data-label">Power Bank Keep-Alive</span>
          <span id="keepalive-state"><span class="badge badge-idle"><svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="5"></circle></svg> DISABLED</span></span>
        </div>
        <div style="margin-top:12px;">
          <button type="button" class="btn btn-secondary" id="btn-toggle-keepalive" onclick="toggleKeepAlive()">
            <svg class="icon" viewBox="0 0 24 24"><path d="M13 2L3 14h9l-1 8 10-12h-9l1-8z"></path></svg>
            <span id="btn-keepalive-label">Toggle Power Bank Keep-Alive</span>
          </button>
        </div>
      </div>
    </div>

    <!-- ==================== TAB 2: REPORTS ==================== -->
    <div class="tab-pane" id="pane-reports">
      <!-- 48-Hour Summary Card -->
      <div class="card">
        <div class="card-title">
          <svg class="icon" viewBox="0 0 24 24"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path><polyline points="14 2 14 8 20 8"></polyline><line x1="16" y1="13" x2="8" y2="13"></line><line x1="16" y1="17" x2="8" y2="17"></line></svg>
          <span>Rack Count Reports (48h Retention)</span>
        </div>
        <div class="btn-grid" style="margin-top:0;">
          <div class="counter-hero" style="margin-bottom:0;padding:14px 10px;">
            <div class="counter-value" id="rep-total-48h" style="font-size:2.2rem;">0</div>
            <div class="counter-sub">Last 48h Total Racks</div>
          </div>
          <div class="counter-hero" style="margin-bottom:0;padding:14px 10px;">
            <div class="counter-value" id="rep-active-days" style="font-size:2.2rem;color:#10b981;">0</div>
            <div class="counter-sub">Active Days Tracked</div>
          </div>
        </div>
      </div>

      <!-- Daily History Table Card -->
      <div class="card">
        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:8px;">
          <div class="card-title" style="margin-bottom:0;">
            <svg class="icon" viewBox="0 0 24 24"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect><line x1="3" y1="9" x2="21" y2="9"></line><line x1="3" y1="15" x2="21" y2="15"></line><line x1="9" y1="3" x2="9" y2="21"></line></svg>
            <span>Daily History Table</span>
          </div>
          <button type="button" class="btn btn-secondary" onclick="fetchReports()" style="width:auto;min-height:32px;padding:4px 10px;font-size:0.75rem;">
            <svg class="icon" viewBox="0 0 24 24"><polyline points="23 4 23 10 17 10"></polyline><polyline points="1 20 1 14 7 14"></polyline><path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"></path></svg>
            <span>Refresh</span>
          </button>
        </div>

        <div class="table-wrap">
          <table class="report-table">
            <thead>
              <tr>
                <th>Date</th>
                <th>Racks</th>
                <th>Last Time</th>
                <th>Status</th>
              </tr>
            </thead>
            <tbody id="report-table-body">
            </tbody>
          </table>
          <div class="empty-state" id="report-empty">
            No rack activity recorded in the past 48 hours.
          </div>
        </div>

        <div class="data-row" style="margin-top:12px;border-bottom:none;padding-bottom:0;">
          <span class="data-label" style="font-size:0.74rem;display:flex;align-items:center;gap:6px;color:var(--text-dim);">
            <svg class="icon" style="width:13px;height:13px;" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"></circle><polyline points="12 6 12 12 16 14"></polyline></svg>
            <span>Auto-cleanup policy: Records older than 48 hours are automatically pruned.</span>
          </span>
        </div>

        <div style="margin-top:12px;">
          <button type="button" class="btn btn-danger" onclick="clearReports()" style="font-size:0.8rem;min-height:38px;">
            <svg class="icon" viewBox="0 0 24 24"><polyline points="3 6 5 6 21 6"></polyline><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path></svg>
            <span>Clear Reports History</span>
          </button>
        </div>
      </div>
    </div>

    <!-- ==================== TAB 3: CONNECTION ==================== -->
    <div class="tab-pane" id="pane-connection">
      <!-- Access Point Card -->
      <div class="card">
        <div class="card-title">
          <svg class="icon" viewBox="0 0 24 24"><path d="M5 12.55a11 11 0 0 1 14.08 0"></path><path d="M1.42 9a16 16 0 0 1 21.16 0"></path><path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path><line x1="12" y1="20" x2="12.01" y2="20"></line></svg>
          <span>Access Point (AP)</span>
        </div>
        <div class="data-row">
          <span class="data-label">AP Status</span>
          <span class="badge badge-online"><svg class="icon" viewBox="0 0 24 24"><polyline points="20 6 9 17 4 12"></polyline></svg> ACTIVE</span>
        </div>
        <div class="data-row"><span class="data-label">AP SSID</span><span class="data-val" id="ap-ssid">--</span></div>
        <div class="data-row"><span class="data-label">Hardware Serial</span><span class="data-val" id="ap-serial" style="font-family:monospace; color:var(--blue); font-weight:700;">--</span></div>
        <div class="data-row"><span class="data-label">Claim PIN</span><span class="data-val" id="ap-claim-pin" style="font-family:monospace; color:var(--green); font-weight:700;">--</span></div>
        <div class="data-row"><span class="data-label">AP IP Address</span><span class="data-val" id="ap-ip">--</span></div>
        <div class="data-row"><span class="data-label">AP MAC Address</span><span class="data-val" id="ap-mac">--</span></div>
        <div class="data-row"><span class="data-label">Connected Clients</span><span class="data-val" id="ap-clients">--</span></div>
      </div>

      <!-- Router Connection Card -->
      <div class="card">
        <div class="card-title">
          <svg class="icon" viewBox="0 0 24 24"><rect x="2" y="2" width="20" height="8" rx="2" ry="2"></rect><rect x="2" y="14" width="20" height="8" rx="2" ry="2"></rect><line x1="6" y1="6" x2="6.01" y2="6"></line><line x1="6" y1="18" x2="6.01" y2="18"></line></svg>
          <span>Router WiFi (STA)</span>
        </div>
        <div class="data-row">
          <span class="data-label">Network Status</span>
          <span id="sta-status"><span class="badge badge-offline">CHECKING</span></span>
        </div>
        <div class="data-row"><span class="data-label">Router SSID</span><span class="data-val" id="sta-ssid">--</span></div>
        <div class="data-row"><span class="data-label">Router IP Address</span><span class="data-val" id="sta-ip">--</span></div>
        <div class="data-row"><span class="data-label">Gateway IP</span><span class="data-val" id="sta-gateway">--</span></div>
        <div class="data-row"><span class="data-label">Station MAC</span><span class="data-val" id="sta-mac">--</span></div>
        <div class="data-row">
          <span class="data-label">Signal Quality</span>
          <span class="data-val" id="sta-rssi-wrap">--</span>
        </div>
      </div>

      <!-- Client Device IP Card -->
      <div class="card">
        <div class="data-row" style="border-bottom:none;">
          <span class="data-label">Your Client IP</span>
          <span class="data-val" id="your-ip" style="color:var(--blue);">--</span>
        </div>
      </div>

      <div>
        <button type="button" class="btn btn-secondary" onclick="switchTab('settings')">
          <svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="3"></circle><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"></path></svg>
          <span>Configure WiFi in Settings</span>
        </button>
      </div>
    </div>

    <!-- ==================== TAB 3: SETTINGS & DIAGNOSTICS ==================== -->
    <div class="tab-pane" id="pane-settings">
      <!-- 2.4GHz WiFi Spectrum Scanner -->
      <div class="card">
        <div class="card-title">
          <svg class="icon" viewBox="0 0 24 24"><path d="M5 12.55a11 11 0 0 1 14.08 0"></path><path d="M1.42 9a16 16 0 0 1 21.16 0"></path><path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path><line x1="12" y1="20" x2="12.01" y2="20"></line></svg>
          <span>2.4GHz WiFi Scanner</span>
        </div>
        <button type="button" class="btn scan-btn" id="scan-btn" onclick="scanNetworks()">
          <svg class="icon" id="scan-icon" viewBox="0 0 24 24"><polyline points="23 4 23 10 17 10"></polyline><polyline points="1 20 1 14 7 14"></polyline><path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"></path></svg>
          <span id="scan-text">Scan Nearby Networks</span>
        </button>
        <div class="scan-results" id="scan-results"></div>
      </div>

      <!-- Staff RF Card Programmer Card -->
      <div class="card">
        <div class="card-title">
          <svg class="icon" viewBox="0 0 24 24"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"></path><circle cx="12" cy="7" r="4"></circle></svg>
          <span>Staff RF Card Programmer</span>
        </div>
        <p style="font-size:0.8rem;color:var(--text-sub);margin-bottom:12px;">
          Write employee name and ID into 13.56MHz MIFARE RFID cards.
        </p>

        <div class="form-group">
          <label>Staff Full Name</label>
          <div class="input-wrap">
            <input type="text" id="card-staff-name" maxlength="15" placeholder="e.g. Alex Morgan">
          </div>
          <p class="hint">Max 15 characters (ASCII)</p>
        </div>

        <div class="form-group">
          <label>Staff Employee ID</label>
          <div class="input-wrap">
            <input type="text" id="card-staff-id" maxlength="15" placeholder="e.g. EMP-104">
          </div>
          <p class="hint">Max 15 characters (e.g. EMP-01, MGR-02)</p>
        </div>

        <!-- Write Status Banner -->
        <div id="card-write-banner" style="display:none;padding:12px;border-radius:var(--radius-md);margin-bottom:12px;font-size:0.84rem;"></div>

        <div class="btn-grid" style="margin-top:8px;">
          <button type="button" class="btn btn-primary" id="btn-arm-writer" onclick="armCardWriter()">
            <svg class="icon" id="writer-icon" viewBox="0 0 24 24"><polyline points="20 6 9 17 4 12"></polyline></svg>
            <span id="writer-btn-text">Write to RF Card</span>
          </button>
          <button type="button" class="btn btn-secondary" id="btn-cancel-writer" onclick="cancelCardWriter()" style="display:none;">
            <svg class="icon" viewBox="0 0 24 24"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>
            <span>Cancel</span>
          </button>
        </div>
      </div>

      <!-- Card Access Control & Blacklist Card -->
      <div class="card">
        <div class="card-title">
          <svg class="icon" viewBox="0 0 24 24"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"></path></svg>
          <span>Card Access Control &amp; Blacklist</span>
        </div>
        <p style="font-size:0.8rem;color:var(--text-sub);margin-bottom:12px;">
          Instantly disable lost, revoked, or stolen cards to block door access and rack counting.
        </p>

        <div class="form-group">
          <label>Card UID to Block</label>
          <div class="input-wrap">
            <input type="text" id="field-block-uid" maxlength="15" placeholder="e.g. 04:A2:3B:5C">
          </div>
          <p class="hint">Hex UID format (e.g. 04:A2:3B:5C)</p>
        </div>

        <div class="btn-grid" style="margin-top:8px;margin-bottom:14px;">
          <button type="button" class="btn btn-danger" onclick="blockEnteredCard()">
            <svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"></circle><line x1="4.93" y1="4.93" x2="19.07" y2="19.07"></line></svg>
            <span>Block This Card</span>
          </button>
          <button type="button" class="btn btn-secondary" onclick="blockLastScannedCard()">
            <svg class="icon" viewBox="0 0 24 24"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"></path><circle cx="12" cy="7" r="4"></circle></svg>
            <span>Block Last Card</span>
          </button>
        </div>

        <div class="card-title" style="margin-top:14px;font-size:0.72rem;">
          <svg class="icon" viewBox="0 0 24 24"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 10 0v4"></path></svg>
          <span>Currently Blocked Cards</span>
        </div>
        <div id="blocked-cards-list" style="margin-top:8px;"></div>
      </div>

      <!-- Servo Angle Calibration Card -->
      <div class="card">
        <div class="card-title">
          <svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="3"></circle><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09a1.65 1.65 0 0 0-1-1.51 1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09a1.65 1.65 0 0 0 1.51-1 1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33h.01a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51h.01a1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82v.01a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"></path></svg>
          <span>Servo Angle Calibration</span>
        </div>
        <p style="font-size:0.8rem;color:var(--text-sub);margin-bottom:12px;">
          Adjust the resting (locked) and open (unlocked) angles. Saved to EEPROM and applied automatically on every reboot.
        </p>

        <div class="form-group">
          <label style="display:flex;justify-content:space-between;align-items:center;">
            <span>Starting Angle (Locked / Resting)</span>
            <span id="servo-start-val" style="font-weight:600;color:var(--accent);">180&deg;</span>
          </label>
          <div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap;">
            <input type="range" id="servo-start-range" min="0" max="180" value="180" oninput="syncServoInput('start', this.value)" style="flex:1;min-width:140px;">
            <input type="number" id="servo-start-num" min="0" max="180" value="180" oninput="syncServoInput('start', this.value)" style="width:76px;text-align:center;">
          </div>
          <p class="hint">Range: 0&deg; to 180&deg; (default: 180&deg;)</p>
        </div>

        <div class="form-group">
          <label style="display:flex;justify-content:space-between;align-items:center;">
            <span>Ending Angle (Unlocked / Open)</span>
            <span id="servo-end-val" style="font-weight:600;color:var(--accent);">0&deg;</span>
          </label>
          <div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap;">
            <input type="range" id="servo-end-range" min="0" max="180" value="0" oninput="syncServoInput('end', this.value)" style="flex:1;min-width:140px;">
            <input type="number" id="servo-end-num" min="0" max="180" value="0" oninput="syncServoInput('end', this.value)" style="width:76px;text-align:center;">
          </div>
          <p class="hint">Range: 0&deg; to 180&deg; (default: 0&deg;)</p>
        </div>

        <div class="form-group">
          <label style="display:flex;justify-content:space-between;align-items:center;">
            <span>Gate Open Hold Time</span>
            <span id="servo-hold-val" style="font-weight:600;color:var(--accent);">4s</span>
          </label>
          <div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap;">
            <input type="range" id="servo-hold-range" min="2" max="10" value="4" oninput="syncServoHold(this.value)" style="flex:1;min-width:140px;">
            <input type="number" id="servo-hold-num" min="2" max="10" value="4" oninput="syncServoHold(this.value)" style="width:76px;text-align:center;">
          </div>
          <p class="hint">How long the ball gate stays open before closing (2s to 10s, default: 4s)</p>
        </div>

        <div id="servo-config-feedback" style="display:none;padding:10px 12px;border-radius:var(--radius-md);margin-bottom:12px;font-size:0.82rem;"></div>

        <div class="btn-grid" style="margin-top:8px;">
          <button type="button" class="btn btn-primary" id="btn-save-servo" onclick="saveServoConfig(false)">
            <svg class="icon" viewBox="0 0 24 24"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"></path><polyline points="17 21 17 13 7 13 7 21"></polyline><polyline points="7 3 7 8 15 8"></polyline></svg>
            <span>Save Angles</span>
          </button>
          <button type="button" class="btn btn-secondary" id="btn-test-servo" onclick="saveServoConfig(true)">
            <svg class="icon" viewBox="0 0 24 24"><polygon points="5 3 19 12 5 21 5 3"></polygon></svg>
            <span>Save &amp; Test Sweep</span>
          </button>
        </div>
        <button type="button" class="btn btn-secondary" id="btn-invert-servo" onclick="invertServoAngles()" style="width:100%;margin-top:8px;">
          <svg class="icon" viewBox="0 0 24 24"><polyline points="17 1 21 5 17 9"></polyline><path d="M3 11V9a4 4 0 0 1 4-4h14"></path><polyline points="7 23 3 19 7 15"></polyline><path d="M21 13v2a4 4 0 0 1-4 4H3"></path></svg>
          <span>Invert Direction (Swap Start &amp; End)</span>
        </button>
        <p id="servo-saved-summary" class="hint" style="margin-top:10px;">Active EEPROM config: Start 180&deg; &rarr; End 0&deg; (Standard)</p>

        <div style="margin-top:16px;padding-top:14px;border-top:1px solid var(--border);">
          <div style="display:flex;align-items:center;justify-content:space-between;gap:8px;margin-bottom:8px;flex-wrap:wrap;">
            <span style="font-size:0.86rem;font-weight:600;color:var(--text-primary);display:inline-flex;align-items:center;gap:6px;">
              <svg class="icon" viewBox="0 0 24 24"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"></polyline></svg>
              <span>Multi-Rack Stress Test</span>
            </span>
            <span id="stress-status-badge" class="badge badge-idle">IDLE</span>
          </div>
          <p class="hint" style="margin-bottom:10px;">Runs repeated autonomous gate racks on the controller to verify zero brownouts or reboots under power-bank load.</p>
          <div style="display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:10px;">
            <div>
              <label for="stress-cycles" style="font-size:0.78rem;color:var(--text-secondary);display:block;margin-bottom:4px;">Rack Cycles (2-30)</label>
              <input type="number" id="stress-cycles" min="2" max="30" value="10" style="width:100%;padding:8px 10px;border-radius:var(--radius-sm);border:1px solid var(--border);background:var(--bg-input);color:var(--text-primary);font-size:0.9rem;">
            </div>
            <div>
              <label for="stress-gap" style="font-size:0.78rem;color:var(--text-secondary);display:block;margin-bottom:4px;">Gap Between Racks (s)</label>
              <input type="number" id="stress-gap" min="2" max="15" value="3" style="width:100%;padding:8px 10px;border-radius:var(--radius-sm);border:1px solid var(--border);background:var(--bg-input);color:var(--text-primary);font-size:0.9rem;">
            </div>
          </div>
          <div id="stress-feedback" style="display:none;padding:9px 12px;border-radius:var(--radius-md);margin-bottom:10px;font-size:0.82rem;"></div>
          <div class="btn-grid">
            <button type="button" class="btn btn-primary" id="btn-start-stress" onclick="startStressTest()">
              <svg class="icon" viewBox="0 0 24 24"><polygon points="5 3 19 12 5 21 5 3"></polygon></svg>
              <span>Run Stress Test</span>
            </button>
            <button type="button" class="btn btn-secondary" id="btn-stop-stress" onclick="stopStressTest()">
              <svg class="icon" viewBox="0 0 24 24"><rect x="6" y="6" width="12" height="12" rx="1"></rect></svg>
              <span>Stop Test</span>
            </button>
          </div>
        </div>
      </div>

      <!-- WiFi & Admin Credentials Form -->
      <div class="card">
        <div class="card-title">
          <svg class="icon" viewBox="0 0 24 24"><rect x="2" y="2" width="20" height="8" rx="2" ry="2"></rect><rect x="2" y="14" width="20" height="8" rx="2" ry="2"></rect><line x1="6" y1="6" x2="6.01" y2="6"></line><line x1="6" y1="18" x2="6.01" y2="18"></line></svg>
          <span>Router WiFi Configuration</span>
        </div>

        <!-- Previously Connected Networks Box -->
        <div id="saved-wifi-box" style="margin-bottom:14px;display:none;">
          <div style="font-size:0.75rem;font-weight:700;color:#94a3b8;text-transform:uppercase;margin-bottom:6px;letter-spacing:0.5px;">
            Previously Connected Networks
          </div>
          <div id="saved-wifi-list"></div>
        </div>

        <form id="form-wifi" onsubmit="event.preventDefault(); saveSettingsWithoutReboot();">
          <div class="form-group">
            <label>Router SSID</label>
            <div class="input-wrap">
              <input type="text" name="ssid" id="field-ssid" maxlength="32" placeholder="e.g. MySnookerClubWiFi">
            </div>
          </div>
          <div class="form-group">
            <label>Router Password</label>
            <div class="input-wrap">
              <input type="password" name="pass" id="field-pass" maxlength="64" placeholder="WPA2 Pre-Shared Key">
              <button type="button" class="toggle-pwd" onclick="togglePassword('field-pass', this)" title="Show/Hide Password">
                <svg class="icon" viewBox="0 0 24 24"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>
              </button>
            </div>
            <p class="hint">Standard WPA/WPA2 keys up to 64 characters.</p>
          </div>

          <div class="divider"></div>

          <div class="card-title">
            <svg class="icon" viewBox="0 0 24 24"><rect x="2" y="2" width="20" height="8" rx="2" ry="2"></rect><rect x="2" y="14" width="20" height="8" rx="2" ry="2"></rect><line x1="6" y1="6" x2="6.01" y2="6"></line><line x1="6" y1="18" x2="6.01" y2="18"></line></svg>
            <span>Adrenaline Cloud Table Binding</span>
          </div>
          <div class="form-group">
            <label>Adrenaline Table Node ID</label>
            <div class="input-wrap">
              <input type="text" name="device_id" id="field-device-id" maxlength="31" placeholder="snooker_table_1">
            </div>
            <p class="hint">Matches your Table Node ID on the Adrenaline web application (Default: <code>snooker_table_1</code>).</p>
          </div>

          <div class="divider"></div>

          <div class="card-title">
            <svg class="icon" viewBox="0 0 24 24"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 10 0v4"></path></svg>
            <span>Portal Authentication</span>
          </div>
          <div class="form-group">
            <label>Admin Username</label>
            <div class="input-wrap">
              <input type="text" name="user" id="field-user" required maxlength="16">
            </div>
          </div>
          <div class="form-group">
            <label>Admin Password</label>
            <div class="input-wrap">
              <input type="password" name="auth" id="field-auth" required maxlength="16">
              <button type="button" class="toggle-pwd" onclick="togglePassword('field-auth', this)" title="Show/Hide Password">
                <svg class="icon" viewBox="0 0 24 24"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>
              </button>
            </div>
          </div>

          <button type="submit" class="btn btn-primary" style="margin-top:6px;">
            <svg class="icon" viewBox="0 0 24 24"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"></path><polyline points="17 21 17 13 7 13 7 21"></polyline><polyline points="7 3 7 8 15 8"></polyline></svg>
            <span>Save &amp; Connect (No Reboot)</span>
          </button>
          <div id="wifi-live-feedback" style="display:none;margin-top:10px;padding:10px;border-radius:8px;font-size:0.8rem;line-height:1.4;"></div>
        </form>
      </div>

      <!-- Device Diagnostics (MOVED TO SETTINGS) -->
      <div class="card">
        <div class="card-title">
          <svg class="icon" viewBox="0 0 24 24"><rect x="4" y="4" width="16" height="16" rx="2" ry="2"></rect><rect x="9" y="9" width="6" height="6"></rect><line x1="9" y1="1" x2="9" y2="4"></line><line x1="15" y1="1" x2="15" y2="4"></line><line x1="9" y1="20" x2="9" y2="23"></line><line x1="15" y1="20" x2="15" y2="23"></line><line x1="20" y1="9" x2="23" y2="9"></line><line x1="20" y1="14" x2="23" y2="14"></line><line x1="1" y1="9" x2="4" y2="9"></line><line x1="1" y1="14" x2="4" y2="14"></line></svg>
          <span>Device Diagnostics</span>
        </div>
        <div class="data-row"><span class="data-label">System Uptime</span><span class="data-val" id="sys-uptime">--</span></div>
        <div class="data-row"><span class="data-label">Free Heap RAM</span><span class="data-val" id="sys-heap">--</span></div>
        <div class="data-row"><span class="data-label">CPU Frequency</span><span class="data-val" id="sys-cpu">--</span></div>
        <div class="data-row"><span class="data-label">Chip ID</span><span class="data-val" id="sys-chip">--</span></div>
      </div>

      <!-- RGB LED Status Indicator Guide -->
      <div class="card" id="card-rgb-guide">
        <div class="card-title">
          <svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="5"></circle><line x1="12" y1="1" x2="12" y2="3"></line><line x1="12" y1="21" x2="12" y2="23"></line><line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line><line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line><line x1="1" y1="12" x2="3" y2="12"></line><line x1="21" y1="12" x2="23" y2="12"></line><line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line><line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line></svg>
          <span>RGB LED Status Guide</span>
        </div>
        <div style="display:flex;align-items:center;gap:10px;padding:10px 12px;border-radius:10px;background:rgba(255,255,255,0.03);border:1px solid var(--card-border);margin-bottom:12px;">
          <span id="led-live-dot" style="width:12px;height:12px;border-radius:50%;background:#10b981;box-shadow:0 0 10px #10b981;flex-shrink:0;"></span>
          <div style="font-size:0.82rem;line-height:1.35;">
            <span style="color:var(--text-sub);font-size:0.74rem;display:block;">Current LED Indicator</span>
            <strong id="led-live-label" style="color:var(--text-main);">Green Heartbeat (Online &amp; Ready)</strong>
          </div>
        </div>
        <div style="display:grid;gap:8px;font-size:0.8rem;">
          <div style="display:flex;align-items:center;gap:10px;padding:7px 10px;border-radius:8px;background:rgba(16,185,129,0.08);border:1px solid rgba(16,185,129,0.2);">
            <span style="width:11px;height:11px;border-radius:50%;background:#10b981;box-shadow:0 0 8px #10b981;flex-shrink:0;"></span>
            <div><strong>Green Pulse (every 2.5s)</strong> &mdash; <span style="color:var(--text-sub);">Online &amp; Ready for RFID card tap</span></div>
          </div>
          <div style="display:flex;align-items:center;gap:10px;padding:7px 10px;border-radius:8px;background:rgba(16,185,129,0.14);border:1px solid rgba(16,185,129,0.32);">
            <span style="width:11px;height:11px;border-radius:50%;background:#34d399;box-shadow:0 0 10px #34d399;flex-shrink:0;"></span>
            <div><strong>Rapid Green Blink (5 Hz)</strong> &mdash; <span style="color:var(--text-sub);">Card Accepted / Ball Gate Open</span></div>
          </div>
          <div style="display:flex;align-items:center;gap:10px;padding:7px 10px;border-radius:8px;background:rgba(245,158,11,0.10);border:1px solid rgba(245,158,11,0.25);">
            <span style="width:11px;height:11px;border-radius:50%;background:#f59e0b;box-shadow:0 0 8px #f59e0b;flex-shrink:0;"></span>
            <div><strong>Amber Pulse (Red + Green)</strong> &mdash; <span style="color:var(--text-sub);">Tap-to-Program Mode Armed (Present Staff Card)</span></div>
          </div>
          <div style="display:flex;align-items:center;gap:10px;padding:7px 10px;border-radius:8px;background:rgba(248,81,73,0.10);border:1px solid rgba(248,81,73,0.25);">
            <span style="width:11px;height:11px;border-radius:50%;background:#f85149;box-shadow:0 0 8px #f85149;flex-shrink:0;"></span>
            <div><strong>Red Blink / Flash</strong> &mdash; <span style="color:var(--text-sub);">Offline / SoftAP Mode or Card Rejected</span></div>
          </div>
        </div>
      </div>

      <!-- Device Event Logs (Black Box) -->
      <div class="card" id="card-device-logs">
        <div class="card-title">
          <svg class="icon" viewBox="0 0 24 24"><polyline points="4 17 10 11 4 5"></polyline><line x1="12" y1="19" x2="20" y2="19"></line></svg>
          <span>Device Event Logs (Black Box)</span>
        </div>
        <p style="font-size:0.8rem;color:var(--text-sub);margin-bottom:10px;">
          Persistent hardware log stored in flash. Captures reboots, brownouts during servo sweeps, RFID reads, lockouts, Wi-Fi drops, and cloud errors.
        </p>
        <div id="device-logs-box" style="background:#090d14;border:1px solid var(--card-border);border-radius:10px;padding:10px 12px;font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-size:0.74rem;line-height:1.5;max-height:280px;overflow-y:auto;white-space:pre-wrap;word-break:break-word;color:#cbd5e1;margin-bottom:10px;">Tap Refresh to load device logs...</div>
        <div class="btn-grid">
          <button type="button" class="btn btn-primary" onclick="fetchDeviceLogs()">
            <svg class="icon" viewBox="0 0 24 24"><polyline points="23 4 23 10 17 10"></polyline><polyline points="1 20 1 14 7 14"></polyline><path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"></path></svg>
            <span>Refresh Logs</span>
          </button>
          <button type="button" class="btn btn-secondary" onclick="downloadDeviceLogs()">
            <svg class="icon" viewBox="0 0 24 24"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path><polyline points="7 10 12 15 17 10"></polyline><line x1="12" y1="15" x2="12" y2="3"></line></svg>
            <span>Download .txt</span>
          </button>
        </div>
        <button type="button" class="btn btn-danger" onclick="clearDeviceLogs()" style="width:100%;margin-top:8px;">
          <svg class="icon" viewBox="0 0 24 24"><polyline points="3 6 5 6 21 6"></polyline><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path></svg>
          <span>Clear Logs</span>
        </button>
      </div>

      <!-- Device Operations -->
      <div class="card">
        <div class="card-title">
          <svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"></circle><line x1="12" y1="8" x2="12" y2="12"></line><line x1="12" y1="16" x2="12.01" y2="16"></line></svg>
          <span>Device Operations</span>
        </div>
        <div class="btn-grid">
          <button type="button" class="btn btn-secondary" onclick="rebootDevice()">
            <svg class="icon" viewBox="0 0 24 24"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg>
            <span>Reboot Hub</span>
          </button>
          <button type="button" class="btn btn-danger" onclick="factoryReset()">
            <svg class="icon" viewBox="0 0 24 24"><polyline points="3 6 5 6 21 6"></polyline><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path></svg>
            <span>Factory Reset</span>
          </button>
        </div>
        <div style="margin-top:10px;">
          <a href="/update" class="btn btn-secondary">
            <svg class="icon" viewBox="0 0 24 24"><polyline points="16 16 12 12 8 16"></polyline><line x1="12" y1="12" x2="12" y2="21"></line><path d="M20.39 18.39A5 5 0 0 0 18 9h-1.26A8 8 0 1 0 3 16.3"></path><polyline points="16 16 12 12 8 16"></polyline></svg>
            <span>Firmware OTA Update</span>
          </a>
        </div>
      </div>
    </div>

    <!-- Live Polling Note -->
    <div class="poll-note">
      <svg class="icon" viewBox="0 0 24 24"><polyline points="23 4 23 10 17 10"></polyline><polyline points="1 20 1 14 7 14"></polyline><path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"></path></svg>
      <span>Live telemetry polling every 2s</span>
    </div>
  </div>

  <script>
    let writePollInterval = null;

    function armCardWriter() {
      const nameInput = document.getElementById("card-staff-name");
      const idInput = document.getElementById("card-staff-id");
      const name = (nameInput.value || "").trim();
      const id = (idInput.value || "").trim();

      if (!name || !id) {
        alert("Please enter both Staff Name and Staff ID.");
        return;
      }

      const btn = document.getElementById("btn-arm-writer");
      const cancelBtn = document.getElementById("btn-cancel-writer");
      const btnText = document.getElementById("writer-btn-text");
      const icon = document.getElementById("writer-icon");
      const banner = document.getElementById("card-write-banner");

      btn.disabled = true;
      cancelBtn.style.display = "inline-flex";

      const formData = new URLSearchParams();
      formData.append("name", name);
      formData.append("id", id);

      fetch("/api/card/write", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: formData.toString()
      })
        .then(r => r.json())
        .then(data => {
          banner.style.display = "block";
          banner.style.background = "var(--blue-dim)";
          banner.style.border = "1px solid rgba(56, 189, 248, 0.3)";
          banner.style.color = "var(--blue)";
          banner.innerHTML = '<div style="display:flex;align-items:center;gap:8px;"><svg class="icon spin" viewBox="0 0 24 24"><polyline points="23 4 23 10 17 10"></polyline><polyline points="1 20 1 14 7 14"></polyline><path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"></path></svg><span>Ready! Tap RF card against reader now... (25s)</span></div>';
          btnText.textContent = "Waiting for Card...";
          icon.classList.add("spin");

          if (writePollInterval) clearInterval(writePollInterval);
          writePollInterval = setInterval(pollWriteStatus, 1000);
        })
        .catch(err => {
          console.error("Arm writer error:", err);
          btn.disabled = false;
          cancelBtn.style.display = "none";
        });
    }

    function pollWriteStatus() {
      fetch("/api/card/write/status")
        .then(r => r.json())
        .then(data => {
          const banner = document.getElementById("card-write-banner");
          const btn = document.getElementById("btn-arm-writer");
          const cancelBtn = document.getElementById("btn-cancel-writer");
          const btnText = document.getElementById("writer-btn-text");
          const icon = document.getElementById("writer-icon");

          if (data.status === "SUCCESS") {
            clearInterval(writePollInterval);
            writePollInterval = null;
            banner.style.display = "block";
            banner.style.background = "var(--green-dim)";
            banner.style.border = "1px solid rgba(16, 185, 129, 0.4)";
            banner.style.color = "#34d399";
            banner.innerHTML = '<strong>Card Programmed!</strong> ' + data.message + ' (UID: ' + data.card_uid + ')';
            btn.disabled = false;
            cancelBtn.style.display = "none";
            btnText.textContent = "Write to RF Card";
            icon.classList.remove("spin");
          } else if (data.status === "ERROR") {
            clearInterval(writePollInterval);
            writePollInterval = null;
            banner.style.display = "block";
            banner.style.background = "var(--red-dim)";
            banner.style.border = "1px solid rgba(248, 81, 73, 0.4)";
            banner.style.color = "#ff7b72";
            banner.innerHTML = '<strong>Error:</strong> ' + data.message;
            btn.disabled = false;
            cancelBtn.style.display = "none";
            btnText.textContent = "Write to RF Card";
            icon.classList.remove("spin");
          }
        })
        .catch(err => console.error("Poll status error:", err));
    }

    function cancelCardWriter() {
      fetch("/api/card/write/cancel", { method: "POST" })
        .then(() => {
          if (writePollInterval) clearInterval(writePollInterval);
          writePollInterval = null;
          const banner = document.getElementById("card-write-banner");
          const btn = document.getElementById("btn-arm-writer");
          const cancelBtn = document.getElementById("btn-cancel-writer");
          const btnText = document.getElementById("writer-btn-text");
          const icon = document.getElementById("writer-icon");

          banner.style.display = "none";
          btn.disabled = false;
          cancelBtn.style.display = "none";
          btnText.textContent = "Write to RF Card";
          icon.classList.remove("spin");
        });
    }

    let currentLastScannedUid = "";

    function fetchBlockedCards() {
      fetch("/api/card/blocked")
        .then(r => r.json())
        .then(data => {
          const container = document.getElementById("blocked-cards-list");
          if (!container) return;
          container.innerHTML = "";

          if (!data.cards || data.cards.length === 0) {
            container.innerHTML = '<div style="padding:10px 12px;color:var(--text-sub);font-size:0.8rem;text-align:center;background:rgba(0,0,0,0.2);border-radius:var(--radius-sm);border:1px solid var(--card-border);">No cards currently blocked. All authorized staff cards permitted.</div>';
          } else {
            data.cards.forEach(card => {
              const row = document.createElement("div");
              row.className = "scan-item";
              row.style.cursor = "default";
              row.innerHTML = 
                '<div style="display:flex;align-items:center;gap:8px;">' +
                  '<svg class="icon" style="color:var(--red);" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"></circle><line x1="4.93" y1="4.93" x2="19.07" y2="19.07"></line></svg>' +
                  '<span style="font-family:ui-monospace,SFMono-Regular,monospace;font-weight:700;color:#f0f6fc;">' + card.uid + '</span>' +
                  '<span class="badge badge-offline">BLOCKED</span>' +
                '</div>' +
                '<button type="button" class="btn btn-secondary" style="width:auto;min-height:30px;padding:3px 10px;font-size:0.75rem;" onclick="unblockCard(\'' + card.uid + '\')">Unblock</button>';
              container.appendChild(row);
            });
          }
        })
        .catch(err => console.error("Fetch blocked cards error:", err));
    }

    function blockEnteredCard() {
      const input = document.getElementById("field-block-uid");
      const uid = (input.value || "").trim().toUpperCase();
      if (!uid) {
        alert("Please enter a Card UID to block.");
        return;
      }
      if (!confirm("Confirm: Block Card UID " + uid + "? Tapping this card will be rejected.")) return;

      const fd = new URLSearchParams();
      fd.append("uid", uid);
      fetch("/api/card/block", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: fd.toString()
      })
        .then(r => r.json())
        .then(data => {
          input.value = "";
          fetchBlockedCards();
        })
        .catch(err => console.error("Block error:", err));
    }

    function blockLastScannedCard() {
      if (!currentLastScannedUid || currentLastScannedUid === "None") {
        alert("No card has been scanned yet since last restart.");
        return;
      }
      if (!confirm("Confirm: Block last scanned card " + currentLastScannedUid + "?")) return;

      const fd = new URLSearchParams();
      fd.append("uid", currentLastScannedUid);
      fetch("/api/card/block", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: fd.toString()
      })
        .then(r => r.json())
        .then(() => {
          fetchBlockedCards();
        })
        .catch(err => console.error("Block error:", err));
    }

    function unblockCard(uid) {
      if (!confirm("Unblock card " + uid + " and restore access?")) return;
      const fd = new URLSearchParams();
      fd.append("uid", uid);
      fetch("/api/card/unblock", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: fd.toString()
      })
        .then(r => r.json())
        .then(() => {
          fetchBlockedCards();
        })
        .catch(err => console.error("Unblock error:", err));
    }

    function formatUptime(secs) {
      const d = Math.floor(secs / 86400);
      const h = Math.floor((secs % 86400) / 3600);
      const m = Math.floor((secs % 3600) / 60);
      const s = secs % 60;
      if (d > 0) return d + "d " + h + "h " + m + "m";
      if (h > 0) return h + "h " + m + "m " + s + "s";
      return m + "m " + s + "s";
    }

    function renderSignalBars(rssi) {
      let activeCount = 0;
      let quality = "Weak";
      if (rssi >= -60) { activeCount = 4; quality = "Excellent"; }
      else if (rssi >= -70) { activeCount = 3; quality = "Good"; }
      else if (rssi >= -80) { activeCount = 2; quality = "Fair"; }
      else if (rssi < -80 && rssi !== 0) { activeCount = 1; quality = "Weak"; }

      let html = '<div class="sig-indicator"><div class="sig-bars">';
      for (let i = 1; i <= 4; i++) {
        html += '<div class="sig-bar' + (i <= activeCount ? ' active' : '') + '"></div>';
      }
      html += '</div><span class="badge badge-sig">' + quality + ' (' + rssi + ' dBm)</span></div>';
      return html;
    }

    function closeNavMenu() {
      const menu = document.getElementById("mobile-menu");
      const backdrop = document.getElementById("nav-backdrop");
      if (menu) menu.classList.remove("open");
      if (backdrop) backdrop.classList.remove("open");
      document.body.style.overflow = "";
    }

    function toggleNavMenu() {
      const menu = document.getElementById("mobile-menu");
      const backdrop = document.getElementById("nav-backdrop");
      if (!menu) return;
      const isOpen = menu.classList.toggle("open");
      if (backdrop) backdrop.classList.toggle("open", isOpen);
      document.body.style.overflow = isOpen ? "hidden" : "";
    }

    function switchTab(tabName) {
      document.querySelectorAll(".tab-btn").forEach(btn => {
        btn.classList.toggle("active", btn.dataset.tab === tabName);
      });
      document.querySelectorAll(".menu-item[data-tab]").forEach(item => {
        item.classList.toggle("active", item.dataset.tab === tabName);
      });
      document.querySelectorAll(".tab-pane").forEach(pane => {
        pane.classList.toggle("active", pane.id === "pane-" + tabName);
      });
      closeNavMenu();
      if (window.history && window.history.replaceState) {
        window.history.replaceState(null, null, "#" + tabName);
      }
      if (tabName === "reports") {
        fetchReports();
      } else if (tabName === "settings") {
        fetchBlockedCards();
      }
    }

    function fetchReports() {
      fetch("/api/reports")
        .then(r => r.json())
        .then(data => {
          document.getElementById("rep-total-48h").textContent = data.total_48h || 0;
          document.getElementById("rep-active-days").textContent = data.count || 0;
          const tbody = document.getElementById("report-table-body");
          const emptyEl = document.getElementById("report-empty");
          tbody.innerHTML = "";

          if (!data.records || data.records.length === 0) {
            emptyEl.style.display = "block";
          } else {
            emptyEl.style.display = "none";
            data.records.forEach(rec => {
              const tr = document.createElement("tr");
              let badge = '<span class="badge badge-online">Active</span>';
              if (rec.age_hours !== undefined) {
                if (rec.age_hours > 36) {
                  badge = '<span class="badge badge-offline">Expiring</span>';
                } else if (rec.age_hours > 24) {
                  badge = '<span class="badge badge-sig">Day 2</span>';
                } else {
                  badge = '<span class="badge badge-online">Active</span>';
                }
              }
              tr.innerHTML = 
                '<td><strong>' + rec.date + '</strong></td>' +
                '<td><span style="font-family:ui-monospace,SFMono-Regular,monospace;font-weight:700;color:var(--blue);">' + rec.racks + '</span></td>' +
                '<td>' + (rec.last_time || "--:--") + '</td>' +
                '<td>' + badge + '</td>';
              tbody.appendChild(tr);
            });
          }
        })
        .catch(err => console.error("Reports fetch error:", err));
    }

    function clearReports() {
      if (!confirm("Are you sure you want to clear all 48-hour reports data?")) return;
      fetch("/api/reports/clear", { method: "POST" })
        .then(r => r.json())
        .then(() => {
          fetchReports();
        })
        .catch(err => console.error("Clear reports error:", err));
    }

    let prevBtnCount = null;

    function resetCounter() {
      const btn = document.getElementById("btn-reset-counter");
      btn.disabled = true;
      fetch("/api/button/reset", { method: "POST" })
        .then(r => r.json())
        .then(data => {
          const countEl = document.getElementById("btn-count");
          countEl.textContent = data.button_count || 0;
          prevBtnCount = data.button_count || 0;
          countEl.classList.add("bump");
          setTimeout(() => countEl.classList.remove("bump"), 180);
        })
        .catch(err => console.error("Reset failed:", err))
        .finally(() => { btn.disabled = false; });
    }

    function openServo() {
      const btn = document.getElementById("btn-trigger-servo");
      btn.disabled = true;
      fetch("/api/servo/open", { method: "POST" })
        .then(r => r.json())
        .then(() => { updateStatus(); })
        .catch(err => console.error("Servo trigger failed:", err))
        .finally(() => {
          setTimeout(() => { btn.disabled = false; }, 3200);
        });
    }

    let currentKeepAlive = false;

    function toggleKeepAlive() {
      const btn = document.getElementById("btn-toggle-keepalive");
      btn.disabled = true;
      const nextVal = currentKeepAlive ? 0 : 1;
      fetch("/api/keepalive?enable=" + nextVal, { method: "POST" })
        .then(r => r.json())
        .then(data => {
          if (data.keepalive_enabled !== undefined) {
            currentKeepAlive = data.keepalive_enabled;
            renderKeepAliveUI(currentKeepAlive);
          }
        })
        .catch(err => console.error("Keep-alive toggle failed:", err))
        .finally(() => { btn.disabled = false; });
    }

    function renderKeepAliveUI(enabled) {
      const el = document.getElementById("keepalive-state");
      const label = document.getElementById("btn-keepalive-label");
      if (enabled) {
        el.innerHTML = '<span class="badge badge-online"><svg class="icon" viewBox="0 0 24 24"><path d="M13 2L3 14h9l-1 8 10-12h-9l1-8z"></path></svg> ACTIVE</span>';
        if (label) label.textContent = "Disable Power Bank Keep-Alive";
      } else {
        el.innerHTML = '<span class="badge badge-idle"><svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="5"></circle></svg> DISABLED</span>';
        if (label) label.textContent = "Enable Power Bank Keep-Alive";
      }
    }

    function togglePassword(fieldId, btn) {
      const field = document.getElementById(fieldId);
      const isPwd = field.type === "password";
      field.type = isPwd ? "text" : "password";
      btn.innerHTML = isPwd
        ? '<svg class="icon" viewBox="0 0 24 24"><path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line></svg>'
        : '<svg class="icon" viewBox="0 0 24 24"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>';
    }

    function selectSSID(ssid) {
      document.getElementById("field-ssid").value = ssid;
      const pass = document.getElementById("field-pass");
      pass.focus();
    }

    function scanNetworks() {
      const btn = document.getElementById("scan-btn");
      const icon = document.getElementById("scan-icon");
      const text = document.getElementById("scan-text");
      const container = document.getElementById("scan-results");

      icon.classList.add("spin");
      text.textContent = "Scanning 2.4GHz spectrum...";
      btn.disabled = true;

      fetch("/api/scan")
        .then(r => r.json())
        .then(networks => {
          container.innerHTML = "";
          if (!networks || networks.length === 0) {
            container.innerHTML = '<div style="padding:12px;text-align:center;color:#8b949e;font-size:0.8rem;">No networks found</div>';
          } else {
            networks.forEach(net => {
              const item = document.createElement("div");
              item.className = "scan-item";
              item.onclick = () => selectSSID(net.ssid);
              const lockSvg = net.secure
                ? '<svg class="icon" style="width:13px;height:13px;color:#f59e0b;" viewBox="0 0 24 24"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 10 0v4"></path></svg>'
                : '<svg class="icon" style="width:13px;height:13px;color:#10b981;" viewBox="0 0 24 24"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 9.9-1"></path></svg>';
              item.innerHTML = '<div class="scan-ssid">' + lockSvg + '<span>' + net.ssid + '</span></div><div class="scan-meta"><span>' + net.rssi + ' dBm</span></div>';
              container.appendChild(item);
            });
          }
          container.style.display = "block";
        })
        .catch(() => {
          container.innerHTML = '<div style="padding:12px;text-align:center;color:#f85149;font-size:0.8rem;">Scan failed</div>';
          container.style.display = "block";
        })
        .finally(() => {
          icon.classList.remove("spin");
          text.textContent = "Scan Nearby Networks";
          btn.disabled = false;
        });
    }

    function renderSavedWiFiUI(networks) {
      const box = document.getElementById("saved-wifi-box");
      const list = document.getElementById("saved-wifi-list");
      if (!box || !list) return;

      if (!networks || networks.length === 0) {
        box.style.display = "none";
        list.innerHTML = "";
        return;
      }

      box.style.display = "block";
      list.innerHTML = "";
      networks.forEach(net => {
        const item = document.createElement("div");
        item.style.cssText = "display:flex;align-items:center;justify-content:space-between;padding:8px 10px;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.08);border-radius:8px;margin-bottom:6px;gap:8px;";

        const left = document.createElement("div");
        left.style.cssText = "display:flex;align-items:center;gap:8px;overflow:hidden;";
        left.innerHTML = '<svg class="icon" style="width:14px;height:14px;color:#38bdf8;flex-shrink:0;" viewBox="0 0 24 24"><path d="M5 12.55a11 11 0 0 1 14.08 0"></path><path d="M1.42 9a16 16 0 0 1 21.16 0"></path><path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path><line x1="12" y1="20" x2="12.01" y2="20"></line></svg>' +
          '<strong style="color:#ffffff;font-size:0.82rem;font-family:monospace;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;">' + net.ssid + '</strong>' +
          (net.is_active ? '<span class="badge badge-online" style="font-size:0.6rem;padding:2px 6px;">ACTIVE</span>' : '');

        const right = document.createElement("div");
        right.style.cssText = "display:flex;gap:4px;flex-shrink:0;";

        if (!net.is_active) {
          const btnConn = document.createElement("button");
          btnConn.type = "button";
          btnConn.className = "btn";
          btnConn.style.cssText = "padding:3px 8px;min-height:26px;font-size:0.72rem;background:rgba(56,189,248,0.15);color:#38bdf8;border-color:rgba(56,189,248,0.3);width:auto;";
          btnConn.textContent = "Connect";
          btnConn.onclick = () => connectSavedWiFi(net.ssid);
          right.appendChild(btnConn);
        }

        const btnForget = document.createElement("button");
        btnForget.type = "button";
        btnForget.className = "btn";
        btnForget.style.cssText = "padding:3px 8px;min-height:26px;font-size:0.72rem;background:rgba(248,81,73,0.12);color:#ff7b72;border-color:rgba(248,81,73,0.25);width:auto;";
        btnForget.textContent = "Forget";
        btnForget.onclick = () => forgetSavedWiFi(net.ssid);
        right.appendChild(btnForget);

        item.appendChild(left);
        item.appendChild(right);
        list.appendChild(item);
      });
    }

    let servoConfigLoaded = false;

    function syncServoInput(which, rawVal) {
      let val = parseInt(rawVal, 10);
      if (isNaN(val)) val = 0;
      if (val < 0) val = 0;
      if (val > 180) val = 180;
      const rEl = document.getElementById("servo-" + which + "-range");
      const nEl = document.getElementById("servo-" + which + "-num");
      const vEl = document.getElementById("servo-" + which + "-val");
      if (rEl) rEl.value = val;
      if (nEl) nEl.value = val;
      if (vEl) vEl.innerHTML = val + "&deg;";
    }

    function syncServoHold(rawVal) {
      let val = parseInt(rawVal, 10);
      if (isNaN(val)) val = 4;
      if (val < 2) val = 2;
      if (val > 10) val = 10;
      const rEl = document.getElementById("servo-hold-range");
      const nEl = document.getElementById("servo-hold-num");
      const vEl = document.getElementById("servo-hold-val");
      if (rEl) rEl.value = val;
      if (nEl) nEl.value = val;
      if (vEl) vEl.textContent = val + "s";
    }

    function formatServoSummary(startDeg, endDeg, holdSec) {
      const dirLabel = startDeg < endDeg ? "Inverted" : (startDeg > endDeg ? "Standard" : "Static");
      const holdTxt = (typeof holdSec === "number" && holdSec >= 2) ? (" &bull; Hold " + holdSec + "s") : "";
      return "Active EEPROM config: Start " + startDeg + "&deg; &rarr; End " + endDeg + "&deg; (" + dirLabel + ")" + holdTxt;
    }

    function invertServoAngles() {
      const curStart = parseInt(document.getElementById("servo-start-num").value, 10);
      const curEnd = parseInt(document.getElementById("servo-end-num").value, 10);
      const nextStart = isNaN(curEnd) ? 0 : curEnd;
      const nextEnd = isNaN(curStart) ? 180 : curStart;
      syncServoInput("start", nextStart);
      syncServoInput("end", nextEnd);
      saveServoConfig(false);
    }

    function saveServoConfig(runTest) {
      const startVal = parseInt(document.getElementById("servo-start-num").value, 10);
      const endVal = parseInt(document.getElementById("servo-end-num").value, 10);
      const holdEl = document.getElementById("servo-hold-num");
      const holdVal = holdEl ? parseInt(holdEl.value, 10) : 4;
      const fb = document.getElementById("servo-config-feedback");

      if (isNaN(startVal) || isNaN(endVal) || startVal < 0 || startVal > 180 || endVal < 0 || endVal > 180 || isNaN(holdVal) || holdVal < 2 || holdVal > 10) {
        fb.style.display = "block";
        fb.style.background = "rgba(248, 81, 73, 0.15)";
        fb.style.color = "#f85149";
        fb.style.border = "1px solid rgba(248, 81, 73, 0.3)";
        fb.textContent = "Angles must be 0–180 and hold time must be 2–10 seconds.";
        return;
      }

      fb.style.display = "block";
      fb.style.background = "rgba(56, 189, 248, 0.15)";
      fb.style.color = "#38bdf8";
      fb.style.border = "1px solid rgba(56, 189, 248, 0.3)";
      fb.textContent = runTest ? "Saving config & running sweep test..." : "Saving servo config to EEPROM...";

      const params = new URLSearchParams();
      params.append("start", String(startVal));
      params.append("end", String(endVal));
      params.append("hold", String(holdVal));

      fetch("/api/servo/config", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: params.toString()
      })
      .then(r => r.json())
      .then(res => {
        if (res.status === "ok") {
          const sumEl = document.getElementById("servo-saved-summary");
          if (sumEl) sumEl.innerHTML = formatServoSummary(res.servo_start_angle, res.servo_end_angle, res.servo_hold_sec);
          fb.style.background = "rgba(16, 185, 129, 0.15)";
          fb.style.color = "#10b981";
          fb.style.border = "1px solid rgba(16, 185, 129, 0.3)";
          fb.innerHTML = "Saved to memory! Start: <strong>" + res.servo_start_angle + "&deg;</strong>, End: <strong>" + res.servo_end_angle + "&deg;</strong>, Hold: <strong>" + (res.servo_hold_sec || holdVal) + "s</strong>." + (runTest ? " Sweeping servo now..." : "");
          if (runTest) {
            fetch("/api/servo/open", { method: "POST" }).catch(() => {});
          }
        } else {
          fb.style.background = "rgba(248, 81, 73, 0.15)";
          fb.style.color = "#f85149";
          fb.style.border = "1px solid rgba(248, 81, 73, 0.3)";
          fb.textContent = res.message || "Failed to save servo configuration.";
        }
      })
      .catch(err => {
        fb.style.background = "rgba(248, 81, 73, 0.15)";
        fb.style.color = "#f85149";
        fb.style.border = "1px solid rgba(248, 81, 73, 0.3)";
        fb.textContent = "Error saving servo angles: " + err.message;
      });
    }

    let stressStartBootCount = null;
    let stressWasActive = false;

    function startStressTest() {
      const cyclesEl = document.getElementById("stress-cycles");
      const gapEl = document.getElementById("stress-gap");
      const fb = document.getElementById("stress-feedback");
      const cycles = Math.max(2, Math.min(30, parseInt(cyclesEl ? cyclesEl.value : "10", 10) || 10));
      const gap = Math.max(2, Math.min(15, parseInt(gapEl ? gapEl.value : "3", 10) || 3));
      if (cyclesEl) cyclesEl.value = cycles;
      if (gapEl) gapEl.value = gap;

      fb.style.display = "block";
      fb.style.background = "rgba(88, 166, 255, 0.12)";
      fb.style.color = "#58a6ff";
      fb.style.border = "1px solid rgba(88, 166, 255, 0.3)";
      fb.textContent = "Starting " + cycles + "-rack stress test (gap " + gap + "s)...";

      stressStartBootCount = null;
      stressWasActive = true;
      fetch("/api/servo/stress?action=start&cycles=" + cycles + "&gap=" + gap, { method: "POST" })
        .then(r => r.json())
        .then(res => {
          if (res.status === "ok") {
            fb.textContent = "Stress test running: 0 / " + res.stress_total + " racks. Monitoring for brownouts...";
          } else {
            fb.style.background = "rgba(248, 81, 73, 0.15)";
            fb.style.color = "#f85149";
            fb.textContent = res.message || "Failed to start stress test.";
          }
        })
        .catch(err => {
          fb.style.background = "rgba(248, 81, 73, 0.15)";
          fb.style.color = "#f85149";
          fb.textContent = "Error starting stress test: " + err.message;
        });
    }

    function stopStressTest() {
      const fb = document.getElementById("stress-feedback");
      stressWasActive = false;
      fetch("/api/servo/stress?action=stop", { method: "POST" })
        .then(r => r.json())
        .then(res => {
          if (fb) {
            fb.style.display = "block";
            fb.style.background = "rgba(210, 153, 34, 0.15)";
            fb.style.color = "#d29922";
            fb.style.border = "1px solid rgba(210, 153, 34, 0.3)";
            fb.textContent = "Stress test stopped at " + (res.stress_done || 0) + " / " + (res.stress_total || 0) + " racks.";
          }
        })
        .catch(() => {});
    }

    function saveSettingsWithoutReboot() {
      const ssid = document.getElementById("field-ssid").value.trim();
      const pass = document.getElementById("field-pass").value;
      const deviceId = document.getElementById("field-device-id") ? document.getElementById("field-device-id").value.trim() : "";
      const user = document.getElementById("field-user").value.trim();
      const auth = document.getElementById("field-auth").value;

      if (!user || !auth) {
        alert("Admin Username and Password cannot be empty.");
        return;
      }

      const fb = document.getElementById("wifi-live-feedback");
      fb.style.display = "block";
      fb.style.background = "rgba(56, 189, 248, 0.15)";
      fb.style.color = "#38bdf8";
      fb.style.border = "1px solid rgba(56, 189, 248, 0.3)";
      fb.innerHTML = 'Connecting to <strong>' + (ssid || "Standalone Mode") + '</strong> in background without rebooting... SoftAP remains online.';

      const params = new URLSearchParams();
      params.append("ssid", ssid);
      params.append("pass", pass);
      params.append("device_id", deviceId);
      params.append("user", user);
      params.append("auth", auth);
      params.append("ajax", "1");

      fetch("/save", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: params.toString()
      })
      .then(r => r.json())
      .then(() => {
        let checkCount = 0;
        const pollInterval = setInterval(() => {
          checkCount++;
          fetch("/api/status")
            .then(r => r.json())
            .then(data => {
              if (data.saved_networks) renderSavedWiFiUI(data.saved_networks);
              if (data.sta_connected && (ssid === "" || data.sta_ssid === ssid)) {
                clearInterval(pollInterval);
                fb.style.background = "rgba(16, 185, 129, 0.15)";
                fb.style.color = "#10b981";
                fb.style.border = "1px solid rgba(16, 185, 129, 0.3)";
                fb.innerHTML = 'Connected to <strong>' + data.sta_ssid + '</strong>! IP: ' + data.sta_ip + ' (' + data.sta_rssi + ' dBm) - Zero Reboots.';
                updateStatus();
              } else if (checkCount > 15) {
                clearInterval(pollInterval);
                fb.style.background = "rgba(245, 158, 11, 0.15)";
                fb.style.color = "#f59e0b";
                fb.style.border = "1px solid rgba(245, 158, 11, 0.3)";
                fb.innerHTML = 'Connection attempt timed out. Check password or router range. Hotspot is still running.';
              }
            })
            .catch(() => {});
        }, 1200);
      })
      .catch(err => {
        fb.style.background = "rgba(248, 81, 73, 0.15)";
        fb.style.color = "#f85149";
        fb.style.border = "1px solid rgba(248, 81, 73, 0.3)";
        fb.innerHTML = 'Failed to submit settings: ' + err.message;
      });
    }

    function connectSavedWiFi(ssid) {
      const fb = document.getElementById("wifi-live-feedback");
      fb.style.display = "block";
      fb.style.background = "rgba(56, 189, 248, 0.15)";
      fb.style.color = "#38bdf8";
      fb.style.border = "1px solid rgba(56, 189, 248, 0.3)";
      fb.innerHTML = 'Connecting to saved network <strong>' + ssid + '</strong> in background without rebooting...';

      const params = new URLSearchParams();
      params.append("ssid", ssid);

      fetch("/api/wifi/connect", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: params.toString()
      })
      .then(r => r.json())
      .then(() => {
        let checkCount = 0;
        const pollInterval = setInterval(() => {
          checkCount++;
          fetch("/api/status")
            .then(r => r.json())
            .then(data => {
              if (data.saved_networks) renderSavedWiFiUI(data.saved_networks);
              if (data.sta_connected && data.sta_ssid === ssid) {
                clearInterval(pollInterval);
                fb.style.background = "rgba(16, 185, 129, 0.15)";
                fb.style.color = "#10b981";
                fb.style.border = "1px solid rgba(16, 185, 129, 0.3)";
                fb.innerHTML = 'Connected to <strong>' + ssid + '</strong>! IP: ' + data.sta_ip + ' (' + data.sta_rssi + ' dBm).';
                updateStatus();
              } else if (checkCount > 15) {
                clearInterval(pollInterval);
                fb.style.background = "rgba(245, 158, 11, 0.15)";
                fb.style.color = "#f59e0b";
                fb.style.border = "1px solid rgba(245, 158, 11, 0.3)";
                fb.innerHTML = 'Could not reach ' + ssid + '. Router might still be rebooting.';
              }
            });
        }, 1200);
      });
    }

    function forgetSavedWiFi(ssid) {
      if (!confirm('Forget "' + ssid + '" from saved memory?')) return;
      const params = new URLSearchParams();
      params.append("ssid", ssid);
      fetch("/api/wifi/forget", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: params.toString()
      })
      .then(r => r.json())
      .then(data => {
        if (data.ok) {
          updateStatus();
        } else {
          alert(data.error || "Failed to remove network");
        }
      });
    }

    function rebootDevice() {
      if (!confirm("Reboot Adrenaline Snooker Hub now?")) return;
      fetch("/api/restart", { method: "POST" })
        .then(() => {
          alert("Hub is rebooting. Please reconnect in 10 seconds.");
          window.location.href = "/";
        })
        .catch(() => { window.location.href = "/"; });
    }

    function factoryReset() {
      if (!confirm("WARNING: Factory Reset will erase all stored WiFi & login credentials. Continue?")) return;
      fetch("/api/reset", { method: "POST" })
        .then(() => {
          alert("Device reset to factory defaults. Connect to NodeMCU_Hub to reconfigure.");
          window.location.href = "/";
        })
        .catch(() => { window.location.href = "/"; });
    }

    function updateStatus() {
      fetch("/api/status")
        .then(r => r.json())
        .then(data => {
          if (data.keepalive_enabled !== undefined) {
            currentKeepAlive = data.keepalive_enabled;
            renderKeepAliveUI(currentKeepAlive);
          }
          if (data.button_count !== undefined) {
            const countEl = document.getElementById("btn-count");
            if (prevBtnCount !== null && data.button_count !== prevBtnCount) {
              countEl.textContent = data.button_count;
              countEl.classList.add("bump");
              setTimeout(() => countEl.classList.remove("bump"), 180);
            } else {
              countEl.textContent = data.button_count;
            }
            prevBtnCount = data.button_count;
          }
          if (data.last_trigger) {
            document.getElementById("last-trigger").textContent = data.last_trigger;
          }
          if (data.last_card_uid) {
            document.getElementById("last-card-uid").textContent = data.last_card_uid;
            currentLastScannedUid = data.last_card_uid;
          }
          if (data.staff_name) {
            document.getElementById("staff-name").textContent = data.staff_name;
          }
          if (data.staff_id) {
            document.getElementById("staff-id").textContent = data.staff_id;
          }
          if (data.rc522_connected !== undefined) {
            const rfidEl = document.getElementById("rc522-status");
            if (data.rc522_connected) {
              rfidEl.innerHTML = '<span class="badge badge-online"><svg class="icon" viewBox="0 0 24 24"><polyline points="20 6 9 17 4 12"></polyline></svg> CONNECTED (' + (data.rc522_version || "") + ')</span>';
            } else {
              rfidEl.innerHTML = '<span class="badge badge-offline"><svg class="icon" viewBox="0 0 24 24"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg> NOT DETECTED</span>';
            }
          }
          if (data.servo_state) {
            const servoEl = document.getElementById("servo-state");
            if (data.servo_state === "UNLOCKED") {
              servoEl.innerHTML = '<span class="badge badge-online"><svg class="icon" viewBox="0 0 24 24"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 9.9-1"></path></svg> UNLOCKED</span>';
            } else {
              servoEl.innerHTML = '<span class="badge badge-idle"><svg class="icon" viewBox="0 0 24 24"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 10 0v4"></path></svg> LOCKED</span>';
            }
          }
          const ledDot = document.getElementById("led-live-dot");
          const ledLbl = document.getElementById("led-live-label");
          if (ledDot && ledLbl) {
            let c = "#f85149", txt = "Red Blink (Offline / SoftAP Mode)";
            if (data.servo_state === "UNLOCKED") { c = "#34d399"; txt = "Rapid Green Blink (Ball Gate Open)"; }
            else if (data.rfid_write_pending) { c = "#f59e0b"; txt = "Amber Pulse (Tap-to-Program Armed)"; }
            else if (data.wifi_connected) { c = "#10b981"; txt = "Green Heartbeat (Online & Ready)"; }
            ledDot.style.background = c;
            ledDot.style.boxShadow = "0 0 10px " + c;
            ledLbl.textContent = txt;
          }
          if (typeof data.servo_start_angle === "number" && typeof data.servo_end_angle === "number") {
            const sumEl = document.getElementById("servo-saved-summary");
            if (sumEl) sumEl.innerHTML = formatServoSummary(data.servo_start_angle, data.servo_end_angle, data.servo_hold_sec);
            if (!servoConfigLoaded) {
              syncServoInput("start", data.servo_start_angle);
              syncServoInput("end", data.servo_end_angle);
              if (typeof data.servo_hold_sec === "number") syncServoHold(data.servo_hold_sec);
              servoConfigLoaded = true;
            }
          }
          if (typeof data.stress_active === "boolean") {
            const sb = document.getElementById("stress-status-badge");
            const sfb = document.getElementById("stress-feedback");
            if (data.stress_active) {
              if (stressStartBootCount === null && typeof data.boot_count === "number") stressStartBootCount = data.boot_count;
              stressWasActive = true;
              if (sb) { sb.className = "badge badge-warn"; sb.textContent = "RUNNING " + data.stress_done + "/" + data.stress_total; }
              if (sfb) {
                sfb.style.display = "block";
                sfb.style.background = "rgba(88, 166, 255, 0.12)";
                sfb.style.color = "#58a6ff";
                sfb.style.border = "1px solid rgba(88, 166, 255, 0.3)";
                sfb.innerHTML = "Running rack <strong>" + data.stress_done + " / " + data.stress_total + "</strong> | Boot #" + (data.boot_count || "-") + " | Heap: " + Math.round((data.free_heap || 0) / 1024) + " KB";
              }
            } else {
              if (sb) { sb.className = "badge badge-idle"; sb.textContent = "IDLE"; }
              if (stressWasActive && sfb) {
                stressWasActive = false;
                const rebooted = (stressStartBootCount !== null && typeof data.boot_count === "number" && data.boot_count > stressStartBootCount);
                sfb.style.display = "block";
                if (rebooted) {
                  sfb.style.background = "rgba(248, 81, 73, 0.15)";
                  sfb.style.color = "#f85149";
                  sfb.style.border = "1px solid rgba(248, 81, 73, 0.3)";
                  sfb.innerHTML = "<strong>BROWNOUT / REBOOT DETECTED!</strong> Boot jumped " + stressStartBootCount + " &rarr; " + data.boot_count + ". Check Device Event Log below.";
                } else if (data.stress_total > 0 && data.stress_done >= data.stress_total) {
                  sfb.style.background = "rgba(16, 185, 129, 0.15)";
                  sfb.style.color = "#10b981";
                  sfb.style.border = "1px solid rgba(16, 185, 129, 0.3)";
                  sfb.innerHTML = "<strong>PASS:</strong> Completed " + data.stress_done + " / " + data.stress_total + " racks with 0 reboots (Boot #" + (data.boot_count || "-") + ").";
                }
              }
            }
          }
          if (typeof data.offline_queue_count === "number") {
            const oqEl = document.getElementById("offline-queue-status");
            if (oqEl) {
              if (data.offline_queue_count > 0) {
                oqEl.innerHTML = '<span class="badge badge-warn"><svg class="icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"></circle><polyline points="12 6 12 12 16 14"></polyline></svg> ' + data.offline_queue_count + ' Pending</span>';
              } else {
                oqEl.innerHTML = '<span class="badge badge-online"><svg class="icon" viewBox="0 0 24 24"><polyline points="20 6 9 17 4 12"></polyline></svg> Synced (0)</span>';
              }
            }
          }
          const revEl = document.getElementById("session-rev-est");
          if (revEl && typeof data.game_count === "number" && typeof data.price_per_game === "number") {
            const total = data.game_count * data.price_per_game;
            const cur = data.currency || "NGN";
            revEl.textContent = "Est. Session: " + cur + " " + total.toLocaleString();
          }
          const tblEl = document.getElementById("table-name-label");
          if (tblEl && data.table_name) {
            tblEl.textContent = data.table_name + (data.maintenance_locked ? " (MAINTENANCE LOCKED)" : " - Total Games Unlocked");
          }

          document.getElementById("ap-ssid").textContent = data.ap_ssid || "--";
          if (document.getElementById("ap-serial")) document.getElementById("ap-serial").textContent = data.serial_number || "--";
          if (document.getElementById("ap-claim-pin")) document.getElementById("ap-claim-pin").textContent = data.claim_pin || "--";
          if (document.getElementById("device-serial")) document.getElementById("device-serial").textContent = data.serial_number || "--";
          if (document.getElementById("device-claim-pin")) document.getElementById("device-claim-pin").textContent = data.claim_pin || "--";
          if (document.getElementById("device-id-label")) document.getElementById("device-id-label").textContent = data.device_id || "--";
          if (document.getElementById("brand-serial") && data.serial_number) document.getElementById("brand-serial").textContent = data.serial_number;
          document.getElementById("ap-ip").textContent = data.ap_ip || "--";
          document.getElementById("ap-mac").textContent = data.ap_mac || "--";
          document.getElementById("ap-clients").textContent = data.ap_clients !== undefined ? data.ap_clients : "--";
          document.getElementById("sta-ssid").textContent = data.sta_ssid || "(Not Configured)";
          document.getElementById("sta-ip").textContent = data.sta_ip || "--";
          document.getElementById("sta-gateway").textContent = data.sta_gateway || "--";
          document.getElementById("sta-mac").textContent = data.sta_mac || "--";
          document.getElementById("your-ip").textContent = data.client_ip || "--";

          const statusEl = document.getElementById("sta-status");
          const rssiEl = document.getElementById("sta-rssi-wrap");
          const dotEl = document.getElementById("hub-status-dot");

          if (data.sta_connected) {
            statusEl.innerHTML = '<span class="badge badge-online"><svg class="icon" viewBox="0 0 24 24"><polyline points="20 6 9 17 4 12"></polyline></svg> ONLINE</span>';
            rssiEl.innerHTML = renderSignalBars(data.sta_rssi);
            if (dotEl) dotEl.className = "status-dot online";
          } else if (data.sta_connecting) {
            statusEl.innerHTML = '<span class="badge badge-idle" style="color:#38bdf8;border-color:rgba(56,189,248,0.4);"><svg class="icon spin" viewBox="0 0 24 24"><line x1="12" y1="2" x2="12" y2="6"></line><line x1="12" y1="18" x2="12" y2="22"></line><line x1="4.93" y1="4.93" x2="7.76" y2="7.76"></line><line x1="16.24" y1="16.24" x2="19.07" y2="19.07"></line><line x1="2" y1="12" x2="6" y2="12"></line><line x1="18" y1="12" x2="22" y2="12"></line><line x1="4.93" y1="19.07" x2="7.76" y2="16.24"></line><line x1="16.24" y1="7.76" x2="19.07" y2="4.93"></line></svg> CONNECTING</span>';
            rssiEl.textContent = "...";
            if (dotEl) dotEl.className = "status-dot";
          } else {
            statusEl.innerHTML = '<span class="badge badge-offline"><svg class="icon" viewBox="0 0 24 24"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg> OFFLINE</span>';
            rssiEl.textContent = "--";
            if (dotEl) dotEl.className = "status-dot";
          }

          if (data.saved_networks) {
            renderSavedWiFiUI(data.saved_networks);
          }

          document.getElementById("sys-uptime").textContent = formatUptime(data.uptime_secs || 0);
          document.getElementById("sys-heap").textContent = ((data.free_heap || 0) / 1024).toFixed(1) + " KB (" + (data.heap_frag || 0) + "% frag)";
          document.getElementById("sys-cpu").textContent = (data.cpu_freq || 80) + " MHz";
          document.getElementById("sys-chip").textContent = data.chip_id ? "0x" + data.chip_id.toUpperCase() : "--";
        })
        .catch(() => {
          const statusEl = document.getElementById("sta-status");
          if (statusEl) statusEl.innerHTML = '<span class="badge badge-offline">UNREACHABLE</span>';
        });
    }

    let cachedDeviceLogsText = "";

    function escapeLogHtml(str) {
      return str.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }

    function fetchDeviceLogs() {
      const box = document.getElementById("device-logs-box");
      if (!box) return;
      box.textContent = "Loading logs...";
      fetch("/api/logs")
        .then(r => r.text())
        .then(txt => {
          cachedDeviceLogsText = txt || "";
          if (!txt || !txt.trim()) {
            box.textContent = "(No diagnostic logs recorded yet)";
            return;
          }
          const lines = txt.trim().split("\n");
          box.innerHTML = lines.map(line => {
            const safe = escapeLogHtml(line);
            if (line.indexOf("[CRIT]") >= 0 || line.indexOf("[ERR]") >= 0) {
              return '<div style="color:#f87171;">' + safe + '</div>';
            } else if (line.indexOf("[WARN]") >= 0) {
              return '<div style="color:#fbbf24;">' + safe + '</div>';
            } else if (line.indexOf("[BOOT]") >= 0 || line.indexOf("[SERVO]") >= 0) {
              return '<div style="color:#38bdf8;">' + safe + '</div>';
            }
            return '<div>' + safe + '</div>';
          }).join("");
          box.scrollTop = box.scrollHeight;
        })
        .catch(err => {
          box.textContent = "Failed to load logs: " + err.message;
        });
    }

    function downloadDeviceLogs() {
      const text = cachedDeviceLogsText || "";
      if (!text.trim()) {
        fetchDeviceLogs();
        return;
      }
      const blob = new Blob([text], { type: "text/plain" });
      const url = URL.createObjectURL(blob);
      const a = document.createElement("a");
      a.href = url;
      a.download = "smarthive_device_logs.txt";
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);
    }

    function clearDeviceLogs() {
      if (!confirm("Clear all stored device diagnostic logs?")) return;
      fetch("/api/logs/clear", { method: "POST" })
        .then(() => fetchDeviceLogs())
        .catch(() => {});
    }

    function initSettingsAccordions() {
      const pane = document.getElementById("pane-settings");
      if (!pane) return;
      const cards = pane.querySelectorAll(":scope > .card");
      cards.forEach(card => {
        const title = card.querySelector(":scope > .card-title");
        if (!title || card.classList.contains("collapsible")) return;
        const body = document.createElement("div");
        body.className = "card-body";
        while (title.nextSibling) {
          body.appendChild(title.nextSibling);
        }
        card.appendChild(body);
        const left = document.createElement("span");
        left.className = "title-left";
        while (title.firstChild) left.appendChild(title.firstChild);
        title.appendChild(left);
        title.insertAdjacentHTML("beforeend", '<svg class="icon chevron" viewBox="0 0 24 24"><polyline points="6 9 12 15 18 9"></polyline></svg>');
        card.classList.add("collapsible");
        title.addEventListener("click", () => {
          const opened = card.classList.toggle("open");
          if (opened && card.id === "card-device-logs") {
            fetchDeviceLogs();
          }
        });
      });
    }

    // Initialize initial tab and load config
    window.addEventListener("DOMContentLoaded", () => {
      initSettingsAccordions();
      const hash = (window.location.hash || "").replace("#", "").toLowerCase();
      const path = (window.location.pathname || "").toLowerCase();
      if (hash === "settings" || path === "/settings") {
        switchTab("settings");
      } else if (hash === "connection" || path === "/connection") {
        switchTab("connection");
      } else if (hash === "reports" || path === "/reports") {
        switchTab("reports");
      } else {
        switchTab("snooker");
      }

      fetch("/api/config")
        .then(r => r.json())
        .then(cfg => {
          if (cfg.ssid) document.getElementById("field-ssid").value = cfg.ssid;
          if (cfg.pass) document.getElementById("field-pass").value = cfg.pass;
          if (cfg.user) document.getElementById("field-user").value = cfg.user;
          if (cfg.auth) document.getElementById("field-auth").value = cfg.auth;
          if (cfg.device_id) document.getElementById("field-device-id").value = cfg.device_id;
        })
        .catch(err => console.error("Config fetch error:", err));

      updateStatus();
      setInterval(updateStatus, 2000);
    });
  </script>
</body>
</html>
)rawliteral";

const char PROGMEM PAGE_SAVED[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Adrenaline Snooker Hub - Saved</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      background: radial-gradient(circle at 50% 0%, #161e2e 0%, #0b0e14 70%);
      color: #f0f6fc;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      padding: 16px;
    }
    .box {
      text-align: center;
      background: rgba(22, 27, 34, 0.9);
      backdrop-filter: blur(16px);
      padding: 36px 24px;
      border-radius: 18px;
      border: 1px solid rgba(255, 255, 255, 0.08);
      max-width: 420px;
      width: 100%;
      box-shadow: 0 10px 30px rgba(0,0,0,0.5);
    }
    .icon-wrap {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 56px;
      height: 56px;
      border-radius: 50%;
      background: rgba(16, 185, 129, 0.15);
      color: #10b981;
      margin-bottom: 16px;
    }
    .icon {
      width: 28px;
      height: 28px;
      fill: none;
      stroke: currentColor;
      stroke-width: 2.5;
      stroke-linecap: round;
      stroke-linejoin: round;
    }
    h2 { color: #ffffff; font-size: 1.3rem; margin-bottom: 8px; }
    p { color: #8b949e; font-size: 0.9rem; line-height: 1.5; margin-bottom: 20px; }
    .btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      background: rgba(255, 255, 255, 0.06);
      color: #38bdf8;
      border: 1px solid rgba(56, 189, 248, 0.3);
      padding: 10px 16px;
      border-radius: 8px;
      text-decoration: none;
      font-size: 0.88rem;
      font-weight: 600;
    }
  </style>
</head>
<body>
  <div class="box">
    <div class="icon-wrap">
      <svg class="icon" viewBox="0 0 24 24"><polyline points="20 6 9 17 4 12"></polyline></svg>
    </div>
    <h2>Settings Saved</h2>
    <p>Configuration updated successfully!<br>The controller is connecting to your router in the background without rebooting.<br>Your connection to the table Access Point remains online.</p>
    <a href="/" class="btn">
      <svg class="icon" style="width:16px;height:16px;" viewBox="0 0 24 24"><line x1="19" y1="12" x2="5" y2="12"></line><polyline points="12 19 5 12 12 5"></polyline></svg>
      <span>Return to Dashboard</span>
    </a>
  </div>
  <script>
    setTimeout(() => { window.location.href = "/"; }, 2500);
  </script>
</body>
</html>
)rawliteral";

const char PAGE_LOGIN[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Sign In | Smarthive Pool</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body {
      min-height: 100vh;
      background: #0d1117;
      background-image: radial-gradient(circle at 50% 0%, rgba(16, 185, 129, 0.14), transparent 60%);
      color: #e6edf3;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .login-card {
      width: 100%;
      max-width: 390px;
      background: #161b22;
      border: 1px solid #30363d;
      border-radius: 16px;
      padding: 28px 24px;
      box-shadow: 0 20px 50px rgba(0, 0, 0, 0.45);
    }
    .logo-wrap {
      width: 54px;
      height: 54px;
      border-radius: 14px;
      background: rgba(16, 185, 129, 0.12);
      border: 1px solid rgba(16, 185, 129, 0.35);
      color: #10b981;
      display: flex;
      align-items: center;
      justify-content: center;
      margin: 0 auto 16px;
    }
    .icon { width: 22px; height: 22px; stroke: currentColor; fill: none; stroke-width: 2; stroke-linecap: round; stroke-linejoin: round; }
    h1 { text-align: center; font-size: 1.25rem; font-weight: 700; margin-bottom: 6px; }
    .sub { text-align: center; font-size: 0.84rem; color: #8b949e; margin-bottom: 22px; }
    .form-group { margin-bottom: 16px; }
    label { display: block; font-size: 0.78rem; font-weight: 600; color: #8b949e; margin-bottom: 6px; text-transform: uppercase; letter-spacing: 0.04em; }
    .input-wrap { position: relative; display: flex; align-items: center; }
    input {
      width: 100%;
      padding: 12px 42px 12px 14px;
      border-radius: 10px;
      border: 1px solid #30363d;
      background: #0d1117;
      color: #e6edf3;
      font-size: 0.95rem;
      outline: none;
      transition: border-color 0.15s;
    }
    input:focus { border-color: #10b981; }
    .toggle-btn {
      position: absolute;
      right: 10px;
      background: transparent;
      border: none;
      color: #8b949e;
      cursor: pointer;
      padding: 4px;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .toggle-btn:hover { color: #e6edf3; }
    .err {
      display: none;
      background: rgba(248, 81, 73, 0.14);
      border: 1px solid rgba(248, 81, 73, 0.35);
      color: #f85149;
      padding: 10px 12px;
      border-radius: 10px;
      font-size: 0.84rem;
      margin-bottom: 14px;
      text-align: center;
    }
    .btn-submit {
      width: 100%;
      padding: 12px 16px;
      border: none;
      border-radius: 10px;
      background: #10b981;
      color: #06281e;
      font-size: 0.95rem;
      font-weight: 700;
      cursor: pointer;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      transition: opacity 0.15s;
    }
    .btn-submit:disabled { opacity: 0.6; cursor: not-allowed; }
    .foot { text-align: center; margin-top: 16px; font-size: 0.76rem; color: #6e7681; }
  </style>
</head>
<body>
  <div class="login-card">
    <div class="logo-wrap">
      <svg class="icon" style="width:28px;height:28px;" viewBox="0 0 24 24"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 10 0v4"></path></svg>
    </div>
    <h1>Smarthive Pool Portal</h1>
    <p class="sub">Sign in with your controller admin credentials</p>
    <div class="err" id="login-err"></div>
    <form id="login-form" onsubmit="submitLogin(event)">
      <div class="form-group">
        <label for="login-user">Username</label>
        <div class="input-wrap">
          <input type="text" id="login-user" name="username" autocomplete="username" placeholder="admin" required autofocus>
        </div>
      </div>
      <div class="form-group">
        <label for="login-pass">Password</label>
        <div class="input-wrap">
          <input type="password" id="login-pass" name="password" autocomplete="current-password" placeholder="Enter password" required>
          <button type="button" class="toggle-btn" onclick="togglePass()" aria-label="Toggle password visibility">
            <svg class="icon" id="eye-icon" style="width:18px;height:18px;" viewBox="0 0 24 24"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>
          </button>
        </div>
      </div>
      <button type="submit" class="btn-submit" id="btn-login">
        <svg class="icon" style="width:18px;height:18px;" viewBox="0 0 24 24"><path d="M15 3h4a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2h-4"></path><polyline points="10 17 15 12 10 7"></polyline><line x1="15" y1="12" x2="3" y2="12"></line></svg>
        <span id="btn-login-text">Sign In</span>
      </button>
    </form>
    <p class="foot">Protected Local Hardware Controller</p>
  </div>
  <script>
    function togglePass() {
      const inp = document.getElementById("login-pass");
      inp.type = inp.type === "password" ? "text" : "password";
    }
    function submitLogin(e) {
      e.preventDefault();
      const errEl = document.getElementById("login-err");
      const btn = document.getElementById("btn-login");
      const txt = document.getElementById("btn-login-text");
      errEl.style.display = "none";
      btn.disabled = true;
      txt.textContent = "Signing in...";
      const params = new URLSearchParams();
      params.append("username", document.getElementById("login-user").value.trim());
      params.append("password", document.getElementById("login-pass").value);
      fetch("/api/login", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: params.toString(),
        credentials: "same-origin"
      })
      .then(r => r.json().then(d => ({ ok: r.ok, data: d })))
      .then(res => {
        if (res.ok && res.data && res.data.ok) {
          window.location.href = "/";
        } else {
          errEl.textContent = (res.data && res.data.message) ? res.data.message : "Invalid username or password.";
          errEl.style.display = "block";
          btn.disabled = false;
          txt.textContent = "Sign In";
        }
      })
      .catch(() => {
        errEl.textContent = "Connection error. Please try again.";
        errEl.style.display = "block";
        btn.disabled = false;
        txt.textContent = "Sign In";
      });
    }
  </script>
</body>
</html>
)rawliteral";
