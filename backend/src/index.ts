import express from "express";
import cors from "cors";
import helmet from "helmet";
import rateLimit from "express-rate-limit";
import dotenv from "dotenv";
import { connectDatabase } from "./database/db";
import {
  authenticateDevice,
  optionalAuthenticateDevice,
} from "./middleware/auth";

// Load environment variables
dotenv.config();

const app = express();
const PORT = process.env.PORT || 3001;

// Security middleware
app.use(helmet());
app.use(
  cors({
    origin: process.env.CLIENT_URL || "http://localhost:3000",
    credentials: true,
  })
);

// Rate limiting
const limiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 100, // limit each IP to 100 requests per windowMs
});
app.use(limiter);

// JSON middleware
app.use(express.json({ limit: "10mb" }));
app.use(express.urlencoded({ extended: true }));

// Health check endpoint
app.get("/health", (req, res) => {
  res.json({
    status: "OK",
    timestamp: new Date().toISOString(),
    version: "1.0.0",
  });
});

// Device registration endpoint
app.post("/api/devices/register", async (req, res) => {
  try {
    const { deviceId, deviceName, deviceType } = req.body;

    if (!deviceId) {
      return res.status(400).json({
        error: "Missing device ID",
        message: "Device ID is required",
      });
    }

    // Register device and generate session token
    // Implementation will be added later
    res.status(200).json({
      message: "Device registration endpoint",
      deviceId,
      deviceName,
      deviceType,
    });
  } catch (error) {
    console.error("Device registration error:", error);
    res.status(500).json({
      error: "Registration failed",
      message: "Internal server error",
    });
  }
});

// Video catalog endpoint (optional authentication)
app.get("/api/videos", optionalAuthenticateDevice, async (req, res) => {
  try {
    // Return video catalog
    // Implementation will be added later
    res.status(200).json({
      message: "Video catalog endpoint",
      authenticated: !!req.deviceId,
    });
  } catch (error) {
    console.error("Video catalog error:", error);
    res.status(500).json({
      error: "Failed to fetch videos",
      message: "Internal server error",
    });
  }
});

// Video streaming endpoint (requires authentication)
app.get("/api/videos/:id/stream", authenticateDevice, async (req, res) => {
  try {
    const { id } = req.params;

    // Stream video
    // Implementation will be added later
    res.status(200).json({
      message: "Video streaming endpoint",
      videoId: id,
      deviceId: req.deviceId,
    });
  } catch (error) {
    console.error("Video streaming error:", error);
    res.status(500).json({
      error: "Streaming failed",
      message: "Internal server error",
    });
  }
});

// Watch progress endpoint
app.post("/api/watch-progress", authenticateDevice, async (req, res) => {
  try {
    const { videoId, progress, duration } = req.body;

    // Save watch progress
    // Implementation will be added later
    res.status(200).json({
      message: "Watch progress saved",
      videoId,
      progress,
      duration,
      deviceId: req.deviceId,
    });
  } catch (error) {
    console.error("Watch progress error:", error);
    res.status(500).json({
      error: "Failed to save progress",
      message: "Internal server error",
    });
  }
});

// Error handling middleware
app.use(
  (
    err: any,
    req: express.Request,
    res: express.Response,
    next: express.NextFunction
  ) => {
    console.error("Unhandled error:", err);
    res.status(500).json({
      error: "Internal server error",
      message: "Something went wrong",
    });
  }
);

// 404 handler
app.use("*", (req, res) => {
  res.status(404).json({
    error: "Not found",
    message: "The requested resource was not found",
  });
});

// Start server
async function startServer() {
  try {
    // Connect to database
    await connectDatabase();

    // Start HTTP server
    app.listen(PORT, () => {
      console.log(`Dorflix Backend Server running on port ${PORT}`);
      console.log(`Health check: http://localhost:${PORT}/health`);
    });
  } catch (error) {
    console.error("Failed to start server:", error);
    process.exit(1);
  }
}

startServer();
