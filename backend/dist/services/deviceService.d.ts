import { Device, DeviceSession } from "../database/db";
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
export declare class DeviceService {
    /**
     * Register a new device or update existing device information
     */
    static registerDevice(data: DeviceRegistrationData): Promise<Device>;
    /**
     * Create a new device session
     */
    static createSession(deviceId: string): Promise<DeviceSessionData>;
    /**
     * Get device by ID
     */
    static getDeviceById(deviceId: string): Promise<Device | null>;
    /**
     * Get active session by token
     */
    static getActiveSessionByToken(sessionToken: string): Promise<DeviceSession | null>;
    /**
     * Invalidate a session
     */
    static invalidateSession(sessionToken: string): Promise<void>;
    /**
     * Clean up expired sessions
     */
    static cleanupExpiredSessions(): Promise<void>;
}
//# sourceMappingURL=deviceService.d.ts.map