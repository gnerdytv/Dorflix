"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.DeviceService = void 0;
const db_1 = require("../database/db");
const auth_1 = require("../middleware/auth");
class DeviceService {
    /**
     * Register a new device or update existing device information
     */
    static async registerDevice(data) {
        try {
            // Sanitize/truncate inputs to match DB column limits (avoid insert errors)
            const sanitized = {
                deviceId: data.deviceId?.substring(0, 255),
                deviceType: data.deviceType ? data.deviceType.substring(0, 50) : null,
                deviceModel: data.deviceModel
                    ? data.deviceModel.substring(0, 100)
                    : null,
                osVersion: data.osVersion ? data.osVersion.substring(0, 50) : null,
                appVersion: data.appVersion ? data.appVersion.substring(0, 50) : null,
            };
            if (data.osVersion && data.osVersion.length > 50) {
                console.warn(`Truncating osVersion for device ${data.deviceId} to 50 chars`);
            }
            // Check if device already exists
            const existingDeviceResult = await (0, db_1.query)("SELECT * FROM devices WHERE device_id = $1", [sanitized.deviceId]);
            let device;
            if (existingDeviceResult.rows.length > 0) {
                // Update existing device
                const updateResult = await (0, db_1.query)(`UPDATE devices 
           SET device_type = COALESCE($2, device_type),
               device_model = COALESCE($3, device_model),
               os_version = COALESCE($4, os_version),
               app_version = COALESCE($5, app_version),
               updated_at = NOW()
           WHERE device_id = $1
           RETURNING *`, [
                    sanitized.deviceId,
                    sanitized.deviceType,
                    sanitized.deviceModel,
                    sanitized.osVersion,
                    sanitized.appVersion,
                ]);
                device = updateResult.rows[0];
            }
            else {
                // Create new device
                const insertResult = await (0, db_1.query)(`INSERT INTO devices (device_id, device_type, device_model, os_version, app_version)
           VALUES ($1, $2, $3, $4, $5)
           RETURNING *`, [
                    sanitized.deviceId,
                    sanitized.deviceType,
                    sanitized.deviceModel,
                    sanitized.osVersion,
                    sanitized.appVersion,
                ]);
                device = insertResult.rows[0];
            }
            return device;
        }
        catch (error) {
            console.error("Device registration error:", error);
            throw new Error("Failed to register device");
        }
    }
    /**
     * Create a new device session
     */
    static async createSession(deviceId) {
        try {
            // Get the device UUID from the device_id string
            const deviceResult = await (0, db_1.query)("SELECT id FROM devices WHERE device_id = $1", [deviceId]);
            if (deviceResult.rows.length === 0) {
                throw new Error("Device not found");
            }
            const deviceUUID = deviceResult.rows[0].id;
            // Invalidate any existing active sessions for this device
            await (0, db_1.query)("UPDATE device_sessions SET is_active = false WHERE device_id = $1 AND is_active = true", [deviceUUID]);
            // Create new session
            const sessionToken = (0, auth_1.generateSessionToken)(deviceId);
            const expiresAt = new Date();
            expiresAt.setDate(expiresAt.getDate() + 7); // 7 days expiry
            const insertResult = await (0, db_1.query)(`INSERT INTO device_sessions (device_id, session_token, expires_at)
         VALUES ($1, $2, $3)
         RETURNING *`, [deviceUUID, sessionToken, expiresAt]);
            const session = insertResult.rows[0];
            return {
                deviceId: session.device_id,
                sessionToken,
                expiresAt: session.expires_at,
            };
        }
        catch (error) {
            console.error("Session creation error:", error);
            throw new Error("Failed to create session");
        }
    }
    /**
     * Get device by ID
     */
    static async getDeviceById(deviceId) {
        try {
            const result = await (0, db_1.query)("SELECT * FROM devices WHERE device_id = $1", [
                deviceId,
            ]);
            return result.rows.length > 0 ? result.rows[0] : null;
        }
        catch (error) {
            console.error("Get device error:", error);
            throw new Error("Failed to get device");
        }
    }
    /**
     * Get active session by token
     */
    static async getActiveSessionByToken(sessionToken) {
        try {
            const result = await (0, db_1.query)(`SELECT ds.*, d.device_id, d.device_type
         FROM device_sessions ds
         JOIN devices d ON ds.device_id = d.id
         WHERE ds.session_token = $1 
         AND ds.is_active = true 
         AND ds.expires_at > NOW()`, [sessionToken]);
            return result.rows.length > 0 ? result.rows[0] : null;
        }
        catch (error) {
            console.error("Get session error:", error);
            throw new Error("Failed to get session");
        }
    }
    /**
     * Invalidate a session
     */
    static async invalidateSession(sessionToken) {
        try {
            await (0, db_1.query)("UPDATE device_sessions SET is_active = false WHERE session_token = $1", [sessionToken]);
        }
        catch (error) {
            console.error("Session invalidation error:", error);
            throw new Error("Failed to invalidate session");
        }
    }
    /**
     * Clean up expired sessions
     */
    static async cleanupExpiredSessions() {
        try {
            await (0, db_1.query)("UPDATE device_sessions SET is_active = false WHERE expires_at <= NOW()");
        }
        catch (error) {
            console.error("Session cleanup error:", error);
            throw new Error("Failed to cleanup sessions");
        }
    }
}
exports.DeviceService = DeviceService;
//# sourceMappingURL=deviceService.js.map