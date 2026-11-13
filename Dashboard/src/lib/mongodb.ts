import dns from "node:dns";
import mongoose from "mongoose";

// Resolve MongoDB Atlas SRV & TXT records using public DNS servers (8.8.8.8, 1.1.1.1)
// to prevent ESERVFAIL errors caused by local router / ISP DNS blocking TXT records
try {
  dns.setServers(["8.8.8.8", "8.8.4.4", "1.1.1.1"]);
} catch {
  // Ignore in restricted environments
}

const MONGODB_URI =
  process.env.MONGODB_URI ||
  "mongodb+srv://smarthivesms_db_user:2PVPlj8IZCo6WvHE@cluster0.1juirmo.mongodb.net/smarthivepool?retryWrites=true&w=majority";

interface MongooseCache {
  conn: typeof mongoose | null;
  promise: Promise<typeof mongoose> | null;
}

declare global {
  // eslint-disable-next-line no-var
  var mongooseCache: MongooseCache | undefined;
}

const cached: MongooseCache = global.mongooseCache || {
  conn: null,
  promise: null,
};

if (!global.mongooseCache) {
  global.mongooseCache = cached;
}

export async function connectDB(): Promise<typeof mongoose> {
  if (cached.conn) {
    return cached.conn;
  }

  if (!cached.promise) {
    cached.promise = mongoose
      .connect(MONGODB_URI, {
        bufferCommands: false,
        serverSelectionTimeoutMS: 8000,
      })
      .then((m) => m);
  }

  try {
    cached.conn = await cached.promise;
  } catch (err) {
    cached.promise = null;
    throw err;
  }

  return cached.conn;
}

