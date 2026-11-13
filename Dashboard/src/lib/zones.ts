import { TableDevice, User } from "@/models";

export const ALL_ZONES = "all";
export const MAX_ZONE_NAME_LENGTH = 50;

/** Normalises a zone name (trim + collapse whitespace). */
export function cleanZoneName(name: unknown): string {
  return String(name ?? "")
    .trim()
    .replace(/\s+/g, " ")
    .slice(0, MAX_ZONE_NAME_LENGTH);
}

/** Case-insensitive lookup of a zone name in a list. */
export function findZone(list: string[], name: string): string | undefined {
  const needle = name.toLowerCase();
  return list.find((v) => v.toLowerCase() === needle);
}

/**
 * Returns the owner's zones: the venues saved on their profile merged with any
 * zone names already used by their tables (legacy data). Missing names are
 * persisted so the list stays the single source of truth.
 */
export async function getOwnerZones(ownerId: string): Promise<string[]> {
  const [user, tableZones] = await Promise.all([
    User.findById(ownerId).select("venues activeVenue").lean(),
    TableDevice.distinct("venueName", { ownerId }),
  ]);

  const zones: string[] = [];
  const add = (name: unknown) => {
    const clean = cleanZoneName(name);
    if (clean && !findZone(zones, clean)) zones.push(clean);
  };

  (user?.venues || []).forEach(add);
  add(user?.activeVenue);
  (tableZones as string[])
    .filter((v) => v && v.toLowerCase() !== "unassigned" && v.toLowerCase() !== "all")
    .forEach(add);

  if (zones.length === 0) add("Main Floor");

  const saved = user?.venues || [];
  if (zones.length !== saved.length || zones.some((z, i) => z !== saved[i])) {
    await User.updateOne({ _id: ownerId }, { $set: { venues: zones } });
  }

  return zones;
}

