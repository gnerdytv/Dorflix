import { query, VideoLike, VideoComment, VideoShare } from "../database/db";

export interface LikeData {
  videoId: string;
  deviceId: string;
}

export interface CommentData {
  videoId: string;
  deviceId: string;
  commentText: string;
}

export interface ShareData {
  videoId: string;
  deviceId: string;
  sharePlatform?: string;
}

export class SocialService {
  /**
   * Like a video
   */
  static async likeVideo(data: LikeData): Promise<VideoLike> {
    try {
      // Check if video exists and is active
      const videoResult = await query(
        "SELECT id FROM videos WHERE id = $1 AND is_active = true",
        [data.videoId]
      );

      if (videoResult.rows.length === 0) {
        throw new Error("Video not found or inactive");
      }

      // Check if already liked
      const existingLikeResult = await query(
        "SELECT id FROM video_likes WHERE video_id = $1 AND device_id = $2",
        [data.videoId, data.deviceId]
      );

      let like: VideoLike;

      if (existingLikeResult.rows.length > 0) {
        // Already liked, return existing
        like = existingLikeResult.rows[0];
      } else {
        // Create new like
        const insertResult = await query(
          `INSERT INTO video_likes (video_id, device_id)
           VALUES ($1, $2)
           RETURNING *`,
          [data.videoId, data.deviceId]
        );
        like = insertResult.rows[0];

        // Update video likes count
        await query(
          "UPDATE videos SET likes_count = likes_count + 1 WHERE id = $1",
          [data.videoId]
        );
      }

      return like;
    } catch (error) {
      console.error("Like video error:", error);
      throw new Error("Failed to like video");
    }
  }

  /**
   * Unlike a video
   */
  static async unlikeVideo(data: LikeData): Promise<void> {
    try {
      // Check if like exists
      const likeResult = await query(
        "SELECT id FROM video_likes WHERE video_id = $1 AND device_id = $2",
        [data.videoId, data.deviceId]
      );

      if (likeResult.rows.length === 0) {
        throw new Error("Video not liked");
      }

      // Remove like
      await query(
        "DELETE FROM video_likes WHERE video_id = $1 AND device_id = $2",
        [data.videoId, data.deviceId]
      );

      // Update video likes count
      await query(
        "UPDATE videos SET likes_count = likes_count - 1 WHERE id = $1",
        [data.videoId]
      );
    } catch (error) {
      console.error("Unlike video error:", error);
      throw new Error("Failed to unlike video");
    }
  }

  /**
   * Check if video is liked by device
   */
  static async isVideoLiked(
    videoId: string,
    deviceId: string
  ): Promise<boolean> {
    try {
      const result = await query(
        "SELECT id FROM video_likes WHERE video_id = $1 AND device_id = $2",
        [videoId, deviceId]
      );
      return result.rows.length > 0;
    } catch (error) {
      console.error("Check like status error:", error);
      throw new Error("Failed to check like status");
    }
  }

  /**
   * Get like count for a video
   */
  static async getLikeCount(videoId: string): Promise<number> {
    try {
      const result = await query(
        "SELECT COUNT(*) as like_count FROM video_likes WHERE video_id = $1",
        [videoId]
      );
      return parseInt(result.rows[0].like_count);
    } catch (error) {
      console.error("Get like count error:", error);
      throw new Error("Failed to get like count");
    }
  }

  /**
   * Share a video
   */
  static async shareVideo(data: ShareData): Promise<VideoShare> {
    try {
      // Check if video exists and is active
      const videoResult = await query(
        "SELECT id FROM videos WHERE id = $1 AND is_active = true",
        [data.videoId]
      );

      if (videoResult.rows.length === 0) {
        throw new Error("Video not found or inactive");
      }

      // Create share record
      const insertResult = await query(
        `INSERT INTO video_shares (video_id, device_id, share_platform)
         VALUES ($1, $2, $3)
         RETURNING *`,
        [data.videoId, data.deviceId, data.sharePlatform]
      );

      const share = insertResult.rows[0];

      // Update video shares count
      await query(
        "UPDATE videos SET shares_count = shares_count + 1 WHERE id = $1",
        [data.videoId]
      );

      return share;
    } catch (error) {
      console.error("Share video error:", error);
      throw new Error("Failed to share video");
    }
  }

  /**
   * Get share count for a video
   */
  static async getShareCount(videoId: string): Promise<number> {
    try {
      const result = await query(
        "SELECT COUNT(*) as share_count FROM video_shares WHERE video_id = $1",
        [videoId]
      );
      return parseInt(result.rows[0].share_count);
    } catch (error) {
      console.error("Get share count error:", error);
      throw new Error("Failed to get share count");
    }
  }

  /**
   * Get shares by platform for a video
   */
  static async getSharesByPlatform(videoId: string): Promise<
    Array<{
      platform: string;
      count: number;
    }>
  > {
    try {
      const result = await query(
        `SELECT share_platform as platform, COUNT(*) as count
         FROM video_shares 
         WHERE video_id = $1 AND share_platform IS NOT NULL
         GROUP BY share_platform
         ORDER BY count DESC`,
        [videoId]
      );

      return result.rows.map((row: any) => ({
        platform: row.platform,
        count: parseInt(row.count),
      }));
    } catch (error) {
      console.error("Get shares by platform error:", error);
      throw new Error("Failed to get shares by platform");
    }
  }

