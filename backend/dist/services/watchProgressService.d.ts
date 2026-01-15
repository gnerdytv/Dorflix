import { WatchProgress } from "../database/db";
export interface WatchProgressData {
    videoId: string;
    deviceId: string;
    progressSeconds: number;
    totalDuration: number;
    percentageWatched: number;
    isCompleted: boolean;
}
export declare class WatchProgressService {
    /**
     * Save or update watch progress for a video
     */
    static saveWatchProgress(data: WatchProgressData): Promise<WatchProgress>;
    /**
     * Get watch progress for a specific video and device
     */
    static getWatchProgress(videoId: string, deviceId: string): Promise<WatchProgress | null>;
    /**
     * Get all watch progress for a device
     */
    static getDeviceWatchProgress(deviceId: string, limit?: number, offset?: number): Promise<{
        progress: WatchProgress[];
        total: number;
    }>;
    /**
     * Get completed videos for a device
     */
    static getCompletedVideos(deviceId: string, limit?: number, offset?: number): Promise<{
        videos: WatchProgress[];
        total: number;
    }>;
    /**
     * Get continue watching videos (not completed)
     */
    static getContinueWatching(deviceId: string, limit?: number): Promise<WatchProgress[]>;
    /**
     * Clear watch progress for a specific video
     */
    static clearWatchProgress(videoId: string, deviceId: string): Promise<void>;
    /**
     * Clear all watch progress for a device
     */
    static clearDeviceWatchProgress(deviceId: string): Promise<void>;
    /**
     * Get watch statistics for a device
     */
    static getWatchStatistics(deviceId: string): Promise<{
        totalVideosWatched: number;
        totalWatchTime: number;
        completedVideos: number;
        averageWatchTime: number;
    }>;
    /**
     * Get watch history analytics for a device
     */
    static getWatchHistoryAnalytics(deviceId: string, days?: number): Promise<{
        dailyWatchTime: Array<{
            date: string;
            watchTime: number;
        }>;
        mostWatchedVideos: Array<{
            videoId: string;
            title: string;
            watchCount: number;
        }>;
        watchTimeByHour: Array<{
            hour: number;
            watchCount: number;
        }>;
    }>;
}
//# sourceMappingURL=watchProgressService.d.ts.map