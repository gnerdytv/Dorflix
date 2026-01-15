"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.optionalAuthenticateDevice = exports.authenticateDevice = exports.verifySessionToken = exports.generateSessionToken = void 0;
const jsonwebtoken_1 = __importDefault(require("jsonwebtoken"));
const db_1 = require("../database/db");
// Generate device session token
const generateSessionToken = (deviceId) => {
    const payload = {
        deviceId,
        type: "device",
    };
    return jsonwebtoken_1.default.sign(payload, process.env.JWT_SECRET, {
        expiresIn: process.env.JWT_EXPIRES_IN || "7d",
    });
};
exports.generateSessionToken = generateSessionToken;
// Verify device session token
const verifySessionToken = async (token) => {
    try {
        const decoded = jsonwebtoken_1.default.verify(token, process.env.JWT_SECRET);
        if (decoded.type !== "device") {
            return null;
        }
        // Check if session exists and is active
        const sessionResult = await (0, db_1.query)(`SELECT ds.id, ds.device_id, ds.expires_at, ds.is_active 
       FROM device_sessions ds 
       WHERE ds.session_token = $1 AND ds.is_active = true 
       AND ds.expires_at > NOW()`, [token]);
        if (sessionResult.rows.length === 0) {
            return null;
        }
        const session = sessionResult.rows[0];
        return {
            deviceId: session.device_id,
            sessionId: session.id,
        };
    }
    catch (error) {
        console.error("Token verification failed:", error);
        return null;
    }
};
exports.verifySessionToken = verifySessionToken;
// Authentication middleware
const authenticateDevice = async (req, res, next) => {
    try {
        const authHeader = req.headers.authorization;
        if (!authHeader || !authHeader.startsWith("Bearer ")) {
            return res.status(401).json({
                error: "Authentication required",
                message: "Please provide a valid session token",
            });
        }
        const token = authHeader.substring(7);
        const sessionData = await (0, exports.verifySessionToken)(token);
        if (!sessionData) {
            return res.status(401).json({
                error: "Invalid token",
                message: "Session token is invalid or expired",
            });
        }
        req.deviceId = sessionData.deviceId;
        req.sessionId = sessionData.sessionId;
        next();
    }
    catch (error) {
        console.error("Authentication error:", error);
        return res.status(500).json({
            error: "Authentication failed",
            message: "Internal server error during authentication",
        });
    }
};
exports.authenticateDevice = authenticateDevice;
// Optional authentication middleware (doesn't fail if no token provided)
const optionalAuthenticateDevice = async (req, res, next) => {
    try {
        const authHeader = req.headers.authorization;
        if (authHeader && authHeader.startsWith("Bearer ")) {
            const token = authHeader.substring(7);
            const sessionData = await (0, exports.verifySessionToken)(token);
            if (sessionData) {
                req.deviceId = sessionData.deviceId;
                req.sessionId = sessionData.sessionId;
            }
        }
        next();
    }
    catch (error) {
        console.error("Optional authentication error:", error);
        next();
    }
};
exports.optionalAuthenticateDevice = optionalAuthenticateDevice;
//# sourceMappingURL=auth.js.map