  /**
   * Get device's liked videos
   */
  static async getDeviceLikedVideos(
    deviceId: string,
    limit: number = 20,
    offset: number = 0
  ): Promise<{ videos: VideoLike[]; total: number }> {
    try {
      // Get total count
      const countResult = await query(
        "SELECT COUNT(*) as total FROM video_likes WHERE device_id = $1",
        [deviceId]
      );
      const total = parseInt(countResult.rows[0].total);

      // Get liked videos
      const likesResult = await query(
        `SELECT vl.*, v.title, v.thumbnail_url
         FROM video_likes vl
         JOIN videos v ON vl.video_id = v.id
         WHERE vl.device_id = $1
         ORDER BY vl.created_at DESC
         LIMIT $2 OFFSET $3`,
        [deviceId, limit, offset]
      );

      return { videos: likesResult.rows, total };
    } catch (error) {
      console.error("Get device liked videos error:", error);
      throw new Error("Failed to get device liked videos");
    }
  }

  /**
   * Get device's shared videos
   */
  static async getDeviceSharedVideos(
    deviceId: string,
    limit: number = 20,
    offset: number = 0
  ): Promise<{ videos: VideoShare[]; total: number }> {
    try {
      // Get total count
      const countResult = await query(
        "SELECT COUNT(*) as total FROM video_shares WHERE device_id = $1",
        [deviceId]
      );
      const total = parseInt(countResult.rows[0].total);

      // Get shared videos
      const sharesResult = await query(
        `SELECT vs.*, v.title, v.thumbnail_url
         FROM video_shares vs
         JOIN videos v ON vs.video_id = v.id
         WHERE vs.device_id = $1
         ORDER BY vs.created_at DESC
         LIMIT $2 OFFSET $3`,
        [deviceId, limit, offset]
      );

      return { videos: sharesResult.rows, total };
    } catch (error) {
      console.error("Get device shared videos error:", error);
      throw new Error("Failed to get device shared videos");
    }
  }

  /**
   * Get social statistics for a video
   */
  static async getVideoSocialStats(videoId: string): Promise<{
    likes: number;
    comments: number;
    shares: number;
    isLiked: boolean;
  }> {
    try {
      const statsResult = await query(
        `SELECT 
           COALESCE(v.likes_count, 0) as likes,
           COALESCE(v.comments_count, 0) as comments,
           COALESCE(v.shares_count, 0) as shares
         FROM videos v
         WHERE v.id = $1`,
        [videoId]
      );

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
    } catch (error) {
      console.error("Get video social stats error:", error);
      throw new Error("Failed to get video social stats");
    }
  }

  /**
   * Get trending videos based on social engagement
   */
  static async getTrendingVideos(
    limit: number = 20,
    offset: number = 0,
    days: number = 7
  ): Promise<{
    videos: Array<{ id: string; title: string; engagement: number }>;
  }> {
    try {
      const result = await query(
        `SELECT 
           v.id,
           v.title,
           COALESCE(v.likes_count, 0) + 
           COALESCE(v.comments_count, 0) * 2 + 
           COALESCE(v.shares_count, 0) * 3 as engagement
         FROM videos v
         WHERE v.is_active = true
         ORDER BY engagement DESC
         LIMIT $1 OFFSET $2`,
        [limit, offset]
      );

      return {
        videos: result.rows.map((row: any) => ({
          id: row.id,
          title: row.title,
          engagement: parseInt(row.engagement),
        })),
      };
    } catch (error) {
      console.error("Get trending videos error:", error);
      throw new Error("Failed to get trending videos");
    }
  }

  /**
   * Get social activity for a device
   */
  static async getDeviceSocialActivity(deviceId: string): Promise<{
    totalLikes: number;
    totalShares: number;
    totalComments: number;
    recentActivity: Array<{
      type: "like" | "share" | "comment";
      videoId: string;
      videoTitle: string;
      timestamp: Date;
    }>;
  }> {
    try {
      // Get counts
      const countsResult = await query(
        `SELECT 
           (SELECT COUNT(*) FROM video_likes WHERE device_id = $1) as total_likes,
           (SELECT COUNT(*) FROM video_shares WHERE device_id = $1) as total_shares,
           (SELECT COUNT(*) FROM video_comments WHERE device_id = $1 AND is_active = true) as total_comments`,
        [deviceId]
      );

      const counts = countsResult.rows[0];

      // Get recent activity
      const activityResult = await query(
        `SELECT 
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
         LIMIT 20`,
        [deviceId]
      );

      return {
        totalLikes: parseInt(counts.total_likes),
        totalShares: parseInt(counts.total_shares),
        totalComments: parseInt(counts.total_comments),
        recentActivity: activityResult.rows.map((row: any) => ({
          type: row.type as "like" | "share" | "comment",
          videoId: row.video_id,
          videoTitle: row.video_title,
          timestamp: row.timestamp,
        })),
      };
    } catch (error) {
      console.error("Get device social activity error:", error);
      throw new Error("Failed to get device social activity");
    }
  }
}
