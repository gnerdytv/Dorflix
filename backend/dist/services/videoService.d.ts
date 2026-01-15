import { Video, VideoComment } from "../database/db";
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
export declare class VideoService {
    /**
     * Get video catalog with pagination and optional authentication
     */
    static getVideoCatalog(params?: VideoCatalogParams): Promise<{
        videos: VideoWithStats[];
        total: number;
    }>;
    /**
     * Get video by ID with detailed information
     */
    static getVideoById(videoId: string, deviceId?: string): Promise<VideoWithStats | null>;
    /**
     * Create a new video
     */
    static createVideo(videoData: {
        title: string;
        description?: string;
        videoUrl: string;
        thumbnailUrl?: string;
        duration: number;
        uploaderName: string;
        uploaderAvatarUrl?: string;
    }): Promise<Video>;
    /**
     * Record video view
     */
    static recordView(videoId: string, deviceId: string, viewDuration: number): Promise<void>;
    /**
     * Get video comments
     */
    static getComments(videoId: string, limit?: number, offset?: number): Promise<{
        comments: VideoComment[];
        total: number;
    }>;
    /**
     * Add comment to video
     */
    static addComment(videoId: string, deviceId: string, commentText: string): Promise<VideoComment>;
    /**
     * Delete comment
     */
    static deleteComment(commentId: string, deviceId: string): Promise<void>;
    /**
     * Search videos by title
     */
    static searchVideos(searchTerm: string, params?: VideoCatalogParams): Promise<{
        videos: VideoWithStats[];
        total: number;
    }>;
    /**
     * Delete video (soft delete)
     */
    static deleteVideo(videoId: string): Promise<void>;
}
//# sourceMappingURL=videoService.d.ts.map