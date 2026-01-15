import express from "express";
import cors from "cors";
import helmet from "helmet";
import rateLimit from "express-rate-limit";
import dotenv from "dotenv";
import { connectDatabase } from "./database/db";
import {
  authenticateDevice,
  optionalAuthenticateDevice,
  AuthRequest,
} from "./middleware/auth";
import { DeviceService } from "./services/deviceService";
import { VideoService } from "./services/videoService";
import { WatchProgressService } from "./services/watchProgressService";
import { SocialService } from "./services/socialService";
import { StreamingService } from "./services/streamingService";
import { videoUploadService } from "./services/videoUploadService";

// Load environment variables
dotenv.config();

const app = express();
const PORT = parseInt(process.env.PORT || "3001", 10);

// Security middleware
const allowed = process.env.CLIENT_URL || "*";
app.use(
  cors({
    origin: (origin, callback) => {
      if (!origin || process.env.NODE_ENV !== "production")
        return callback(null, true);
      if (allowed === "*" || allowed.split(",").includes(origin))
        return callback(null, true);
      callback(new Error("Not allowed by CORS"));
    },
    credentials: true,
    methods: ["GET", "POST", "PUT", "DELETE", "OPTIONS"],
    allowedHeaders: [
      "Content-Type",
      "Authorization",
      "Range",
      "Accept-Ranges",
      "Content-Range",
      "Content-Length",
      "Cache-Control",
      "ETag",
      "If-None-Match",
    ],
    exposedHeaders: [
      "Content-Range",
      "Accept-Ranges",
      "Content-Length",
      "Content-Type",
      "Cache-Control",
      "ETag",
    ],
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
app.get("/health", (req: express.Request, res: express.Response) => {
  res.json({
    status: "OK",
    timestamp: new Date().toISOString(),
    version: "1.0.0",
  });
});

// Device registration endpoint
app.post("/api/devices/register", async (req, res) => {
  try {
    const {
      deviceId,
      deviceName,
      deviceType,
      deviceModel,
      osVersion,
      appVersion,
    } = req.body;

    if (!deviceId) {
      return res.status(400).json({
        error: "Missing device ID",
        message: "Device ID is required",
      });
    }

    // Register device
    const device = await DeviceService.registerDevice({
      deviceId,
      deviceName,
      deviceType,
      deviceModel,
      osVersion,
      appVersion,
    });

    // Create session
    const session = await DeviceService.createSession(deviceId);

    res.status(200).json({
      message: "Device registered successfully",
      device: {
        id: device.id,
        deviceId: device.device_id,
        deviceName: device.device_name,
        deviceType: device.device_type,
      },
      session: {
        sessionToken: session.sessionToken,
        expiresAt: session.expiresAt,
      },
    });
  } catch (error) {
    console.error("Device registration error:", error);
    res.status(500).json({
      error: "Registration failed",
      message: error instanceof Error ? error.message : "Internal server error",
    });
  }
});

// Device session validation endpoint
app.post(
  "/api/devices/validate-session",
  authenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      res.status(200).json({
        message: "Session valid",
        deviceId: req.deviceId,
        sessionId: req.sessionId,
      });
    } catch (error) {
      console.error("Session validation error:", error);
      res.status(500).json({
        error: "Session validation failed",
        message: "Internal server error",
      });
    }
  }
);

// Video catalog endpoint (optional authentication)
app.get(
  "/api/videos",
  optionalAuthenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const {
        limit = 20,
        offset = 0,
        sortBy = "created_at",
        sortOrder = "desc",
      } = req.query;

      const params = {
        limit: parseInt(limit as string),
        offset: parseInt(offset as string),
        deviceId: req.deviceId,
        sortBy: sortBy as "created_at" | "views_count" | "likes_count",
        sortOrder: sortOrder as "asc" | "desc",
      };

      const result = await VideoService.getVideoCatalog(params);

      res.status(200).json({
        message: "Video catalog fetched successfully",
        videos: result.videos,
        total: result.total,
        hasMore: result.videos.length === params.limit,
      });
    } catch (error) {
      console.error("Video catalog error:", error);
      res.status(500).json({
        error: "Failed to fetch videos",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Video search endpoint
app.get(
  "/api/videos/search",
  optionalAuthenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const { q: searchTerm, limit = 20, offset = 0 } = req.query;

      if (!searchTerm) {
        return res.status(400).json({
          error: "Missing search term",
          message: "Search term is required",
        });
      }

      const params = {
        limit: parseInt(limit as string),
        offset: parseInt(offset as string),
        deviceId: req.deviceId,
      };

      const result = await VideoService.searchVideos(
        searchTerm as string,
        params
      );

      res.status(200).json({
        message: "Search completed successfully",
        searchTerm,
        videos: result.videos,
        total: result.total,
        hasMore: result.videos.length === params.limit,
      });
    } catch (error) {
      console.error("Video search error:", error);
      res.status(500).json({
        error: "Failed to search videos",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Video details endpoint
app.get(
  "/api/videos/:id",
  optionalAuthenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const { id } = req.params;

      console.log(`=== GET VIDEO BY ID: ${id} ===`);
      console.log(`Device ID: ${req.deviceId || "unauthenticated"}`);

      const video = await VideoService.getVideoById(id, req.deviceId);

      console.log(`Video service returned:`, video);

      if (!video) {
        console.log(`Video not found for ID: ${id}`);
        return res.status(404).json({
          error: "Video not found",
          message: "The requested video does not exist",
        });
      }

      console.log(
        `Video object before serialization:`,
        JSON.stringify(video, null, 2)
      );
      console.log(`Video URL field: '${video.video_url}'`);
      console.log(`Video ID field: '${video.id}'`);

      // Check if the video object has any properties
      console.log(`Video object keys:`, Object.keys(video));
      console.log(
        `Video object has video_url property:`,
        video.hasOwnProperty("video_url")
      );
      console.log(`Video object has id property:`, video.hasOwnProperty("id"));

      const responseData = {
        message: "Video details fetched successfully",
        video,
      };

      console.log(`Response data:`, JSON.stringify(responseData, null, 2));

      // Double-check the response data structure
      console.log(`Response data keys:`, Object.keys(responseData));
      console.log(`Response data.video keys:`, Object.keys(responseData.video));

      // Final check right before sending response
      console.log(`=== FINAL RESPONSE CHECK ===`);
      console.log(
        `About to call res.json() with:`,
        JSON.stringify(responseData, null, 2)
      );
      console.log(`Response data.video.id:`, responseData.video.id);
      console.log(
        `Response data.video.video_url:`,
        responseData.video.video_url
      );

      // Override res.json to capture what gets sent
      const originalJson = res.json;
      res.json = function (data) {
        console.log(`=== RES.JSON CALLED WITH ===`);
        console.log(`Data being sent:`, JSON.stringify(data, null, 2));
        return originalJson.call(this, data);
      };

      res.status(200).json(responseData);
    } catch (error) {
      console.error("Video details error:", error);
      res.status(500).json({
        error: "Failed to fetch video details",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Video streaming endpoint (requires authentication)
app.get(
  "/api/videos/:id/stream",
  authenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const { id } = req.params;

      // Get video metadata first
      const video = await StreamingService.getVideoMetadata(id);

      if (!video) {
        return res.status(404).json({
          error: "Video not found",
          message: "The requested video does not exist",
        });
      }

      // Use the StreamingService to handle the actual streaming
      await StreamingService.streamVideo(req, res, id, req.deviceId!);
    } catch (error) {
      console.error("Video streaming error:", error);
      res.status(500).json({
        error: "Streaming failed",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Watch progress endpoint
app.post(
  "/api/watch-progress",
  authenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const { videoId, progressSeconds, totalDuration } = req.body;

      if (!videoId || progressSeconds == null || totalDuration == null) {
        return res.status(400).json({
          error: "Missing required fields",
          message: "videoId, progressSeconds, and totalDuration are required",
        });
      }

      const progress = await WatchProgressService.saveWatchProgress({
        videoId,
        deviceId: req.deviceId!,
        progressSeconds,
        totalDuration,
        percentageWatched: Math.round((progressSeconds / totalDuration) * 100),
        isCompleted: progressSeconds >= totalDuration * 0.95,
      });

      res.status(200).json({
        message: "Watch progress saved successfully",
        progress: {
          videoId: progress.video_id,
          progressSeconds: progress.progress_seconds,
          totalDuration: progress.total_duration,
          percentageWatched: progress.percentage_watched,
          isCompleted: progress.is_completed,
          lastWatchedAt: progress.last_watched_at,
        },
      });
    } catch (error) {
      console.error("Watch progress error:", error);
      res.status(500).json({
        error: "Failed to save progress",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Get watch progress for a video
app.get(
  "/api/watch-progress/:videoId",
  authenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const { videoId } = req.params;

      const progress = await WatchProgressService.getWatchProgress(
        videoId,
        req.deviceId!
      );

      res.status(200).json({
        message: "Watch progress fetched successfully",
        progress: progress || null,
      });
    } catch (error) {
      console.error("Get watch progress error:", error);
      res.status(500).json({
        error: "Failed to fetch progress",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Get continue watching videos
app.get(
  "/api/continue-watching",
  authenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const { limit = 10 } = req.query;

      const videos = await WatchProgressService.getContinueWatching(
        req.deviceId!,
        parseInt(limit as string)
      );

      res.status(200).json({
        message: "Continue watching videos fetched successfully",
        videos,
      });
    } catch (error) {
      console.error("Continue watching error:", error);
      res.status(500).json({
        error: "Failed to fetch continue watching videos",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Like video endpoint
app.post(
  "/api/videos/:id/like",
  authenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const { id } = req.params;

      const like = await SocialService.likeVideo({
        videoId: id,
        deviceId: req.deviceId!,
      });

      res.status(200).json({
        message: "Video liked successfully",
        like: {
          id: like.id,
          videoId: like.video_id,
          deviceId: like.device_id,
          createdAt: like.created_at,
        },
      });
    } catch (error) {
      console.error("Like video error:", error);
      res.status(500).json({
        error: "Failed to like video",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Unlike video endpoint
app.delete(
  "/api/videos/:id/like",
  authenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const { id } = req.params;

      await SocialService.unlikeVideo({
        videoId: id,
        deviceId: req.deviceId!,
      });

      res.status(200).json({
        message: "Video unliked successfully",
      });
    } catch (error) {
      console.error("Unlike video error:", error);
      res.status(500).json({
        error: "Failed to unlike video",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Check if video is liked
app.get(
  "/api/videos/:id/is-liked",
  authenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const { id } = req.params;

      const isLiked = await SocialService.isVideoLiked(id, req.deviceId!);

      res.status(200).json({
        message: "Like status checked successfully",
        isLiked,
      });
    } catch (error) {
      console.error("Check like status error:", error);
      res.status(500).json({
        error: "Failed to check like status",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Share video endpoint
app.post(
  "/api/videos/:id/share",
  authenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const { id } = req.params;
      const { sharePlatform } = req.body;

      const share = await SocialService.shareVideo({
        videoId: id,
        deviceId: req.deviceId!,
        sharePlatform,
      });

      res.status(200).json({
        message: "Video shared successfully",
        share: {
          id: share.id,
          videoId: share.video_id,
          deviceId: share.device_id,
          sharePlatform: share.share_platform,
          createdAt: share.created_at,
        },
      });
    } catch (error) {
      console.error("Share video error:", error);
      res.status(500).json({
        error: "Failed to share video",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Get video comments
app.get(
  "/api/videos/:id/comments",
  optionalAuthenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const { id } = req.params;
      const { limit = 20, offset = 0 } = req.query;

      const result = await VideoService.getComments(
        id,
        parseInt(limit as string),
        parseInt(offset as string)
      );

      res.status(200).json({
        message: "Comments fetched successfully",
        comments: result.comments,
        total: result.total,
        hasMore: result.comments.length === parseInt(limit as string),
      });
    } catch (error) {
      console.error("Get comments error:", error);
      res.status(500).json({
        error: "Failed to fetch comments",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Add comment to video
app.post(
  "/api/videos/:id/comments",
  authenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const { id } = req.params;
      const { commentText } = req.body;

      if (!commentText || commentText.trim().length === 0) {
        return res.status(400).json({
          error: "Missing comment text",
          message: "Comment text is required",
        });
      }

      const comment = await VideoService.addComment(
        id,
        req.deviceId!,
        commentText
      );

      res.status(200).json({
        message: "Comment added successfully",
        comment: {
          id: comment.id,
          videoId: comment.video_id,
          deviceId: comment.device_id,
          commentText: comment.comment_text,
          createdAt: comment.created_at,
        },
      });
    } catch (error) {
      console.error("Add comment error:", error);
      res.status(500).json({
        error: "Failed to add comment",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Delete comment
app.delete(
  "/api/comments/:id",
  authenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const { id } = req.params;

      await VideoService.deleteComment(id, req.deviceId!);

      res.status(200).json({
        message: "Comment deleted successfully",
      });
    } catch (error) {
      console.error("Delete comment error:", error);
      res.status(500).json({
        error: "Failed to delete comment",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Get video recommendations
app.get(
  "/api/recommendations",
  authenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const { limit = 10 } = req.query;

      const videos = await StreamingService.getRecommendations(
        req.deviceId!,
        parseInt(limit as string)
      );

      res.status(200).json({
        message: "Recommendations fetched successfully",
        videos,
      });
    } catch (error) {
      console.error("Get recommendations error:", error);
      res.status(500).json({
        error: "Failed to get recommendations",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Get device watch statistics
app.get(
  "/api/stats/watch",
  authenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const stats = await WatchProgressService.getWatchStatistics(
        req.deviceId!
      );

      res.status(200).json({
        message: "Watch statistics fetched successfully",
        stats,
      });
    } catch (error) {
      console.error("Get watch statistics error:", error);
      res.status(500).json({
        error: "Failed to get watch statistics",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Get device social activity
app.get(
  "/api/stats/social",
  authenticateDevice,
  async (req: AuthRequest, res: express.Response) => {
    try {
      const activity = await SocialService.getDeviceSocialActivity(
        req.deviceId!
      );

      res.status(200).json({
        message: "Social activity fetched successfully",
        activity,
      });
    } catch (error) {
      console.error("Get social activity error:", error);
      res.status(500).json({
        error: "Failed to get social activity",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Video upload endpoints (no authentication required for admin panel)
// Get Cloudinary configuration for frontend
app.get("/api/upload/config", (req, res) => {
  try {
    const config = videoUploadService.getCloudinaryConfig();
    res.status(200).json({
      message: "Cloudinary configuration fetched successfully",
      config,
    });
  } catch (error) {
    console.error("Get Cloudinary config error:", error);
    res.status(500).json({
      error: "Failed to get Cloudinary configuration",
      message: error instanceof Error ? error.message : "Internal server error",
    });
  }
});

// Upload video endpoint
app.post(
  "/api/upload/video",
  videoUploadService.getUploadMiddleware(),
  async (req, res) => {
    try {
      const { title, description, uploaderName, uploaderAvatarUrl } = req.body;

      if (!req.file) {
        return res.status(400).json({
          error: "No file uploaded",
          message: "Please upload a video file",
        });
      }

      if (!title || !uploaderName) {
        return res.status(400).json({
          error: "Missing required fields",
          message: "Title and uploader name are required",
        });
      }

      const uploadData = {
        title,
        description,
        uploaderName,
        uploaderAvatarUrl,
      };

      const result = await videoUploadService.uploadVideo(req.file, uploadData);

      res.status(200).json({
        message: "Video uploaded successfully",
        video: result.video,
        uploadResult: result.uploadResult,
      });
    } catch (error) {
      console.error("Video upload error:", error);
      res.status(500).json({
        error: "Failed to upload video",
        message:
          error instanceof Error ? error.message : "Internal server error",
      });
    }
  }
);

// Delete video endpoint
app.delete("/api/upload/video/:videoId", async (req, res) => {
  try {
    const { videoId } = req.params;

    await videoUploadService.deleteVideo(videoId);

    res.status(200).json({
      message: "Video deleted successfully",
    });
  } catch (error) {
    console.error("Delete video error:", error);
    res.status(500).json({
      error: "Failed to delete video",
      message: error instanceof Error ? error.message : "Internal server error",
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

    // Start HTTP server and bind to HOST (default 0.0.0.0 so it accepts external connections)
    const HOST = process.env.HOST || "0.0.0.0";
    if (HOST === "127.0.0.1" || HOST === "localhost") {
      console.warn(
        `⚠️  HOST is set to ${HOST} — the server will only accept localhost connections.`
      );
    }
    const server = app.listen(PORT, HOST, () => {
      const addr: any = server.address();
      const bound =
        typeof addr === "string" ? addr : `${addr?.address}:${addr?.port}`;
      console.log(`Dorflix Backend Server running on ${bound}`);
      console.log(`Health check: http://${bound}/health`);
      console.log(`API endpoints available at: http://${bound}/api`);
    });
  } catch (error) {
    console.error("Failed to start server:", error);
    process.exit(1);
  }
}

startServer();
