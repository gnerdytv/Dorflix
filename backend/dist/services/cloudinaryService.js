"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.cloudinaryService = void 0;
const cloudinary_1 = __importDefault(require("cloudinary"));
const dotenv_1 = __importDefault(require("dotenv"));
dotenv_1.default.config();
// Configure Cloudinary
cloudinary_1.default.v2.config({
    cloud_name: process.env.CLOUDINARY_CLOUD_NAME,
    api_key: process.env.CLOUDINARY_API_KEY,
    api_secret: process.env.CLOUDINARY_API_SECRET,
});
class CloudinaryService {
    constructor() {
        this.folder = process.env.CLOUDINARY_FOLDER || "dorflix_videos";
    }
    /**
     * Upload video to Cloudinary
     */
    async uploadVideo(fileBuffer, fileName) {
        return new Promise((resolve, reject) => {
            cloudinary_1.default.v2.uploader
                .upload_stream({
                resource_type: "video",
                folder: this.folder,
                public_id: fileName.replace(/\.[^/.]+$/, ""), // Remove extension for public_id
                chunk_size: 6000000, // 6MB chunks for large files
                eager: [
                    {
                        width: 300,
                        height: 169,
                        crop: "pad",
                        gravity: "north",
                    },
                ],
                eager_async: true,
            }, (error, result) => {
                if (error) {
                    reject(error);
                }
                else if (result) {
                    resolve({
                        public_id: result.public_id,
                        secure_url: result.secure_url,
                        duration: result.duration,
                        width: result.width,
                        height: result.height,
                        format: result.format || "mp4",
                        original_filename: result.original_filename || fileName,
                    });
                }
                else {
                    reject(new Error("Upload failed: No result returned"));
                }
            })
                .end(fileBuffer);
        });
    }
    /**
     * Get video information from Cloudinary
     */
    async getVideoInfo(publicId) {
        return new Promise((resolve, reject) => {
            cloudinary_1.default.v2.api.resource(publicId, { resource_type: "video" }, (error, result) => {
                if (error) {
                    reject(error);
                }
                else if (result) {
                    resolve({
                        duration: result.duration || 0,
                        width: result.width || 0,
                        height: result.height || 0,
                        format: result.format || "mp4",
                        original_filename: result.original_filename || "unknown",
                    });
                }
                else {
                    reject(new Error("Video not found"));
                }
            });
        });
    }
    /**
     * Delete video from Cloudinary
     */
    async deleteVideo(publicId) {
        return new Promise((resolve, reject) => {
            cloudinary_1.default.v2.uploader.destroy(publicId, { resource_type: "video" }, (error, result) => {
                if (error) {
                    reject(error);
                }
                else {
                    resolve();
                }
            });
        });
    }
    /**
     * Generate video thumbnail URL
     */
    generateThumbnailUrl(publicId, width = 300) {
        return cloudinary_1.default.v2.url(publicId, {
            resource_type: "video",
            transformation: [
                {
                    width: width,
                    height: Math.floor(width * 0.5625), // 16:9 aspect ratio
                    crop: "fill",
                    gravity: "north",
                },
            ],
        });
    }
    /**
     * Generate adaptive streaming URLs
     */
    generateStreamingUrls(publicId) {
        return {
            hls: cloudinary_1.default.v2.url(publicId, {
                resource_type: "video",
                format: "m3u8",
                transformation: [
                    {
                        streaming_profile: "full_hd",
                    },
                ],
            }),
            mp4: cloudinary_1.default.v2.url(publicId, {
                resource_type: "video",
                format: "mp4",
                transformation: [
                    {
                        quality: "auto",
                        fetch_format: "auto",
                    },
                ],
            }),
            webm: cloudinary_1.default.v2.url(publicId, {
                resource_type: "video",
                format: "webm",
                transformation: [
                    {
                        quality: "auto",
                        fetch_format: "auto",
                    },
                ],
            }),
        };
    }
    /**
     * Get Cloudinary configuration for frontend
     */
    getCloudinaryConfig() {
        return {
            cloudName: process.env.CLOUDINARY_CLOUD_NAME,
            apiKey: process.env.CLOUDINARY_API_KEY,
            folder: this.folder,
        };
    }
}
exports.cloudinaryService = new CloudinaryService();
//# sourceMappingURL=cloudinaryService.js.map