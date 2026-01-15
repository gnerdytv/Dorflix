export interface CloudinaryUploadResult {
    public_id: string;
    secure_url: string;
    duration?: number;
    width?: number;
    height?: number;
    format: string;
    original_filename: string;
}
export interface CloudinaryVideoInfo {
    duration: number;
    width: number;
    height: number;
    format: string;
    original_filename: string;
}
declare class CloudinaryService {
    private folder;
    /**
     * Upload video to Cloudinary
     */
    uploadVideo(fileBuffer: Buffer, fileName: string): Promise<CloudinaryUploadResult>;
    /**
     * Get video information from Cloudinary
     */
    getVideoInfo(publicId: string): Promise<CloudinaryVideoInfo>;
    /**
     * Delete video from Cloudinary
     */
    deleteVideo(publicId: string): Promise<void>;
    /**
     * Generate video thumbnail URL
     */
    generateThumbnailUrl(publicId: string, width?: number): string;
    /**
     * Generate adaptive streaming URLs
     */
    generateStreamingUrls(publicId: string): {
        hls: string;
        mp4: string;
        webm: string;
    };
    /**
     * Get Cloudinary configuration for frontend
     */
    getCloudinaryConfig(): {
        cloudName: string | undefined;
        apiKey: string | undefined;
        folder: string;
    };
}
export declare const cloudinaryService: CloudinaryService;
export {};
//# sourceMappingURL=cloudinaryService.d.ts.map