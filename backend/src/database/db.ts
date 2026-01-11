import { Pool } from "pg";
import dotenv from "dotenv";

dotenv.config();

// Database connection configuration
const pool = new Pool({
  connectionString:
    process.env.DATABASE_URL ||
    `postgresql://${process.env.DB_USER}:${process.env.DB_PASSWORD}@${process.env.DB_HOST}:${process.env.DB_PORT}/${process.env.DB_NAME}`,
  ssl:
    process.env.NODE_ENV === "production"
      ? { rejectUnauthorized: false }
      : false,
});

// Test database connection
export const testConnection = async () => {
  try {
    const client = await pool.connect();
    console.log("✅ Database connected successfully");
    client.release();
    return true;
  } catch (error) {
    console.error("❌ Database connection failed:", error);
    return false;
  }
};

// Database query helper
export const query = async (text: string, params?: any[]) => {
  const start = Date.now();
  try {
    const res = await pool.query(text, params);
    const duration = Date.now() - start;
    console.log("✅ Executed query", { text, duration, rows: res.rowCount });
    return res;
  } catch (error) {
    console.error("❌ Query error:", { text, params, error });
    throw error;
  }
};

// Database types
export interface Device {
  id: string;
  device_id: string;
  device_type?: string;
  device_model?: string;
  os_version?: string;
  app_version?: string;
  created_at: Date;
  updated_at: Date;
}

export interface DeviceSession {
  id: string;
  device_id: string;
  session_token: string;
  expires_at: Date;
  is_active: boolean;
  created_at: Date;
  updated_at: Date;
}

export interface Video {
  id: string;
  title: string;
  description?: string;
  video_url: string;
  thumbnail_url?: string;
  duration: number;
  uploader_name: string;
  uploader_avatar_url?: string;
  likes_count: number;
  comments_count: number;
  shares_count: number;
  views_count: number;
  is_active: boolean;
  created_at: Date;
  updated_at: Date;
}

export interface WatchProgress {
  id: string;
  video_id: string;
  device_id: string;
  progress_seconds: number;
  total_duration: number;
  percentage_watched: number;
  is_completed: boolean;
  last_watched_at: Date;
  created_at: Date;
  updated_at: Date;
}

export interface VideoView {
  id: string;
  video_id: string;
  device_id: string;
  view_duration: number;
  is_qualified_view: boolean;
  created_at: Date;
}

export interface VideoLike {
  id: string;
  video_id: string;
  device_id: string;
  created_at: Date;
}

export interface VideoComment {
  id: string;
  video_id: string;
  device_id: string;
  comment_text: string;
  is_active: boolean;
  created_at: Date;
  updated_at: Date;
}

export interface VideoShare {
  id: string;
  video_id: string;
  device_id: string;
  share_platform?: string;
  created_at: Date;
}

export default pool;
