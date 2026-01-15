import { Video } from "../database/db";
import { Request, Response } from "express";
export interface StreamMetadata {
    videoId: string;
    deviceId: string;
    startTime: number;
    duration: number;
    quality: string;
    bitrate: number;
}
export declare class StreamingService {
    /**
     * Get video stream with proper headers and range support
     */
    static streamVideo(req: Request, res: Response, videoId: string, deviceId: string): Promise<void>;
    /**
     * Stream video from Cloudinary with range request support
     */
    private static streamFromCloudinary;
    /**
     * Stream video from local file
     */
    private static streamFromLocalFile;
    /**
     * Get video metadata for streaming
     */
    static getVideoMetadata(videoId: string): Promise<Video | null>;
    /**
     * Get available video qualities
     */
    static getAvailableQualities(): Array<{
        quality: string;
        resolution: string;
        bitrate: number;
        label: string;
    }>;
    /**
     * Get video path from URL
     */
    private static getVideoPath;
    /**
     * Record video view
     */
    private static recordView;
    /**
     * Track streaming session
     */
    private static trackStreamingSession;
    /**
     * Get video recommendations based on watch history
     */
    static getRecommendations(deviceId: string, limit?: number): Promise<Video[]>;
    /**
     * Get video analytics
     */
    static getVideoAnalytics(videoId: string): Promise<{
        totalViews: number;
        qualifiedViews: number;
        averageWatchTime: number;
        completionRate: number;
        peakHours: Array<{
            hour: number;
            views: number;
        }>;
    }>;
    /**
     * Get device streaming history
     */
    static getDeviceStreamingHistory(deviceId: string, limit?: number): Promise<Array<{
        videoId: string;
        title: string;
        watchTime: number;
        lastWatched: Date;
        isCompleted: boolean;
    }>>;
    /**
     * Optimize video for streaming (transcoding)
     */
    static optimizeVideo(videoId: string): Promise<void>;
    /**
     * Get streaming health metrics
     */
    static getStreamingHealth(): {
        activeConnections: number;
        totalBandwidth: number;
        averageLatency: number;
        errorRate: number;
    };
    /**
     * Get quality from request headers
     */
    private static getQualityFromRequest;
    /**
     * Generate Cloudinary URL with transformations
     */
    private static generateCloudinaryUrl;
    /**
     * Get quality transformation string
     */
    private static getQualityTransform;
    /**
     * Get video info from Cloudinary
     */
    private static getCloudinaryVideoInfo;
    /**
     * Parse range request and calculate start, end, content length
     */
    private static parseRangeRequest;
}
//# sourceMappingURL=streamingService.d.ts.map