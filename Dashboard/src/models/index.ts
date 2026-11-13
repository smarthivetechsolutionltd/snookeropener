import mongoose, { Schema, Document, Model } from "mongoose";

export interface IUser extends Document {
  name: string;
  email: string;
  password?: string;
  emailVerified: boolean;
  emailVerificationToken?: string;
  emailVerificationExpires?: Date;
  role: "superadmin" | "admin" | "owner" | "manager" | "attendant";
  companyName: string;
  phoneNumber?: string;
  venues: string[];
  activeVenue: string;
  subscriptionPlan: "starter" | "growth" | "enterprise";
  subscriptionStatus: "active" | "trial" | "past_due";
  trialEndsAt?: Date;
  image?: string;
  city?: string;
  tableCount?: number;
  onboardingCompleted: boolean;
  dashboardZone: string;
  createdAt: Date;
  updatedAt: Date;
}

const UserSchema = new Schema<IUser>(
  {
    name: { type: String, required: true },
    email: { type: String, required: true, unique: true, lowercase: true, index: true },
    password: { type: String },
    emailVerified: { type: Boolean, default: false },
    emailVerificationToken: { type: String },
    emailVerificationExpires: { type: Date },
    role: {
      type: String,
      enum: ["superadmin", "admin", "owner", "manager", "attendant"],
      default: "owner",
    },
    companyName: { type: String, default: "My Billiards Arena" },
    phoneNumber: { type: String, default: "" },
    venues: { type: [String], default: ["Main Floor"] },
    activeVenue: { type: String, default: "Main Floor" },
    subscriptionPlan: {
      type: String,
      enum: ["starter", "growth", "enterprise"],
      default: "growth",
    },
    subscriptionStatus: {
      type: String,
      enum: ["active", "trial", "past_due"],
      default: "active",
    },
    trialEndsAt: { type: Date },
    image: { type: String },
    city: { type: String, default: "" },
    tableCount: { type: Number, default: 0 },
    // Google (OAuth) users must complete their profile before accessing the dashboard
    onboardingCompleted: { type: Boolean, default: false },
    // Last zone selected in the dashboard zone switcher ("all" = every zone)
    dashboardZone: { type: String, default: "all" },
  },
  { timestamps: true }
);

export interface IRfidWriteCmd {
  arm: boolean;
  staffName: string;
  staffId: string;
  role: string;
  armedAt?: Date | null;
}

export interface IOtaCmd {
  pending: boolean;
  targetVersion: string;
  firmwareUrl: string;
  triggeredAt?: Date | null;
  status: "idle" | "pending" | "downloading" | "success" | "failed";
  errorMessage?: string;
}

export interface IUnlockCmd {
  pending: boolean;
  requestedAt?: Date | null;
  staffName?: string;
}

export interface ITableDevice extends Document {
  deviceId: string;
  serialNumber: string;
  claimCode: string;
  isClaimed: boolean;
  ownerId?: string | null;
  ownerEmail?: string | null;
  name: string;
  venueName: string;
  location: string;
  deviceToken: string;
  pricePerGame: number;
  currency: string;
  subscriptionTier: "starter" | "growth" | "enterprise";
  subscriptionStatus: "active" | "trial" | "past_due";
  firmwareVersion: string;
  lastHeartbeat: Date | null;
  rssi: number;
  localIp: string;
  chipId: string;
  macAddress: string;
  freeHeap?: number;
  resetReason?: string;
  rc522Ok?: boolean;
  offlineQueueCount?: number;
  bootCount?: number;
  totalRacksLifetime: number;
  maintenanceLocked: boolean;
  rfidWriteCmd: IRfidWriteCmd;
  otaCmd: IOtaCmd;
  unlockCmd?: IUnlockCmd;
  createdAt: Date;
  updatedAt: Date;
}

