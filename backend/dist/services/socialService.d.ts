import { VideoLike, VideoShare } from "../database/db";
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
export declare class SocialService {
    /**
     * Like a video
     */
    static likeVideo(data: LikeData): Promise<VideoLike>;
    /**
     * Unlike a video
     */
    static unlikeVideo(data: LikeData): Promise<void>;
    /**
     * Check if video is liked by device
     */
    static isVideoLiked(videoId: string, deviceId: string): Promise<boolean>;
    /**
     * Get like count for a video
     */
    static getLikeCount(videoId: string): Promise<number>;
    /**
     * Share a video
     */
    static shareVideo(data: ShareData): Promise<VideoShare>;
    /**
     * Get share count for a video
     */
    static getShareCount(videoId: string): Promise<number>;
    /**
     * Get shares by platform for a video
     */
    static getSharesByPlatform(videoId: string): Promise<Array<{
        platform: string;
        count: number;
    }>>;
    /**
     * Get device's liked videos
     */
    static getDeviceLikedVideos(deviceId: string, limit?: number, offset?: number): Promise<{
        videos: VideoLike[];
        total: number;
    }>;
    /**
     * Get device's shared videos
     */
    static getDeviceSharedVideos(deviceId: string, limit?: number, offset?: number): Promise<{
        videos: VideoShare[];
        total: number;
    }>;
    /**
     * Get social statistics for a video
     */
    static getVideoSocialStats(videoId: string): Promise<{
        likes: number;
        comments: number;
        shares: number;
        isLiked: boolean;
    }>;
    /**
     * Get trending videos based on social engagement
     */
    static getTrendingVideos(limit?: number, offset?: number, days?: number): Promise<{
        videos: Array<{
            id: string;
            title: string;
            engagement: number;
        }>;
    }>;
    /**
     * Get social activity for a device
     */
    static getDeviceSocialActivity(deviceId: string): Promise<{
        totalLikes: number;
        totalShares: number;
        totalComments: number;
        recentActivity: Array<{
            type: "like" | "share" | "comment";
            videoId: string;
            videoTitle: string;
            timestamp: Date;
        }>;
    }>;
}
//# sourceMappingURL=socialService.d.ts.map