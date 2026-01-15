"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.SocialService = void 0;
const db_1 = require("../database/db");
class SocialService {
    /**
     * Like a video
     */
    static async likeVideo(data) {
        try {
            // Check if video exists and is active
            const videoResult = await (0, db_1.query)("SELECT id FROM videos WHERE id = $1 AND is_active = true", [data.videoId]);
            if (videoResult.rows.length === 0) {
                throw new Error("Video not found or inactive");
            }
            // Check if already liked
            const existingLikeResult = await (0, db_1.query)("SELECT id FROM video_likes WHERE video_id = $1 AND device_id = $2", [data.videoId, data.deviceId]);
            let like;
            if (existingLikeResult.rows.length > 0) {
                // Already liked, return existing
                like = existingLikeResult.rows[0];
            }
            else {
                // Create new like
                const insertResult = await (0, db_1.query)(`INSERT INTO video_likes (video_id, device_id)
           VALUES ($1, $2)
           RETURNING *`, [data.videoId, data.deviceId]);
                like = insertResult.rows[0];
                // Update video likes count
                await (0, db_1.query)("UPDATE videos SET likes_count = likes_count + 1 WHERE id = $1", [data.videoId]);
            }
            return like;
        }
        catch (error) {
            console.error("Like video error:", error);
            throw new Error("Failed to like video");
        }
    }
    /**
     * Unlike a video
     */
    static async unlikeVideo(data) {
        try {
            // Check if like exists
            const likeResult = await (0, db_1.query)("SELECT id FROM video_likes WHERE video_id = $1 AND device_id = $2", [data.videoId, data.deviceId]);
            if (likeResult.rows.length === 0) {
                throw new Error("Video not liked");
            }
            // Remove like
            await (0, db_1.query)("DELETE FROM video_likes WHERE video_id = $1 AND device_id = $2", [data.videoId, data.deviceId]);
            // Update video likes count
            await (0, db_1.query)("UPDATE videos SET likes_count = likes_count - 1 WHERE id = $1", [data.videoId]);
        }
        catch (error) {
            console.error("Unlike video error:", error);
            throw new Error("Failed to unlike video");
        }
    }
    /**
     * Check if video is liked by device
     */
    static async isVideoLiked(videoId, deviceId) {
        try {
            const result = await (0, db_1.query)("SELECT id FROM video_likes WHERE video_id = $1 AND device_id = $2", [videoId, deviceId]);
            return result.rows.length > 0;
        }
        catch (error) {
            console.error("Check like status error:", error);
            throw new Error("Failed to check like status");
        }
    }
    /**
     * Get like count for a video
     */
    static async getLikeCount(videoId) {
        try {
            const result = await (0, db_1.query)("SELECT COUNT(*) as like_count FROM video_likes WHERE video_id = $1", [videoId]);
            return parseInt(result.rows[0].like_count);
        }
        catch (error) {
            console.error("Get like count error:", error);
            throw new Error("Failed to get like count");
        }
    }
    /**
     * Share a video
     */
    static async shareVideo(data) {
        try {
            // Check if video exists and is active
            const videoResult = await (0, db_1.query)("SELECT id FROM videos WHERE id = $1 AND is_active = true", [data.videoId]);
            if (videoResult.rows.length === 0) {
                throw new Error("Video not found or inactive");
            }
            // Create share record
            const insertResult = await (0, db_1.query)(`INSERT INTO video_shares (video_id, device_id, share_platform)
         VALUES ($1, $2, $3)
         RETURNING *`, [data.videoId, data.deviceId, data.sharePlatform]);
            const share = insertResult.rows[0];
            // Update video shares count
            await (0, db_1.query)("UPDATE videos SET shares_count = shares_count + 1 WHERE id = $1", [data.videoId]);
            return share;
        }
        catch (error) {
            console.error("Share video error:", error);
            throw new Error("Failed to share video");
        }
    }
    /**
     * Get share count for a video
     */
    static async getShareCount(videoId) {
        try {
            const result = await (0, db_1.query)("SELECT COUNT(*) as share_count FROM video_shares WHERE video_id = $1", [videoId]);
            return parseInt(result.rows[0].share_count);
        }
        catch (error) {
            console.error("Get share count error:", error);
            throw new Error("Failed to get share count");
        }
    }
    /**
     * Get shares by platform for a video
     */
    static async getSharesByPlatform(videoId) {
        try {
            const result = await (0, db_1.query)(`SELECT share_platform as platform, COUNT(*) as count
         FROM video_shares 
         WHERE video_id = $1 AND share_platform IS NOT NULL
         GROUP BY share_platform
         ORDER BY count DESC`, [videoId]);
            return result.rows.map((row) => ({
                platform: row.platform,
                count: parseInt(row.count),
            }));
        }
        catch (error) {
            console.error("Get shares by platform error:", error);
            throw new Error("Failed to get shares by platform");
        }
    }
    /**
     * Get device's liked videos
     */
    static async getDeviceLikedVideos(deviceId, limit = 20, offset = 0) {
        try {
            // Get total count
            const countResult = await (0, db_1.query)("SELECT COUNT(*) as total FROM video_likes WHERE device_id = $1", [deviceId]);
            const total = parseInt(countResult.rows[0].total);
            // Get liked videos
            const likesResult = await (0, db_1.query)(`SELECT vl.*, v.title, v.thumbnail_url
         FROM video_likes vl
         JOIN videos v ON vl.video_id = v.id
         WHERE vl.device_id = $1
         ORDER BY vl.created_at DESC
         LIMIT $2 OFFSET $3`, [deviceId, limit, offset]);
            return { videos: likesResult.rows, total };
        }
        catch (error) {
            console.error("Get device liked videos error:", error);
            throw new Error("Failed to get device liked videos");
        }
    }
    /**
     * Get device's shared videos
     */
    static async getDeviceSharedVideos(deviceId, limit = 20, offset = 0) {
        try {
            // Get total count
            const countResult = await (0, db_1.query)("SELECT COUNT(*) as total FROM video_shares WHERE device_id = $1", [deviceId]);
            const total = parseInt(countResult.rows[0].total);
            // Get shared videos
            const sharesResult = await (0, db_1.query)(`SELECT vs.*, v.title, v.thumbnail_url
         FROM video_shares vs
         JOIN videos v ON vs.video_id = v.id
         WHERE vs.device_id = $1
         ORDER BY vs.created_at DESC
         LIMIT $2 OFFSET $3`, [deviceId, limit, offset]);
            return { videos: sharesResult.rows, total };
        }
        catch (error) {
            console.error("Get device shared videos error:", error);
            throw new Error("Failed to get device shared videos");
        }
    }
    /**
     * Get social statistics for a video
     */
    static async getVideoSocialStats(videoId) {
        try {
            const statsResult = await (0, db_1.query)(`SELECT 
           COALESCE(v.likes_count, 0) as likes,
           COALESCE(v.comments_count, 0) as comments,
           COALESCE(v.shares_count, 0) as shares
         FROM videos v
         WHERE v.id = $1`, [videoId]);
            if (statsResult.rows.length === 0) {
                throw new Error("Video not found");
            }
            const stats = statsResult.rows[0];
            return {
                likes: parseInt(stats.likes),
                comments: parseInt(stats.comments),
                shares: parseInt(stats.shares),
                isLiked: false, // This would need deviceId to check
            };
        }
        catch (error) {
            console.error("Get video social stats error:", error);
            throw new Error("Failed to get video social stats");
        }
    }
    /**
     * Get trending videos based on social engagement
     */
    static async getTrendingVideos(limit = 20, offset = 0, days = 7) {
        try {
            const result = await (0, db_1.query)(`SELECT 
           v.id,
           v.title,
           COALESCE(v.likes_count, 0) + 
           COALESCE(v.comments_count, 0) * 2 + 
           COALESCE(v.shares_count, 0) * 3 as engagement
         FROM videos v
         WHERE v.is_active = true
         ORDER BY engagement DESC
         LIMIT $1 OFFSET $2`, [limit, offset]);
            return {
                videos: result.rows.map((row) => ({
                    id: row.id,
                    title: row.title,
                    engagement: parseInt(row.engagement),
                })),
            };
        }
        catch (error) {
            console.error("Get trending videos error:", error);
            throw new Error("Failed to get trending videos");
        }
    }
    /**
     * Get social activity for a device
     */
    static async getDeviceSocialActivity(deviceId) {
        try {
            // Get counts
            const countsResult = await (0, db_1.query)(`SELECT 
           (SELECT COUNT(*) FROM video_likes WHERE device_id = $1) as total_likes,
           (SELECT COUNT(*) FROM video_shares WHERE device_id = $1) as total_shares,
           (SELECT COUNT(*) FROM video_comments WHERE device_id = $1 AND is_active = true) as total_comments`, [deviceId]);
            const counts = countsResult.rows[0];
            // Get recent activity
            const activityResult = await (0, db_1.query)(`SELECT 
           'like' as type,
           vl.video_id,
           v.title as video_title,
           vl.created_at as timestamp
         FROM video_likes vl
         JOIN videos v ON vl.video_id = v.id
         WHERE vl.device_id = $1
         
         UNION ALL
         
         SELECT 
           'share' as type,
           vs.video_id,
           v.title as video_title,
           vs.created_at as timestamp
         FROM video_shares vs
         JOIN videos v ON vs.video_id = v.id
         WHERE vs.device_id = $1
         
         UNION ALL
         
         SELECT 
           'comment' as type,
           vc.video_id,
           v.title as video_title,
           vc.created_at as timestamp
         FROM video_comments vc
         JOIN videos v ON vc.video_id = v.id
         WHERE vc.device_id = $1 AND vc.is_active = true
         
         ORDER BY timestamp DESC
         LIMIT 20`, [deviceId]);
            return {
                totalLikes: parseInt(counts.total_likes),
                totalShares: parseInt(counts.total_shares),
                totalComments: parseInt(counts.total_comments),
                recentActivity: activityResult.rows.map((row) => ({
                    type: row.type,
                    videoId: row.video_id,
                    videoTitle: row.video_title,
                    timestamp: row.timestamp,
                })),
            };
        }
        catch (error) {
            console.error("Get device social activity error:", error);
            throw new Error("Failed to get device social activity");
        }
    }
}
exports.SocialService = SocialService;
//# sourceMappingURL=socialService.js.map