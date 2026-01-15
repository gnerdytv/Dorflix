import { query, WatchProgress } from "../database/db";

export interface WatchProgressData {
  videoId: string;
  deviceId: string;
  progressSeconds: number;
  totalDuration: number;
  percentageWatched: number;
  isCompleted: boolean;
}

export class WatchProgressService {
  /**
   * Save or update watch progress for a video
   */
  static async saveWatchProgress(
    data: WatchProgressData
  ): Promise<WatchProgress> {
    try {
      // Check if video exists and is active
      const videoResult = await query(
        "SELECT id, duration FROM videos WHERE id = $1 AND is_active = true",
        [data.videoId]
      );

      if (videoResult.rows.length === 0) {
        throw new Error("Video not found or inactive");
      }

      const video = videoResult.rows[0];
      const actualTotalDuration = video.duration;
      const actualPercentageWatched = Math.min(
        100,
        Math.round((data.progressSeconds / actualTotalDuration) * 100)
      );
      const actualIsCompleted =
        data.progressSeconds >= actualTotalDuration * 0.95; // 95% threshold

      // Check if progress record exists for this video and device
      const existingProgressResult = await query(
        "SELECT id FROM watch_progress WHERE video_id = $1 AND device_id = $2",
        [data.videoId, data.deviceId]
      );

      let progress: WatchProgress;

      if (existingProgressResult.rows.length > 0) {
        // Update existing progress
        const updateResult = await query(
          `UPDATE watch_progress 
           SET progress_seconds = $3, 
               total_duration = $4, 
               percentage_watched = $5, 
               is_completed = $6, 
               last_watched_at = NOW(),
               updated_at = NOW()
           WHERE video_id = $1 AND device_id = $2
           RETURNING *`,
          [
            data.videoId,
            data.deviceId,
            data.progressSeconds,
            actualTotalDuration,
            actualPercentageWatched,
            actualIsCompleted,
          ]
        );
        progress = updateResult.rows[0];
      } else {
        // Create new progress record
        const insertResult = await query(
          `INSERT INTO watch_progress (video_id, device_id, progress_seconds, total_duration, percentage_watched, is_completed)
           VALUES ($1, $2, $3, $4, $5, $6)
           RETURNING *`,
          [
            data.videoId,
            data.deviceId,
            data.progressSeconds,
            actualTotalDuration,
            actualPercentageWatched,
            actualIsCompleted,
          ]
        );
        progress = insertResult.rows[0];
      }

      return progress;
    } catch (error) {
      console.error("Save watch progress error:", error);
      throw new Error("Failed to save watch progress");
    }
  }

  /**
   * Get watch progress for a specific video and device
   */
  static async getWatchProgress(
    videoId: string,
    deviceId: string
  ): Promise<WatchProgress | null> {
    try {
      const result = await query(
        `SELECT * FROM watch_progress 
         WHERE video_id = $1 AND device_id = $2 
         ORDER BY last_watched_at DESC 
         LIMIT 1`,
        [videoId, deviceId]
      );
      return result.rows.length > 0 ? result.rows[0] : null;
    } catch (error) {
      console.error("Get watch progress error:", error);
      throw new Error("Failed to get watch progress");
    }
  }

  /**
   * Get all watch progress for a device
   */
  static async getDeviceWatchProgress(
    deviceId: string,
    limit: number = 20,
    offset: number = 0
  ): Promise<{ progress: WatchProgress[]; total: number }> {
    try {
      // Get total count
      const countResult = await query(
        "SELECT COUNT(*) as total FROM watch_progress WHERE device_id = $1",
        [deviceId]
      );
      const total = parseInt(countResult.rows[0].total);

      // Get progress records
      const progressResult = await query(
        `SELECT wp.*, v.title, v.thumbnail_url
         FROM watch_progress wp
         JOIN videos v ON wp.video_id = v.id
         WHERE wp.device_id = $1
         ORDER BY wp.last_watched_at DESC
         LIMIT $2 OFFSET $3`,
        [deviceId, limit, offset]
      );

      return { progress: progressResult.rows, total };
    } catch (error) {
      console.error("Get device watch progress error:", error);
      throw new Error("Failed to get device watch progress");
    }
  }

  /**
   * Get completed videos for a device
   */
  static async getCompletedVideos(
    deviceId: string,
    limit: number = 20,
    offset: number = 0
  ): Promise<{ videos: WatchProgress[]; total: number }> {
    try {
      // Get total count
      const countResult = await query(
        "SELECT COUNT(*) as total FROM watch_progress WHERE device_id = $1 AND is_completed = true",
        [deviceId]
      );
      const total = parseInt(countResult.rows[0].total);

      // Get completed videos
      const completedResult = await query(
        `SELECT wp.*, v.title, v.thumbnail_url
         FROM watch_progress wp
         JOIN videos v ON wp.video_id = v.id
         WHERE wp.device_id = $1 AND wp.is_completed = true
         ORDER BY wp.last_watched_at DESC
         LIMIT $2 OFFSET $3`,
        [deviceId, limit, offset]
      );

      return { videos: completedResult.rows, total };
    } catch (error) {
      console.error("Get completed videos error:", error);
      throw new Error("Failed to get completed videos");
    }
  }

