import { CloudinaryUploadResult } from "./cloudinaryService";
export interface VideoUploadData {
    title: string;
    description?: string;
    uploaderName: string;
    uploaderAvatarUrl?: string;
}
export declare class VideoUploadService {
    private upload;
    constructor();
    /**
     * Get multer upload middleware
     */
    getUploadMiddleware(): import("express").RequestHandler<import("express-serve-static-core").ParamsDictionary, any, any, import("qs").ParsedQs, Record<string, any>>;
    /**
     * Upload video to Cloudinary and create database record
     */
    uploadVideo(file: Express.Multer.File, uploadData: VideoUploadData): Promise<{
        video: any;
        uploadResult: CloudinaryUploadResult;
    }>;
    /**
     * Get Cloudinary configuration for frontend
     */
    getCloudinaryConfig(): {
        cloudName: string | undefined;
        apiKey: string | undefined;
        folder: string;
    };
    /**
     * Delete video from Cloudinary and database
     */
    deleteVideo(videoId: string): Promise<void>;
    /**
     * Extract public ID from Cloudinary URL
     */
    private extractPublicIdFromUrl;
}
export declare const videoUploadService: VideoUploadService;
//# sourceMappingURL=videoUploadService.d.ts.map