const TableDeviceSchema = new Schema<ITableDevice>(
  {
    deviceId: { type: String, required: true, unique: true, index: true },
    serialNumber: { type: String, required: true, unique: true, index: true },
    claimCode: { type: String, required: true, index: true },
    isClaimed: { type: Boolean, default: false, index: true },
    ownerId: { type: String, default: null, index: true },
    ownerEmail: { type: String, default: null, index: true },
    name: { type: String, required: true },
    venueName: { type: String, default: "Main Floor" },
    location: { type: String, default: "North Bay" },
    deviceToken: {
      type: String,
      default: "adrenaline_snooker_secret_2026",
    },
    pricePerGame: { type: Number, default: 500 },
    currency: { type: String, default: "NGN" },
    subscriptionTier: {
      type: String,
      enum: ["starter", "growth", "enterprise"],
      default: "growth",
    },
    subscriptionStatus: {
      type: String,
      enum: ["active", "trial", "past_due"],
      default: "active",
    },
    firmwareVersion: { type: String, default: "Smarthive Pool V1.0" },
    lastHeartbeat: { type: Date, default: null },
    rssi: { type: Number, default: -58 },
    localIp: { type: String, default: "0.0.0.0" },
    chipId: { type: String, default: "Hardware Node" },
    macAddress: { type: String, default: "" },
    freeHeap: { type: Number, default: 0 },
    resetReason: { type: String, default: "" },
    rc522Ok: { type: Boolean, default: true },
    offlineQueueCount: { type: Number, default: 0 },
    bootCount: { type: Number, default: 0 },
    totalRacksLifetime: { type: Number, default: 0 },
    maintenanceLocked: { type: Boolean, default: false },
    rfidWriteCmd: {
      arm: { type: Boolean, default: false },
      staffName: { type: String, default: "" },
      staffId: { type: String, default: "" },
      role: { type: String, default: "staff" },
      armedAt: { type: Date, default: null },
    },
    otaCmd: {
      pending: { type: Boolean, default: false },
      targetVersion: { type: String, default: "" },
      firmwareUrl: { type: String, default: "" },
      triggeredAt: { type: Date, default: null },
      status: {
        type: String,
        enum: ["idle", "pending", "downloading", "success", "failed"],
        default: "idle",
      },
      errorMessage: { type: String, default: "" },
    },
    unlockCmd: {
      pending: { type: Boolean, default: false },
      requestedAt: { type: Date, default: null },
      staffName: { type: String, default: "" },
    },
  },
  { timestamps: true }
);

export interface IAuthorizedCard extends Document {
  ownerId?: string | null;
  cardUid: string;
  staffName: string;
  staffId: string;
  role: "admin" | "manager" | "staff" | "vip_member";
  status: "active" | "blocked";
  assignedTables: string[];
  dailyLimit: number;
  totalGamesUnlocked: number;
  totalRevenueGenerated: number;
  lastUsedAt: Date | null;
  lastUsedDevice: string;
  notes: string;
  createdAt: Date;
  updatedAt: Date;
}

const AuthorizedCardSchema = new Schema<IAuthorizedCard>(
  {
    ownerId: { type: String, default: null, index: true },
    cardUid: { type: String, required: true, unique: true, index: true },
    staffName: { type: String, required: true },
    staffId: { type: String, required: true },
    role: {
      type: String,
      enum: ["admin", "manager", "staff", "vip_member"],
      default: "staff",
    },
    status: {
      type: String,
      enum: ["active", "blocked"],
      default: "active",
    },
    assignedTables: { type: [String], default: ["*"] },
    dailyLimit: { type: Number, default: 200 },
    totalGamesUnlocked: { type: Number, default: 0 },
    totalRevenueGenerated: { type: Number, default: 0 },
    lastUsedAt: { type: Date, default: null },
    lastUsedDevice: { type: String, default: "" },
    notes: { type: String, default: "" },
  },
  { timestamps: true }
);

export interface IRackEvent extends Document {
  ownerId?: string | null;
  deviceId: string;
  serialNumber?: string;
  tableName: string;
  venueName: string;
  event:
    | "button_press"
    | "card_written_success"
    | "unauthorized_card_attempt"
    | "blocked_card_attempt"
    | "heartbeat"
    | "remote_unlock";
  trigger: "rfid" | "button" | "cloud" | "system";
  cardUid: string;
  staffName: string;
  staffId: string;
  priceCharged: number;
  currency: string;
  syncedFromOffline: boolean;
  clientEventId?: string;
  ageSeconds?: number;
  hardwareDate: string;
  hardwareTime: string;
  rssi: number;
  localIp: string;
  createdAt: Date;
}

const RackEventSchema = new Schema<IRackEvent>(
  {
    ownerId: { type: String, default: null, index: true },
    deviceId: { type: String, required: true, index: true },
    serialNumber: { type: String, default: "" },
    tableName: { type: String, default: "Table 1" },
    venueName: { type: String, default: "Main Floor" },
    event: {
      type: String,
      enum: [
        "button_press",
        "card_written_success",
        "unauthorized_card_attempt",
        "blocked_card_attempt",
        "heartbeat",
        "remote_unlock",
      ],
      required: true,
      index: true,
    },
    trigger: {
      type: String,
      enum: ["rfid", "button", "cloud", "system"],
      default: "rfid",
    },
    cardUid: { type: String, default: "" },
    staffName: { type: String, default: "" },
    staffId: { type: String, default: "" },
    priceCharged: { type: Number, default: 0 },
    currency: { type: String, default: "NGN" },
    syncedFromOffline: { type: Boolean, default: false },
    clientEventId: { type: String, unique: true, sparse: true },
    ageSeconds: { type: Number, default: 0 },
    hardwareDate: { type: String, default: "" },
    hardwareTime: { type: String, default: "" },
    rssi: { type: Number, default: -60 },
    localIp: { type: String, default: "" },
  },
  { timestamps: true }
);