  /**
   * Get continue watching videos (not completed)
   */
  static async getContinueWatching(
    deviceId: string,
    limit: number = 10
  ): Promise<WatchProgress[]> {
    try {
      const result = await query(
        `SELECT wp.*, v.title, v.thumbnail_url
         FROM watch_progress wp
         JOIN videos v ON wp.video_id = v.id
         WHERE wp.device_id = $1 AND wp.is_completed = false
         ORDER BY wp.last_watched_at DESC
         LIMIT $2`,
        [deviceId, limit]
      );
      return result.rows;
    } catch (error) {
      console.error("Get continue watching error:", error);
      throw new Error("Failed to get continue watching videos");
    }
  }

  /**
   * Clear watch progress for a specific video
   */
  static async clearWatchProgress(
    videoId: string,
    deviceId: string
  ): Promise<void> {
    try {
      await query(
        "DELETE FROM watch_progress WHERE video_id = $1 AND device_id = $2",
        [videoId, deviceId]
      );
    } catch (error) {
      console.error("Clear watch progress error:", error);
      throw new Error("Failed to clear watch progress");
    }
  }

  /**
   * Clear all watch progress for a device
   */
  static async clearDeviceWatchProgress(deviceId: string): Promise<void> {
    try {
      await query("DELETE FROM watch_progress WHERE device_id = $1", [
        deviceId,
      ]);
    } catch (error) {
      console.error("Clear device watch progress error:", error);
      throw new Error("Failed to clear device watch progress");
    }
  }

  /**
   * Get watch statistics for a device
   */
  static async getWatchStatistics(deviceId: string): Promise<{
    totalVideosWatched: number;
    totalWatchTime: number;
    completedVideos: number;
    averageWatchTime: number;
  }> {
    try {
      const statsResult = await query(
        `SELECT 
           COUNT(*) as total_videos_watched,
           COALESCE(SUM(progress_seconds), 0) as total_watch_time,
           COUNT(CASE WHEN is_completed = true THEN 1 END) as completed_videos
         FROM watch_progress 
         WHERE device_id = $1`,
        [deviceId]
      );

      const stats = statsResult.rows[0];
      const averageWatchTime =
        stats.total_videos_watched > 0
          ? Math.round(stats.total_watch_time / stats.total_videos_watched)
          : 0;

      return {
        totalVideosWatched: parseInt(stats.total_videos_watched),
        totalWatchTime: parseInt(stats.total_watch_time),
        completedVideos: parseInt(stats.completed_videos),
        averageWatchTime,
      };
    } catch (error) {
      console.error("Get watch statistics error:", error);
      throw new Error("Failed to get watch statistics");
    }
  }

  /**
   * Get watch history analytics for a device
   */
  static async getWatchHistoryAnalytics(
    deviceId: string,
    days: number = 30
  ): Promise<{
    dailyWatchTime: Array<{ date: string; watchTime: number }>;
    mostWatchedVideos: Array<{
      videoId: string;
      title: string;
      watchCount: number;
    }>;
    watchTimeByHour: Array<{ hour: number; watchCount: number }>;
  }> {
    try {
      // Daily watch time
      const dailyWatchTimeResult = await query(
        `SELECT 
           DATE(last_watched_at) as date,
           SUM(progress_seconds) as watch_time
         FROM watch_progress 
         WHERE device_id = $1 
           AND last_watched_at >= NOW() - INTERVAL '${days} days'
         GROUP BY DATE(last_watched_at)
         ORDER BY date DESC`,
        [deviceId]
      );

      // Most watched videos
      const mostWatchedResult = await query(
        `SELECT 
           v.id as video_id,
           v.title,
           COUNT(*) as watch_count
         FROM watch_progress wp
         JOIN videos v ON wp.video_id = v.id
         WHERE wp.device_id = $1
         GROUP BY v.id, v.title
         ORDER BY watch_count DESC
         LIMIT 10`,
        [deviceId]
      );

      // Watch time by hour
      const watchTimeByHourResult = await query(
        `SELECT 
           EXTRACT(hour FROM last_watched_at) as hour,
           COUNT(*) as watch_count
         FROM watch_progress 
         WHERE device_id = $1
         GROUP BY EXTRACT(hour FROM last_watched_at)
         ORDER BY hour`,
        [deviceId]
      );

      return {
        dailyWatchTime: dailyWatchTimeResult.rows.map((row: any) => ({
          date: row.date,
          watchTime: parseInt(row.watch_time),
        })),
        mostWatchedVideos: mostWatchedResult.rows.map((row: any) => ({
          videoId: row.video_id,
          title: row.title,
          watchCount: parseInt(row.watch_count),
        })),
        watchTimeByHour: watchTimeByHourResult.rows.map((row: any) => ({
          hour: parseInt(row.hour),
          watchCount: parseInt(row.watch_count),
        })),
      };
    } catch (error) {
      console.error("Get watch history analytics error:", error);
      throw new Error("Failed to get watch history analytics");
    }
  }
}
