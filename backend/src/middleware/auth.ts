import jwt from "jsonwebtoken";
import { Request, Response, NextFunction } from "express";
import { query } from "../database/db";

export interface AuthRequest extends Request {
  deviceId?: string;
  sessionId?: string;
}

// Generate device session token
export const generateSessionToken = (deviceId: string): string => {
  const payload = {
    deviceId,
    type: "device",
  };

  return jwt.sign(payload, process.env.JWT_SECRET!, {
    expiresIn: process.env.JWT_EXPIRES_IN || "7d",
  });
};

// Verify device session token
export const verifySessionToken = async (
  token: string
): Promise<{ deviceId: string; sessionId: string } | null> => {
  try {
    const decoded = jwt.verify(token, process.env.JWT_SECRET!) as {
      deviceId: string;
      type: string;
    };

    if (decoded.type !== "device") {
      return null;
    }

    // Check if session exists and is active
    const sessionResult = await query(
      `SELECT ds.id, ds.device_id, ds.expires_at, ds.is_active 
       FROM device_sessions ds 
       WHERE ds.session_token = $1 AND ds.is_active = true 
       AND ds.expires_at > NOW()`,
      [token]
    );

    if (sessionResult.rows.length === 0) {
      return null;
    }

    const session = sessionResult.rows[0];
    return {
      deviceId: session.device_id,
      sessionId: session.id,
    };
  } catch (error) {
    console.error("Token verification failed:", error);
    return null;
  }
};

// Authentication middleware
export const authenticateDevice = async (
  req: AuthRequest,
  res: Response,
  next: NextFunction
) => {
  try {
    const authHeader = req.headers.authorization;

    if (!authHeader || !authHeader.startsWith("Bearer ")) {
      return res.status(401).json({
        error: "Authentication required",
        message: "Please provide a valid session token",
      });
    }

    const token = authHeader.substring(7);
    const sessionData = await verifySessionToken(token);

    if (!sessionData) {
      return res.status(401).json({
        error: "Invalid token",
        message: "Session token is invalid or expired",
      });
    }

    req.deviceId = sessionData.deviceId;
    req.sessionId = sessionData.sessionId;
    next();
  } catch (error) {
    console.error("Authentication error:", error);
    return res.status(500).json({
      error: "Authentication failed",
      message: "Internal server error during authentication",
    });
  }
};

// Optional authentication middleware (doesn't fail if no token provided)
export const optionalAuthenticateDevice = async (
  req: AuthRequest,
  res: Response,
  next: NextFunction
) => {
  try {
    const authHeader = req.headers.authorization;

    if (authHeader && authHeader.startsWith("Bearer ")) {
      const token = authHeader.substring(7);
      const sessionData = await verifySessionToken(token);

      if (sessionData) {
        req.deviceId = sessionData.deviceId;
        req.sessionId = sessionData.sessionId;
      }
    }

    next();
  } catch (error) {
    console.error("Optional authentication error:", error);
    next();
  }
};