export interface ICashReconciliation extends Document {
  ownerId?: string | null;
  venueName: string;
  shiftDate: string;
  shiftLabel: string;
  managerName: string;
  expectedCashFromCoinBox: number;
  expectedRevenueFromStaffRfid: number;
  actualCashCounted: number;
  variance: number;
  notes: string;
  status: "balanced" | "shortage" | "overage";
  createdAt: Date;
}

const CashReconciliationSchema = new Schema<ICashReconciliation>(
  {
    ownerId: { type: String, default: null, index: true },
    // Zone the shift was reconciled for ("" = all zones)
    venueName: { type: String, default: "", index: true },
    shiftDate: { type: String, required: true },
    shiftLabel: { type: String, default: "Evening Shift" },
    managerName: { type: String, required: true },
    expectedCashFromCoinBox: { type: Number, required: true },
    expectedRevenueFromStaffRfid: { type: Number, required: true },
    actualCashCounted: { type: Number, required: true },
    variance: { type: Number, required: true },
    notes: { type: String, default: "" },
    status: {
      type: String,
      enum: ["balanced", "shortage", "overage"],
      default: "balanced",
    },
  },
  { timestamps: true }
);

export interface IFirmwareRelease extends Document {
  version: string;
  targetHardware: string;
  fileName: string;
  downloadUrl: string;
  fileSizeBytes: number;
  releaseNotes: string;
  isDefault: boolean;
  isActive: boolean;
  uploadedBy: string;
  createdAt: Date;
  updatedAt: Date;
}

const FirmwareReleaseSchema = new Schema<IFirmwareRelease>(
  {
    version: { type: String, required: true, unique: true, index: true },
    targetHardware: { type: String, default: "esp32_devkit_v1" },
    fileName: { type: String, required: true },
    downloadUrl: { type: String, required: true },
    fileSizeBytes: { type: Number, default: 0 },
    releaseNotes: { type: String, default: "" },
    isDefault: { type: Boolean, default: false },
    isActive: { type: Boolean, default: true },
    uploadedBy: { type: String, default: "admin" },
  },
  { timestamps: true }
);

export interface IErrorLog extends Document {
  type:
    | "ota_failure"
    | "hardware_error"
    | "unauthorized_card"
    | "offline_drop"
    | "sync_error"
    | "client_exception";
  severity: "critical" | "warning" | "info";
  deviceId?: string;
  tableName?: string;
  venueName?: string;
  ownerEmail?: string;
  message: string;
  details?: string;
  resolved: boolean;
  resolvedAt?: Date | null;
  resolvedBy?: string;
  createdAt: Date;
  updatedAt: Date;
}

const ErrorLogSchema = new Schema<IErrorLog>(
  {
    type: {
      type: String,
      enum: [
        "ota_failure",
        "hardware_error",
        "unauthorized_card",
        "offline_drop",
        "sync_error",
        "client_exception",
      ],
      required: true,
      index: true,
    },
    severity: {
      type: String,
      enum: ["critical", "warning", "info"],
      default: "warning",
      index: true,
    },
    deviceId: { type: String, index: true },
    tableName: { type: String, default: "" },
    venueName: { type: String, default: "" },
    ownerEmail: { type: String, index: true },
    message: { type: String, required: true },
    details: { type: String, default: "" },
    resolved: { type: Boolean, default: false, index: true },
    resolvedAt: { type: Date, default: null },
    resolvedBy: { type: String, default: "" },
  },
  { timestamps: true }
);

/**
 * Returns a model, re-registering it in development so schema edits take effect
 * after hot reload (otherwise Mongoose keeps the stale cached schema and silently
 * strips any newly added fields on save/update).
 */
function getModel<T>(name: string, schema: Schema<T>): Model<T> {
  if (process.env.NODE_ENV !== "production" && mongoose.models[name]) {
    mongoose.deleteModel(name);
  }
  return (mongoose.models[name] as Model<T>) || mongoose.model<T>(name, schema);
}

export const User: Model<IUser> = getModel<IUser>("User", UserSchema);

export const TableDevice: Model<ITableDevice> = getModel<ITableDevice>(
  "TableDevice",
  TableDeviceSchema
);

export const AuthorizedCard: Model<IAuthorizedCard> = getModel<IAuthorizedCard>(
  "AuthorizedCard",
  AuthorizedCardSchema
);

export const RackEvent: Model<IRackEvent> = getModel<IRackEvent>(
  "RackEvent",
  RackEventSchema
);

export const CashReconciliation: Model<ICashReconciliation> =
  getModel<ICashReconciliation>("CashReconciliation", CashReconciliationSchema);

export const FirmwareRelease: Model<IFirmwareRelease> =
  getModel<IFirmwareRelease>("FirmwareRelease", FirmwareReleaseSchema);

export const ErrorLog: Model<IErrorLog> =
  getModel<IErrorLog>("ErrorLog", ErrorLogSchema);
