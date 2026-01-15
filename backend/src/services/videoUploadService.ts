import { VideoService } from "./videoService";
import { cloudinaryService, CloudinaryUploadResult } from "./cloudinaryService";
import multer from "multer";
import path from "path";
import fs from "fs";

export interface VideoUploadData {
  title: string;
  description?: string;
  uploaderName: string;
  uploaderAvatarUrl?: string;
}

export class VideoUploadService {
  private upload: multer.Multer;

  constructor() {
    // Configure multer for video uploads
    const storage = multer.diskStorage({
      destination: (req, file, cb) => {
        const uploadDir = "uploads/";
        if (!fs.existsSync(uploadDir)) {
          fs.mkdirSync(uploadDir, { recursive: true });
        }
        cb(null, uploadDir);
      },
      filename: (req, file, cb) => {
        const uniqueSuffix = Date.now() + "-" + Math.round(Math.random() * 1e9);
        cb(
          null,
          file.fieldname + "-" + uniqueSuffix + path.extname(file.originalname)
        );
      },
    });

    this.upload = multer({
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
        } else {
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
  async uploadVideo(
    file: Express.Multer.File,
    uploadData: VideoUploadData
  ): Promise<{ video: any; uploadResult: CloudinaryUploadResult }> {
    try {
      // Upload to Cloudinary
      const uploadResult = await cloudinaryService.uploadVideo(
        file.buffer || fs.readFileSync(file.path),
        file.originalname
      );

      // Create video record in database
      const video = await VideoService.createVideo({
        title: uploadData.title,
        description: uploadData.description,
        videoUrl: uploadResult.secure_url,
        thumbnailUrl: cloudinaryService.generateThumbnailUrl(
          uploadResult.public_id
        ),
        duration: Math.round(uploadResult.duration || 0),
        uploaderName: uploadData.uploaderName,
        uploaderAvatarUrl: uploadData.uploaderAvatarUrl,
      });

      // Clean up local file
      if (file.path) {
        fs.unlinkSync(file.path);
      }

      return { video, uploadResult };
    } catch (error) {
      // Clean up local file on error
      if (file.path && fs.existsSync(file.path)) {
        fs.unlinkSync(file.path);
      }
      throw error;
    }
  }

  /**
   * Get Cloudinary configuration for frontend
   */
  getCloudinaryConfig() {
    return cloudinaryService.getCloudinaryConfig();
  }

  /**
   * Delete video from Cloudinary and database
   */
  async deleteVideo(videoId: string): Promise<void> {
    try {
      // Get video from database
      const video = await VideoService.getVideoById(videoId);
      if (!video) {
        throw new Error("Video not found");
      }

      // Extract public ID from Cloudinary URL
      const publicId = this.extractPublicIdFromUrl(video.video_url);
      if (publicId) {
        // Delete from Cloudinary
        await cloudinaryService.deleteVideo(publicId);
      }

      // Delete from database (soft delete)
      await VideoService.deleteVideo(videoId);
    } catch (error) {
      console.error("Delete video error:", error);
      throw new Error("Failed to delete video");
    }
  }

  /**
   * Extract public ID from Cloudinary URL
   */
  private extractPublicIdFromUrl(url: string): string | null {
    try {
      const urlParts = url.split("/");
      const videoIndex = urlParts.indexOf("video");
      if (videoIndex !== -1 && urlParts[videoIndex + 1]) {
        return urlParts[videoIndex + 1].split(".")[0];
      }
      return null;
    } catch (error) {
      return null;
    }
  }
}

export const videoUploadService = new VideoUploadService();
