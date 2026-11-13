/**
 * Smarthive Pool Hardware Identity & Cryptographic Claim Derivation
 *
 * Derives a deterministic 6-digit Claim PIN directly from the ESP8266 silicon Chip ID.
 * Parity tested with the C++ implementation on NodeMCU firmware.
 */

export function computeClaimPin(chipId: number): string {
  let hash = (chipId ^ 0x5a3c9e71) >>> 0;
  hash = Math.imul((hash >>> 16) ^ hash, 0x45d9f3b) >>> 0;
  hash = Math.imul((hash >>> 16) ^ hash, 0x45d9f3b) >>> 0;
  hash = ((hash >>> 16) ^ hash) >>> 0;
  const pin = 100000 + (hash % 900000);
  return pin.toString();
}

/**
 * Parses a serial number string (e.g. "SMP-8FA32B") into its 24-bit numeric Chip ID.
 * Returns null if the format does not match.
 */
export function parseSerialToChipId(serial: string): number | null {
  const match = serial.trim().toUpperCase().match(/^SMP-([0-9A-F]{6})$/i);
  if (!match) return null;
  const num = parseInt(match[1], 16);
  return Number.isNaN(num) ? null : num;
}

