import cloudinary from "cloudinary";
import dotenv from "dotenv";

dotenv.config();

// Configure Cloudinary
cloudinary.v2.config({
  cloud_name: process.env.CLOUDINARY_CLOUD_NAME,
  api_key: process.env.CLOUDINARY_API_KEY,
  api_secret: process.env.CLOUDINARY_API_SECRET,
});

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

class CloudinaryService {
  private folder: string = process.env.CLOUDINARY_FOLDER || "dorflix_videos";

  /**
   * Upload video to Cloudinary
   */
  async uploadVideo(
    fileBuffer: Buffer,
    fileName: string
  ): Promise<CloudinaryUploadResult> {
    return new Promise((resolve, reject) => {
      cloudinary.v2.uploader
        .upload_stream(
          {
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
          },
          (error, result) => {
            if (error) {
              reject(error);
            } else if (result) {
              resolve({
                public_id: result.public_id,
                secure_url: result.secure_url,
                duration: result.duration,
                width: result.width,
                height: result.height,
                format: result.format || "mp4",
                original_filename: result.original_filename || fileName,
              });
            } else {
              reject(new Error("Upload failed: No result returned"));
            }
          }
        )
        .end(fileBuffer);
    });
  }

  /**
   * Get video information from Cloudinary
   */
  async getVideoInfo(publicId: string): Promise<CloudinaryVideoInfo> {
    return new Promise((resolve, reject) => {
      cloudinary.v2.api.resource(
        publicId,
        { resource_type: "video" },
        (error, result) => {
          if (error) {
            reject(error);
          } else if (result) {
            resolve({
              duration: result.duration || 0,
              width: result.width || 0,
              height: result.height || 0,
              format: result.format || "mp4",
              original_filename: result.original_filename || "unknown",
            });
          } else {
            reject(new Error("Video not found"));
          }
        }
      );
    });
  }

  /**
   * Delete video from Cloudinary
   */
  async deleteVideo(publicId: string): Promise<void> {
    return new Promise((resolve, reject) => {
      cloudinary.v2.uploader.destroy(
        publicId,
        { resource_type: "video" },
        (error, result) => {
          if (error) {
            reject(error);
          } else {
            resolve();
          }
        }
      );
    });
  }

  /**
   * Generate video thumbnail URL
   */
  generateThumbnailUrl(publicId: string, width: number = 300): string {
    return cloudinary.v2.url(publicId, {
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
  generateStreamingUrls(publicId: string): {
    hls: string;
    mp4: string;
    webm: string;
  } {
    return {
      hls: cloudinary.v2.url(publicId, {
        resource_type: "video",
        format: "m3u8",
        transformation: [
          {
            streaming_profile: "full_hd",
          },
        ],
      }),
      mp4: cloudinary.v2.url(publicId, {
        resource_type: "video",
        format: "mp4",
        transformation: [
          {
            quality: "auto",
            fetch_format: "auto",
          },
        ],
      }),
      webm: cloudinary.v2.url(publicId, {
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

export const cloudinaryService = new CloudinaryService();
