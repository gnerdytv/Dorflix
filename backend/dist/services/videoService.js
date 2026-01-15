"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.VideoService = void 0;
const db_1 = require("../database/db");
class VideoService {
    /**
     * Get video catalog with pagination and optional authentication
     */
    static async getVideoCatalog(params = {}) {
        try {
            const { limit = 20, offset = 0, deviceId, sortBy = "created_at", sortOrder = "desc", } = params;
            // Build base query
            let baseQuery = `
        SELECT v.*, 
               COALESCE(v.likes_count, 0) as likes_count,
               COALESCE(v.comments_count, 0) as comments_count,
               COALESCE(v.shares_count, 0) as shares_count,
               COALESCE(v.views_count, 0) as views_count
        FROM videos v
        WHERE v.is_active = true
      `;
            // Add device-specific data if authenticated
            if (deviceId) {
                baseQuery = `
          SELECT v.*, 
                 COALESCE(v.likes_count, 0) as likes_count,
                 COALESCE(v.comments_count, 0) as comments_count,
                 COALESCE(v.shares_count, 0) as shares_count,
                 COALESCE(v.views_count, 0) as views_count,
                 (SELECT EXISTS(
                   SELECT 1 FROM video_likes vl 
                   WHERE vl.video_id = v.id AND vl.device_id = $3
                 )) as is_liked,
                 (SELECT EXISTS(
                   SELECT 1 FROM watch_progress wp 
                   WHERE wp.video_id = v.id AND wp.device_id = $3 AND wp.is_completed = true
                 )) as is_completed,
                 (SELECT progress_seconds FROM watch_progress wp 
                  WHERE wp.video_id = v.id AND wp.device_id = $3 
                  ORDER BY wp.last_watched_at DESC LIMIT 1
                 ) as current_progress
          FROM videos v
          WHERE v.is_active = true
        `;
            }
            // Add sorting and pagination
            const orderByClause = `ORDER BY v.${sortBy} ${sortOrder.toUpperCase()}`;
            const limitClause = `LIMIT $1 OFFSET $2`;
            // Get total count
            const countQuery = `
        SELECT COUNT(*) as total 
        FROM videos v 
        WHERE v.is_active = true
      `;
            const countResult = await (0, db_1.query)(countQuery);
            const total = parseInt(countResult.rows[0].total);
            // Get videos
            const videoQuery = `${baseQuery} ${orderByClause} ${limitClause}`;
            const queryParams = deviceId
                ? [limit, offset, deviceId]
                : [limit, offset];
            const videoResult = await (0, db_1.query)(videoQuery, queryParams);
            const videos = videoResult.rows;
            return { videos, total };
        }
        catch (error) {
            console.error("Get video catalog error:", error);
            throw new Error("Failed to fetch video catalog");
        }
    }
    /**
     * Get video by ID with detailed information
     */
    static async getVideoById(videoId, deviceId) {
        try {
            console.log(`=== VideoService.getVideoById START ===`);
            console.log(`Input videoId: '${videoId}'`);
            console.log(`Input deviceId: '${deviceId || "none"}'`);
            let queryText = `
        SELECT v.*,
               COALESCE(v.likes_count, 0) as likes_count,
               COALESCE(v.comments_count, 0) as comments_count,
               COALESCE(v.shares_count, 0) as shares_count,
               COALESCE(v.views_count, 0) as views_count
        FROM videos v
        WHERE v.id = $1 AND v.is_active = true
      `;
            let queryParams = [videoId];
            if (deviceId) {
                queryText = `
          SELECT v.*,
                 COALESCE(v.likes_count, 0) as likes_count,
                 COALESCE(v.comments_count, 0) as comments_count,
                 COALESCE(v.shares_count, 0) as shares_count,
                 COALESCE(v.views_count, 0) as views_count,
                 (SELECT EXISTS(
                   SELECT 1 FROM video_likes vl
                   WHERE vl.video_id = v.id AND vl.device_id = $2
                 )) as is_liked,
                 (SELECT EXISTS(
                   SELECT 1 FROM watch_progress wp
                   WHERE wp.video_id = v.id AND wp.device_id = $2 AND wp.is_completed = true
                 )) as is_completed,
                 (SELECT progress_seconds FROM watch_progress wp
                  WHERE wp.video_id = v.id AND wp.device_id = $2
                  ORDER BY wp.last_watched_at DESC LIMIT 1
                 ) as current_progress
          FROM videos v
          WHERE v.id = $1 AND v.is_active = true
        `;
                queryParams = [videoId, deviceId];
            }
            console.log(`Executing query:`, queryText);
            console.log(`Query params:`, queryParams);
            const result = await (0, db_1.query)(queryText, queryParams);
            console.log(`Raw database result:`, JSON.stringify(result, null, 2));
            console.log(`Result rows count: ${result.rows.length}`);
            console.log(`Result rows:`, result.rows);
            if (result.rows.length > 0) {
                console.log(`First row data:`, result.rows[0]);
                console.log(`First row video_url:`, result.rows[0].video_url);
                console.log(`First row id:`, result.rows[0].id);
                console.log(`First row title:`, result.rows[0].title);
            }
            let finalResult = null;
            if (result.rows.length > 0) {
                const row = result.rows[0];
                // Explicitly map the database result to ensure proper JSON serialization
                finalResult = {
                    id: row.id,
                    title: row.title,
                    description: row.description,
                    video_url: row.video_url,
                    thumbnail_url: row.thumbnail_url,
                    duration: row.duration,
                    uploader_name: row.uploader_name,
                    uploader_avatar_url: row.uploader_avatar_url,
                    likes_count: row.likes_count || 0,
                    comments_count: row.comments_count || 0,
                    shares_count: row.shares_count || 0,
                    views_count: row.views_count || 0,
                    is_active: row.is_active,
                    created_at: row.created_at,
                    updated_at: row.updated_at,
                    // Include device-specific fields if they exist
                    ...(row.is_liked !== undefined && { is_liked: row.is_liked }),
                    ...(row.is_completed !== undefined && {
                        is_completed: row.is_completed,
                    }),
                    ...(row.current_progress !== undefined && {
                        current_progress: row.current_progress,
                    }),
                };
                console.log(`Mapped result:`, finalResult);
            }
            console.log(`Returning result:`, finalResult);
            return finalResult;
        }
        catch (error) {
            console.error("Get video by ID error:", error);
            throw new Error("Failed to fetch video");
        }
    }
    /**
     * Create a new video
     */
    static async createVideo(videoData) {
        try {
            const insertResult = await (0, db_1.query)(`INSERT INTO videos (title, description, video_url, thumbnail_url, duration, uploader_name, uploader_avatar_url)
         VALUES ($1, $2, $3, $4, $5, $6, $7)
         RETURNING *`, [
                videoData.title,
                videoData.description,
                videoData.videoUrl,
                videoData.thumbnailUrl,
                videoData.duration,
                videoData.uploaderName,
                videoData.uploaderAvatarUrl,
            ]);
            return insertResult.rows[0];
        }
        catch (error) {
            console.error("Create video error:", error);
            throw new Error("Failed to create video");
        }
    }
    /**
     * Record video view
     */
    static async recordView(videoId, deviceId, viewDuration) {
        try {
            // Check if video exists and is active
            const videoResult = await (0, db_1.query)("SELECT id FROM videos WHERE id = $1 AND is_active = true", [videoId]);
            if (videoResult.rows.length === 0) {
                throw new Error("Video not found or inactive");
            }
            // Insert view record
            await (0, db_1.query)(`INSERT INTO video_views (video_id, device_id, view_duration, is_qualified_view)
         VALUES ($1, $2, $3, $4)`, [videoId, deviceId, viewDuration, viewDuration >= 30] // Qualified view if watched for 30+ seconds
            );
            // Update video views count
            await (0, db_1.query)("UPDATE videos SET views_count = views_count + 1 WHERE id = $1", [videoId]);
        }
        catch (error) {
            console.error("Record view error:", error);
            throw new Error("Failed to record view");
        }
    }
    /**
     * Get video comments
     */
    static async getComments(videoId, limit = 20, offset = 0) {
        try {
            // Get total count
            const countResult = await (0, db_1.query)("SELECT COUNT(*) as total FROM video_comments WHERE video_id = $1 AND is_active = true", [videoId]);
            const total = parseInt(countResult.rows[0].total);
            // Get comments
            const commentsResult = await (0, db_1.query)(`SELECT * FROM video_comments 
         WHERE video_id = $1 AND is_active = true 
         ORDER BY created_at DESC 
         LIMIT $2 OFFSET $3`, [videoId, limit, offset]);
            return { comments: commentsResult.rows, total };
        }
        catch (error) {
            console.error("Get comments error:", error);
            throw new Error("Failed to fetch comments");
        }
    }
    /**
     * Add comment to video
     */
    static async addComment(videoId, deviceId, commentText) {
        try {
            // Check if video exists and is active
            const videoResult = await (0, db_1.query)("SELECT id FROM videos WHERE id = $1 AND is_active = true", [videoId]);
            if (videoResult.rows.length === 0) {
                throw new Error("Video not found or inactive");
            }
            // Insert comment
            const insertResult = await (0, db_1.query)(`INSERT INTO video_comments (video_id, device_id, comment_text)
         VALUES ($1, $2, $3)
         RETURNING *`, [videoId, deviceId, commentText]);
            // Update video comments count
            await (0, db_1.query)("UPDATE videos SET comments_count = comments_count + 1 WHERE id = $1", [videoId]);
            return insertResult.rows[0];
        }
        catch (error) {
            console.error("Add comment error:", error);
            throw new Error("Failed to add comment");
        }
    }
    /**
     * Delete comment
     */
    static async deleteComment(commentId, deviceId) {
        try {
            // Check if comment exists and belongs to device
            const commentResult = await (0, db_1.query)("SELECT id, device_id FROM video_comments WHERE id = $1", [commentId]);
            if (commentResult.rows.length === 0) {
                throw new Error("Comment not found");
            }
            const comment = commentResult.rows[0];
            if (comment.device_id !== deviceId) {
                throw new Error("Unauthorized to delete this comment");
            }
            // Soft delete comment
            await (0, db_1.query)("UPDATE video_comments SET is_active = false WHERE id = $1", [
                commentId,
            ]);
            // Update video comments count
            await (0, db_1.query)("UPDATE videos SET comments_count = comments_count - 1 WHERE id = (SELECT video_id FROM video_comments WHERE id = $1)", [commentId]);
        }
        catch (error) {
            console.error("Delete comment error:", error);
            throw new Error("Failed to delete comment");
        }
    }
    /**
     * Search videos by title
     */
    static async searchVideos(searchTerm, params = {}) {
        try {
            const { limit = 20, offset = 0, deviceId, sortBy = "created_at", sortOrder = "desc", } = params;
            // Build search query
            let searchQuery = `
        SELECT v.*, 
               COALESCE(v.likes_count, 0) as likes_count,
               COALESCE(v.comments_count, 0) as comments_count,
               COALESCE(v.shares_count, 0) as shares_count,
               COALESCE(v.views_count, 0) as views_count
        FROM videos v
        WHERE v.is_active = true 
        AND v.title ILIKE $1
      `;
            if (deviceId) {
                searchQuery = `
          SELECT v.*, 
                 COALESCE(v.likes_count, 0) as likes_count,
                 COALESCE(v.comments_count, 0) as comments_count,
                 COALESCE(v.shares_count, 0) as shares_count,
                 COALESCE(v.views_count, 0) as views_count,
                 (SELECT EXISTS(
                   SELECT 1 FROM video_likes vl 
                   WHERE vl.video_id = v.id AND vl.device_id = $3
                 )) as is_liked,
                 (SELECT EXISTS(
                   SELECT 1 FROM watch_progress wp 
                   WHERE wp.video_id = v.id AND wp.device_id = $3 AND wp.is_completed = true
                 )) as is_completed,
                 (SELECT progress_seconds FROM watch_progress wp 
                  WHERE wp.video_id = v.id AND wp.device_id = $3 
                  ORDER BY wp.last_watched_at DESC LIMIT 1
                 ) as current_progress
          FROM videos v
          WHERE v.is_active = true 
          AND v.title ILIKE $1
        `;
            }
            const orderByClause = `ORDER BY v.${sortBy} ${sortOrder.toUpperCase()}`;
            const limitClause = `LIMIT $2 OFFSET $3`;
            // Get total count
            const countQuery = `
        SELECT COUNT(*) as total 
        FROM videos v 
        WHERE v.is_active = true 
        AND v.title ILIKE $1
      `;
            const countResult = await (0, db_1.query)(countQuery, [`%${searchTerm}%`]);
            const total = parseInt(countResult.rows[0].total);
            // Get videos
            const videoQuery = `${searchQuery} ${orderByClause} ${limitClause}`;
            const queryParams = deviceId
                ? [`%${searchTerm}%`, limit, offset, deviceId]
                : [`%${searchTerm}%`, limit, offset];
            const videoResult = await (0, db_1.query)(videoQuery, queryParams);
            const videos = videoResult.rows;
            return { videos, total };
        }
        catch (error) {
            console.error("Search videos error:", error);
            throw new Error("Failed to search videos");
        }
    }
    /**
     * Delete video (soft delete)
     */
    static async deleteVideo(videoId) {
        try {
            // Soft delete video
            await (0, db_1.query)("UPDATE videos SET is_active = false WHERE id = $1", [
                videoId,
            ]);
        }
        catch (error) {
            console.error("Delete video error:", error);
            throw new Error("Failed to delete video");
        }
    }
}
exports.VideoService = VideoService;
//# sourceMappingURL=videoService.js.map