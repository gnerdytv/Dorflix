import {
  query,
  Video,
  VideoView,
  VideoLike,
  VideoComment,
  VideoShare,
} from "../database/db";

export interface VideoCatalogParams {
  limit?: number;
  offset?: number;
  deviceId?: string;
  sortBy?: "created_at" | "views_count" | "likes_count";
  sortOrder?: "asc" | "desc";
}

export interface VideoWithStats extends Video {
  is_liked?: boolean;
  is_completed?: boolean;
  current_progress?: number;
}

export class VideoService {
  /**
   * Get video catalog with pagination and optional authentication
   */
  static async getVideoCatalog(
    params: VideoCatalogParams = {}
  ): Promise<{ videos: VideoWithStats[]; total: number }> {
    try {
      const {
        limit = 20,
        offset = 0,
        deviceId,
        sortBy = "created_at",
        sortOrder = "desc",
      } = params;

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

      const countResult = await query(countQuery);
      const total = parseInt(countResult.rows[0].total);

      // Get videos
      const videoQuery = `${baseQuery} ${orderByClause} ${limitClause}`;
      const queryParams = deviceId
        ? [limit, offset, deviceId]
        : [limit, offset];

      const videoResult = await query(videoQuery, queryParams);
      const videos = videoResult.rows;

      return { videos, total };
    } catch (error) {
      console.error("Get video catalog error:", error);
      throw new Error("Failed to fetch video catalog");
    }
  }

  /**
   * Get video by ID with detailed information
   */
  static async getVideoById(
    videoId: string,
    deviceId?: string
  ): Promise<VideoWithStats | null> {
    try {
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

      const result = await query(queryText, queryParams);
      return result.rows.length > 0 ? result.rows[0] : null;
    } catch (error) {
      console.error("Get video by ID error:", error);
      throw new Error("Failed to fetch video");
    }
  }

  /**
   * Create a new video
   */
  static async createVideo(videoData: {
    title: string;
    description?: string;
    videoUrl: string;
    thumbnailUrl?: string;
    duration: number;
    uploaderName: string;
    uploaderAvatarUrl?: string;
  }): Promise<Video> {
    try {
      const insertResult = await query(
        `INSERT INTO videos (title, description, video_url, thumbnail_url, duration, uploader_name, uploader_avatar_url)
         VALUES ($1, $2, $3, $4, $5, $6, $7)
         RETURNING *`,
        [
          videoData.title,
          videoData.description,
          videoData.videoUrl,
          videoData.thumbnailUrl,
          videoData.duration,
          videoData.uploaderName,
          videoData.uploaderAvatarUrl,
        ]
      );
      return insertResult.rows[0];
    } catch (error) {
      console.error("Create video error:", error);
      throw new Error("Failed to create video");
    }
  }

  /**
   * Record video view
   */
  static async recordView(
    videoId: string,
    deviceId: string,
    viewDuration: number
  ): Promise<void> {
    try {
      // Check if video exists and is active
      const videoResult = await query(
        "SELECT id FROM videos WHERE id = $1 AND is_active = true",
        [videoId]
      );

      if (videoResult.rows.length === 0) {
        throw new Error("Video not found or inactive");
      }

      // Insert view record
      await query(
        `INSERT INTO video_views (video_id, device_id, view_duration, is_qualified_view)
         VALUES ($1, $2, $3, $4)`,
        [videoId, deviceId, viewDuration, viewDuration >= 30] // Qualified view if watched for 30+ seconds
      );

      // Update video views count
      await query(
        "UPDATE videos SET views_count = views_count + 1 WHERE id = $1",
        [videoId]
      );
    } catch (error) {
      console.error("Record view error:", error);
      throw new Error("Failed to record view");
    }
  }

  /**
   * Get video comments
   */
  static async getComments(
    videoId: string,
    limit: number = 20,
    offset: number = 0
  ): Promise<{ comments: VideoComment[]; total: number }> {
    try {
      // Get total count
      const countResult = await query(
        "SELECT COUNT(*) as total FROM video_comments WHERE video_id = $1 AND is_active = true",
        [videoId]
      );
      const total = parseInt(countResult.rows[0].total);

      // Get comments
      const commentsResult = await query(
        `SELECT * FROM video_comments 
         WHERE video_id = $1 AND is_active = true 
         ORDER BY created_at DESC 
         LIMIT $2 OFFSET $3`,
        [videoId, limit, offset]
      );

      return { comments: commentsResult.rows, total };
    } catch (error) {
      console.error("Get comments error:", error);
      throw new Error("Failed to fetch comments");
    }
  }

  /**
   * Add comment to video
   */
  static async addComment(
    videoId: string,
    deviceId: string,
    commentText: string
  ): Promise<VideoComment> {
    try {
      // Check if video exists and is active
      const videoResult = await query(
        "SELECT id FROM videos WHERE id = $1 AND is_active = true",
        [videoId]
      );

      if (videoResult.rows.length === 0) {
        throw new Error("Video not found or inactive");
      }

      // Insert comment
      const insertResult = await query(
        `INSERT INTO video_comments (video_id, device_id, comment_text)
         VALUES ($1, $2, $3)
         RETURNING *`,
        [videoId, deviceId, commentText]
      );

      // Update video comments count
      await query(
        "UPDATE videos SET comments_count = comments_count + 1 WHERE id = $1",
        [videoId]
      );

      return insertResult.rows[0];
    } catch (error) {
      console.error("Add comment error:", error);
      throw new Error("Failed to add comment");
    }
  }

  /**
   * Delete comment
   */
  static async deleteComment(
    commentId: string,
    deviceId: string
  ): Promise<void> {
    try {
      // Check if comment exists and belongs to device
      const commentResult = await query(
        "SELECT id, device_id FROM video_comments WHERE id = $1",
        [commentId]
      );

      if (commentResult.rows.length === 0) {
        throw new Error("Comment not found");
      }

      const comment = commentResult.rows[0];
      if (comment.device_id !== deviceId) {
        throw new Error("Unauthorized to delete this comment");
      }

      // Soft delete comment
      await query("UPDATE video_comments SET is_active = false WHERE id = $1", [
        commentId,
      ]);

      // Update video comments count
      await query(
        "UPDATE videos SET comments_count = comments_count - 1 WHERE id = (SELECT video_id FROM video_comments WHERE id = $1)",
        [commentId]
      );
    } catch (error) {
      console.error("Delete comment error:", error);
      throw new Error("Failed to delete comment");
    }
  }

  /**
   * Search videos by title
   */
  static async searchVideos(
    searchTerm: string,
    params: VideoCatalogParams = {}
  ): Promise<{ videos: VideoWithStats[]; total: number }> {
    try {
      const {
        limit = 20,
        offset = 0,
        deviceId,
        sortBy = "created_at",
        sortOrder = "desc",
      } = params;

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

      const countResult = await query(countQuery, [`%${searchTerm}%`]);
      const total = parseInt(countResult.rows[0].total);

      // Get videos
      const videoQuery = `${searchQuery} ${orderByClause} ${limitClause}`;
      const queryParams = deviceId
        ? [`%${searchTerm}%`, limit, offset, deviceId]
        : [`%${searchTerm}%`, limit, offset];

      const videoResult = await query(videoQuery, queryParams);
      const videos = videoResult.rows;

      return { videos, total };
    } catch (error) {
      console.error("Search videos error:", error);
      throw new Error("Failed to search videos");
    }
  }
}
