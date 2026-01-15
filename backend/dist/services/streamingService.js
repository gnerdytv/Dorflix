"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.StreamingService = void 0;
const db_1 = require("../database/db");
const fs = __importStar(require("fs"));
const path = __importStar(require("path"));
const axios_1 = __importDefault(require("axios"));
class StreamingService {
    /**
     * Get video stream with proper headers and range support
     */
    static async streamVideo(req, res, videoId, deviceId) {
        try {
            // Get video metadata
            const videoResult = await (0, db_1.query)("SELECT * FROM videos WHERE id = $1 AND is_active = true", [videoId]);
            if (videoResult.rows.length === 0) {
                res.status(404).json({ error: "Video not found" });
                return;
            }
            const video = videoResult.rows[0];
            // Record view
            await this.recordView(videoId, deviceId, 0);
            // Determine if we should stream from Cloudinary or local file
            const isCloudinaryUrl = video.video_url.includes("cloudinary.com");
            if (isCloudinaryUrl) {
                await this.streamFromCloudinary(req, res, video, deviceId);
            }
            else {
                await this.streamFromLocalFile(req, res, video, deviceId);
            }
        }
        catch (error) {
            console.error("Stream video error:", error);
            if (!res.headersSent) {
                res.status(500).json({ error: "Failed to stream video" });
            }
        }
    }
    /**
     * Stream video from Cloudinary with range request support
     */
    static async streamFromCloudinary(req, res, video, deviceId) {
        try {
            const range = req.headers.range;
            const quality = this.getQualityFromRequest(req);
            // Generate Cloudinary URL with transformations
            const cloudinaryUrl = this.generateCloudinaryUrl(video.video_url, quality);
            // Get video info from Cloudinary
            const videoInfo = await this.getCloudinaryVideoInfo(cloudinaryUrl);
            if (!videoInfo) {
                res
                    .status(500)
                    .json({ error: "Failed to get video info from Cloudinary" });
                return;
            }
            // Handle range requests
            const { start, end, contentLength, statusCode } = this.parseRangeRequest(range, videoInfo.size, 10 * 1024 * 1024 // 10MB chunk size for Cloudinary
            );
            const headers = {
                "Content-Range": `bytes ${start}-${end}/${videoInfo.size}`,
                "Accept-Ranges": "bytes",
                "Content-Length": contentLength,
                "Content-Type": "video/mp4",
                "Cache-Control": "public, max-age=31536000",
                ETag: `"${video.id}-${video.updated_at.getTime()}"`,
                "X-Source": "cloudinary",
            };
            res.status(statusCode).set(headers);
            // Stream from Cloudinary
            const cloudinaryResponse = await (0, axios_1.default)({
                method: "GET",
                url: cloudinaryUrl,
                responseType: "stream",
                headers: {
                    Range: `bytes=${start}-${end}`,
                    "User-Agent": req.headers["user-agent"] || "Dorflix/1.0",
                },
                timeout: 30000,
            });
            cloudinaryResponse.data.pipe(res);
            // Track streaming session
            this.trackStreamingSession(video.id, deviceId, start, contentLength);
        }
        catch (error) {
            console.error("Stream from Cloudinary error:", error);
            if (!res.headersSent) {
                res.status(500).json({ error: "Failed to stream from Cloudinary" });
            }
        }
    }
    /**
     * Stream video from local file
     */
    static async streamFromLocalFile(req, res, video, deviceId) {
        try {
            const range = req.headers.range;
            const videoPath = this.getVideoPath(video.video_url);
            if (!fs.existsSync(videoPath)) {
                res.status(404).json({ error: "Video file not found" });
                return;
            }
            const videoSize = fs.statSync(videoPath).size;
            const chunkSize = 10 ** 6; // 1MB chunks
            const { start, end, contentLength, statusCode } = this.parseRangeRequest(range, videoSize, chunkSize);
            const headers = {
                "Content-Range": `bytes ${start}-${end}/${videoSize}`,
                "Accept-Ranges": "bytes",
                "Content-Length": contentLength,
                "Content-Type": "video/mp4",
                "Cache-Control": "public, max-age=31536000",
                ETag: `"${video.id}-${video.updated_at.getTime()}"`,
                "X-Source": "local",
            };
            res.status(statusCode).set(headers);
            const videoStream = fs.createReadStream(videoPath, { start, end });
            videoStream.pipe(res);
            // Track streaming session
            this.trackStreamingSession(video.id, deviceId, start, contentLength);
        }
        catch (error) {
            console.error("Stream from local file error:", error);
            if (!res.headersSent) {
                res.status(500).json({ error: "Failed to stream from local file" });
            }
        }
    }
    /**
     * Get video metadata for streaming
     */
    static async getVideoMetadata(videoId) {
        try {
            const result = await (0, db_1.query)("SELECT * FROM videos WHERE id = $1 AND is_active = true", [videoId]);
            return result.rows.length > 0 ? result.rows[0] : null;
        }
        catch (error) {
            console.error("Get video metadata error:", error);
            throw new Error("Failed to get video metadata");
        }
    }
    /**
     * Get available video qualities
     */
    static getAvailableQualities() {
        return [
            { quality: "1080p", resolution: "1920x1080", bitrate: 5000, label: "HD" },
            { quality: "720p", resolution: "1280x720", bitrate: 3000, label: "SD" },
            { quality: "480p", resolution: "854x480", bitrate: 1500, label: "LD" },
            { quality: "360p", resolution: "640x360", bitrate: 800, label: "Mobile" },
        ];
    }
    /**
     * Get video path from URL
     */
    static getVideoPath(videoUrl) {
        // In a real implementation, this would map URLs to actual file paths
        // For now, we'll assume the URL is a relative path
        return path.join(process.cwd(), "videos", videoUrl);
    }
    /**
     * Record video view
     */
    static async recordView(videoId, deviceId, viewDuration) {
        try {
            await (0, db_1.query)(`INSERT INTO video_views (video_id, device_id, view_duration, is_qualified_view)
         VALUES ($1, $2, $3, $4)`, [videoId, deviceId, viewDuration, viewDuration >= 30]);
            await (0, db_1.query)("UPDATE videos SET views_count = views_count + 1 WHERE id = $1", [videoId]);
        }
        catch (error) {
            console.error("Record view error:", error);
        }
    }
    /**
     * Track streaming session
     */
    static trackStreamingSession(videoId, deviceId, start, contentLength) {
        // In a real implementation, this would track streaming metrics
        // For now, we'll just log the session
        console.log(`Streaming session: ${videoId} for ${deviceId}, bytes ${start}-${start + contentLength - 1}`);
    }
    /**
     * Get video recommendations based on watch history
     */
    static async getRecommendations(deviceId, limit = 10) {
        try {
            const result = await (0, db_1.query)(`WITH watched_categories AS (
           SELECT DISTINCT v.id
           FROM watch_progress wp
           JOIN videos v ON wp.video_id = v.id
           WHERE wp.device_id = $1 AND wp.is_completed = true
           LIMIT 5
         ),
         similar_videos AS (
           SELECT v.*, COUNT(*) as match_count
           FROM videos v
           JOIN watch_progress wp ON v.id = wp.video_id
           WHERE wp.device_id = $1 AND wp.is_completed = true
           AND v.id NOT IN (SELECT id FROM watched_categories)
           GROUP BY v.id
           ORDER BY match_count DESC
           LIMIT 20
         )
         SELECT * FROM similar_videos
         UNION
         SELECT v.* FROM videos v
         WHERE v.is_active = true
         AND v.id NOT IN (SELECT id FROM similar_videos)
         ORDER BY v.created_at DESC
         LIMIT $2`, [deviceId, limit]);
            return result.rows;
        }
        catch (error) {
            console.error("Get recommendations error:", error);
            throw new Error("Failed to get recommendations");
        }
    }
    /**
     * Get video analytics
     */
    static async getVideoAnalytics(videoId) {
        try {
            const analyticsResult = await (0, db_1.query)(`SELECT 
           COUNT(*) as total_views,
           COUNT(CASE WHEN is_qualified_view = true THEN 1 END) as qualified_views,
           COALESCE(AVG(view_duration), 0) as avg_watch_time
         FROM video_views 
         WHERE video_id = $1`, [videoId]);
            const completionResult = await (0, db_1.query)(`SELECT 
           COUNT(*) as total_watches,
           COUNT(CASE WHEN is_completed = true THEN 1 END) as completed_watches
         FROM watch_progress 
         WHERE video_id = $1`, [videoId]);
            const peakHoursResult = await (0, db_1.query)(`SELECT 
           EXTRACT(hour FROM created_at) as hour,
           COUNT(*) as views
         FROM video_views 
         WHERE video_id = $1
         GROUP BY EXTRACT(hour FROM created_at)
         ORDER BY views DESC
         LIMIT 5`, [videoId]);
            const analytics = analyticsResult.rows[0];
            const completion = completionResult.rows[0];
            return {
                totalViews: parseInt(analytics.total_views),
                qualifiedViews: parseInt(analytics.qualified_views),
                averageWatchTime: parseFloat(analytics.avg_watch_time),
                completionRate: completion.total_watches > 0
                    ? (parseInt(completion.completed_watches) /
                        parseInt(completion.total_watches)) *
                        100
                    : 0,
                peakHours: peakHoursResult.rows.map((row) => ({
                    hour: parseInt(row.hour),
                    views: parseInt(row.views),
                })),
            };
        }
        catch (error) {
            console.error("Get video analytics error:", error);
            throw new Error("Failed to get video analytics");
        }
    }
    /**
     * Get device streaming history
     */
    static async getDeviceStreamingHistory(deviceId, limit = 50) {
        try {
            const result = await (0, db_1.query)(`SELECT 
           v.id as video_id,
           v.title,
           wp.progress_seconds as watch_time,
           wp.last_watched_at,
           wp.is_completed
         FROM watch_progress wp
         JOIN videos v ON wp.video_id = v.id
         WHERE wp.device_id = $1
         ORDER BY wp.last_watched_at DESC
         LIMIT $2`, [deviceId, limit]);
            return result.rows.map((row) => ({
                videoId: row.video_id,
                title: row.title,
                watchTime: parseInt(row.watch_time),
                lastWatched: row.last_watched_at,
                isCompleted: row.is_completed,
            }));
        }
        catch (error) {
            console.error("Get device streaming history error:", error);
            throw new Error("Failed to get device streaming history");
        }
    }
    /**
     * Optimize video for streaming (transcoding)
     */
    static async optimizeVideo(videoId) {
        try {
            // In a real implementation, this would use a video processing library
            // like FFmpeg to create multiple quality versions
            console.log(`Optimizing video ${videoId} for streaming...`);
            // Update video metadata to mark as optimized
            await (0, db_1.query)("UPDATE videos SET updated_at = NOW() WHERE id = $1", [
                videoId,
            ]);
        }
        catch (error) {
            console.error("Optimize video error:", error);
            throw new Error("Failed to optimize video");
        }
    }
    /**
     * Get streaming health metrics
     */
    static getStreamingHealth() {
        // In a real implementation, this would gather actual metrics
        return {
            activeConnections: 0,
            totalBandwidth: 0,
            averageLatency: 0,
            errorRate: 0,
        };
    }
    /**
     * Get quality from request headers
     */
    static getQualityFromRequest(req) {
        const quality = req.headers["x-quality"];
        const availableQualities = this.getAvailableQualities();
        if (quality && availableQualities.find((q) => q.quality === quality)) {
            return quality;
        }
        // Default to 720p for mobile devices
        const userAgent = req.headers["user-agent"] || "";
        if (userAgent.includes("Mobile")) {
            return "480p";
        }
        return "720p";
    }
    /**
     * Generate Cloudinary URL with transformations
     */
    static generateCloudinaryUrl(videoUrl, quality) {
        // Parse Cloudinary URL and add transformations
        const url = new URL(videoUrl);
        const pathParts = url.pathname.split("/");
        // Find the transformation part
        let transformationIndex = pathParts.findIndex((part) => part.includes("w_") || part.includes("q_") || part.includes("f_"));
        if (transformationIndex === -1) {
            transformationIndex = pathParts.length - 1;
        }
        // Add quality transformation
        const qualityTransform = this.getQualityTransform(quality);
        pathParts.splice(transformationIndex, 0, qualityTransform);
        url.pathname = pathParts.join("/");
        return url.toString();
    }
    /**
     * Get quality transformation string
     */
    static getQualityTransform(quality) {
        switch (quality) {
            case "1080p":
                return "w_1920,h_1080,q_auto,f_auto";
            case "720p":
                return "w_1280,h_720,q_auto,f_auto";
            case "480p":
                return "w_854,h_480,q_auto,f_auto";
            case "360p":
                return "w_640,h_360,q_auto,f_auto";
            default:
                return "w_1280,h_720,q_auto,f_auto";
        }
    }
    /**
     * Get video info from Cloudinary
     */
    static async getCloudinaryVideoInfo(cloudinaryUrl) {
        try {
            const response = await axios_1.default.head(cloudinaryUrl, {
                timeout: 10000,
                headers: {
                    "User-Agent": "Dorflix/1.0",
                },
            });
            const contentLength = response.headers["content-length"];
            return {
                size: contentLength ? parseInt(contentLength) : 0,
            };
        }
        catch (error) {
            console.error("Get Cloudinary video info error:", error);
            return null;
        }
    }
    /**
     * Parse range request and calculate start, end, content length
     */
    static parseRangeRequest(range, totalSize, chunkSize) {
        let start = 0;
        let end = totalSize - 1;
        let statusCode = 200;
        if (range) {
            const rangeMatch = range.match(/bytes=(\d+)-(\d+)?/);
            if (rangeMatch) {
                start = parseInt(rangeMatch[1]);
                end = rangeMatch[2]
                    ? parseInt(rangeMatch[2])
                    : Math.min(start + chunkSize - 1, totalSize - 1);
                if (start >= totalSize || end >= totalSize) {
                    return {
                        start: 0,
                        end: 0,
                        contentLength: 0,
                        statusCode: 416, // Requested Range Not Satisfiable
                    };
                }
                statusCode = 206; // Partial Content
            }
        }
        const contentLength = end - start + 1;
        return { start, end, contentLength, statusCode };
    }
}
exports.StreamingService = StreamingService;
//# sourceMappingURL=streamingService.js.map