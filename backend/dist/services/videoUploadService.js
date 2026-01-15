"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.videoUploadService = exports.VideoUploadService = void 0;
const videoService_1 = require("./videoService");
const cloudinaryService_1 = require("./cloudinaryService");
const multer_1 = __importDefault(require("multer"));
const path_1 = __importDefault(require("path"));
const fs_1 = __importDefault(require("fs"));
class VideoUploadService {
    constructor() {
        // Configure multer for video uploads
        const storage = multer_1.default.diskStorage({
            destination: (req, file, cb) => {
                const uploadDir = "uploads/";
                if (!fs_1.default.existsSync(uploadDir)) {
                    fs_1.default.mkdirSync(uploadDir, { recursive: true });
                }
                cb(null, uploadDir);
            },
            filename: (req, file, cb) => {
                const uniqueSuffix = Date.now() + "-" + Math.round(Math.random() * 1e9);
                cb(null, file.fieldname + "-" + uniqueSuffix + path_1.default.extname(file.originalname));
            },
        });
        this.upload = (0, multer_1.default)({
            storage: storage,
            limits: {
                fileSize: 1024 * 1024 * 1024, // 1GB limit
            },
            fileFilter: (req, file, cb) => {
                const allowedTypes = [
                    "video/mp4",
                    "video/mov",
                    "video/avi",
                    "video/mkv",
                    "video/webm",
                    "video/flv",
                ];
                if (allowedTypes.includes(file.mimetype)) {
                    cb(null, true);
                }
                else {
                    cb(new Error("Invalid file type. Only video files are allowed."));
                }
            },
        });
    }
    /**
     * Get multer upload middleware
     */
    getUploadMiddleware() {
        return this.upload.single("video");
    }
    /**
     * Upload video to Cloudinary and create database record
     */
    async uploadVideo(file, uploadData) {
        try {
            // Upload to Cloudinary
            const uploadResult = await cloudinaryService_1.cloudinaryService.uploadVideo(file.buffer || fs_1.default.readFileSync(file.path), file.originalname);
            // Create video record in database
            const video = await videoService_1.VideoService.createVideo({
                title: uploadData.title,
                description: uploadData.description,
                videoUrl: uploadResult.secure_url,
                thumbnailUrl: cloudinaryService_1.cloudinaryService.generateThumbnailUrl(uploadResult.public_id),
                duration: Math.round(uploadResult.duration || 0),
                uploaderName: uploadData.uploaderName,
                uploaderAvatarUrl: uploadData.uploaderAvatarUrl,
            });
            // Clean up local file
            if (file.path) {
                fs_1.default.unlinkSync(file.path);
            }
            return { video, uploadResult };
        }
        catch (error) {
            // Clean up local file on error
            if (file.path && fs_1.default.existsSync(file.path)) {
                fs_1.default.unlinkSync(file.path);
            }
            throw error;
        }
    }
    /**
     * Get Cloudinary configuration for frontend
     */
    getCloudinaryConfig() {
        return cloudinaryService_1.cloudinaryService.getCloudinaryConfig();
    }
    /**
     * Delete video from Cloudinary and database
     */
    async deleteVideo(videoId) {
        try {
            // Get video from database
            const video = await videoService_1.VideoService.getVideoById(videoId);
            if (!video) {
                throw new Error("Video not found");
            }
            // Extract public ID from Cloudinary URL
            const publicId = this.extractPublicIdFromUrl(video.video_url);
            if (publicId) {
                // Delete from Cloudinary
                await cloudinaryService_1.cloudinaryService.deleteVideo(publicId);
            }
            // Delete from database (soft delete)
            await videoService_1.VideoService.deleteVideo(videoId);
        }
        catch (error) {
            console.error("Delete video error:", error);
            throw new Error("Failed to delete video");
        }
    }
    /**
     * Extract public ID from Cloudinary URL
     */
    extractPublicIdFromUrl(url) {
        try {
            const urlParts = url.split("/");
            const videoIndex = urlParts.indexOf("video");
            if (videoIndex !== -1 && urlParts[videoIndex + 1]) {
                return urlParts[videoIndex + 1].split(".")[0];
            }
            return null;
        }
        catch (error) {
            return null;
        }
    }
}
exports.VideoUploadService = VideoUploadService;
exports.videoUploadService = new VideoUploadService();
//# sourceMappingURL=videoUploadService.js.map