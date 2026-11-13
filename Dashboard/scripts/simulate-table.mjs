#!/usr/bin/env node
/**
 * Developer CLI Tool: Simulate hardware ESP8266/ESP32 table events
 * Usage:
 *   node scripts/simulate-table.mjs [deviceId] [event] [trigger]
 *
 * Example:
 *   node scripts/simulate-table.mjs snooker_table_01 button_press rfid
 */

const BASE_URL = process.env.BASE_URL || "http://localhost:3000";
const DEVICE_TOKEN = process.env.DEVICE_TOKEN || "adrenaline_snooker_secret_2026";

const deviceId = process.argv[2] || "snooker_table_01";
const event = process.argv[3] || "button_press";
const trigger = process.argv[4] || "rfid";

const payload = {
  device_id: deviceId,
  event: event,
  trigger: trigger,
  rssi: -58,
  ip: "192.168.1.155",
  chip_id: "SIM8266",
  ...(trigger === "rfid"
    ? {
        card_uid: "04:A2:3B:5C",
        staff_name: "Test Staff",
        staff_id: "STF-CLI",
      }
    : {}),
};

console.log(`📡 Transmitting simulated packet to ${BASE_URL}/api/snooker/sync...`);
console.log(JSON.stringify(payload, null, 2));

try {
  const res = await fetch(`${BASE_URL}/api/snooker/sync`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      "x-device-token": DEVICE_TOKEN,
    },
    body: JSON.stringify(payload),
  });

  const data = await res.json();
  console.log(`\nResponse [HTTP ${res.status}]:`, JSON.stringify(data, null, 2));
} catch (err) {
  console.error("\n❌ Network error:", err.message);
  process.exit(1);
}

