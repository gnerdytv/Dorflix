import { query, Device, DeviceSession } from "../database/db";
import { generateSessionToken } from "../middleware/auth";

export interface DeviceRegistrationData {
  deviceId: string;
  deviceName?: string;
  deviceType?: string;
  deviceModel?: string;
  osVersion?: string;
  appVersion?: string;
}

export interface DeviceSessionData {
  deviceId: string;
  sessionToken: string;
  expiresAt: Date;
}

export class DeviceService {
  /**
   * Register a new device or update existing device information
   */
  static async registerDevice(data: DeviceRegistrationData): Promise<Device> {
    try {
      // Check if device already exists
      const existingDeviceResult = await query(
        "SELECT * FROM devices WHERE device_id = $1",
        [data.deviceId]
      );

      let device: Device;

      if (existingDeviceResult.rows.length > 0) {
        // Update existing device
        const updateResult = await query(
          `UPDATE devices 
           SET device_name = COALESCE($2, device_name),
               device_type = COALESCE($3, device_type),
               device_model = COALESCE($4, device_model),
               os_version = COALESCE($5, os_version),
               app_version = COALESCE($6, app_version),
               updated_at = NOW()
           WHERE device_id = $1
           RETURNING *`,
          [
            data.deviceId,
            data.deviceName,
            data.deviceType,
            data.deviceModel,
            data.osVersion,
            data.appVersion,
          ]
        );
        device = updateResult.rows[0];
      } else {
        // Create new device
        const insertResult = await query(
          `INSERT INTO devices (device_id, device_name, device_type, device_model, os_version, app_version)
           VALUES ($1, $2, $3, $4, $5, $6)
           RETURNING *`,
          [
            data.deviceId,
            data.deviceName,
            data.deviceType,
            data.deviceModel,
            data.osVersion,
            data.appVersion,
          ]
        );
        device = insertResult.rows[0];
      }

      return device;
    } catch (error) {
      console.error("Device registration error:", error);
      throw new Error("Failed to register device");
    }
  }

  /**
   * Create a new device session
   */
  static async createSession(deviceId: string): Promise<DeviceSessionData> {
    try {
      // Invalidate any existing active sessions for this device
      await query(
        "UPDATE device_sessions SET is_active = false WHERE device_id = $1 AND is_active = true",
        [deviceId]
      );

      // Create new session
      const sessionToken = generateSessionToken(deviceId);
      const expiresAt = new Date();
      expiresAt.setDate(expiresAt.getDate() + 7); // 7 days expiry

      const insertResult = await query(
        `INSERT INTO device_sessions (device_id, session_token, expires_at)
         VALUES ($1, $2, $3)
         RETURNING *`,
        [deviceId, sessionToken, expiresAt]
      );

      const session = insertResult.rows[0];

      return {
        deviceId: session.device_id,
        sessionToken,
        expiresAt: session.expires_at,
      };
    } catch (error) {
      console.error("Session creation error:", error);
      throw new Error("Failed to create session");
    }
  }

  /**
   * Get device by ID
   */
  static async getDeviceById(deviceId: string): Promise<Device | null> {
    try {
      const result = await query("SELECT * FROM devices WHERE device_id = $1", [
        deviceId,
      ]);
      return result.rows.length > 0 ? result.rows[0] : null;
    } catch (error) {
      console.error("Get device error:", error);
      throw new Error("Failed to get device");
    }
  }

  /**
   * Get active session by token
   */
  static async getActiveSessionByToken(
    sessionToken: string
  ): Promise<DeviceSession | null> {
    try {
      const result = await query(
        `SELECT ds.*, d.device_id, d.device_name, d.device_type
         FROM device_sessions ds
         JOIN devices d ON ds.device_id = d.id
         WHERE ds.session_token = $1 
         AND ds.is_active = true 
         AND ds.expires_at > NOW()`,
        [sessionToken]
      );
      return result.rows.length > 0 ? result.rows[0] : null;
    } catch (error) {
      console.error("Get session error:", error);
      throw new Error("Failed to get session");
    }
  }

  /**
   * Invalidate a session
   */
  static async invalidateSession(sessionToken: string): Promise<void> {
    try {
      await query(
        "UPDATE device_sessions SET is_active = false WHERE session_token = $1",
        [sessionToken]
      );
    } catch (error) {
      console.error("Session invalidation error:", error);
      throw new Error("Failed to invalidate session");
    }
  }

  /**
   * Clean up expired sessions
   */
  static async cleanupExpiredSessions(): Promise<void> {
    try {
      await query(
        "UPDATE device_sessions SET is_active = false WHERE expires_at <= NOW()"
      );
    } catch (error) {
      console.error("Session cleanup error:", error);
      throw new Error("Failed to cleanup sessions");
    }
  }
}
