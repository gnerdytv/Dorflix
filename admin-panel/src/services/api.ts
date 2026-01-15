import axios from "axios";

const API_BASE_URL = "http://192.168.0.101:3001/api";

export const api = axios.create({
  baseURL: API_BASE_URL,
  headers: {
    "Content-Type": "application/json",
  },
});

// Video upload API
export const uploadVideo = async (formData: FormData) => {
  return api.post("/upload/video", formData, {
    headers: {
      "Content-Type": "multipart/form-data",
    },
    onUploadProgress: (progressEvent) => {
      const progress = Math.round(
        (progressEvent.loaded * 100) / progressEvent.total!
      );
      console.log(`Upload progress: ${progress}%`);
    },
  });
};

// Get Cloudinary configuration
export const getCloudinaryConfig = async () => {
  return api.get("/upload/config");
};

// Delete video
export const deleteVideo = async (videoId: string) => {
  return api.delete(`/upload/video/${videoId}`);
};

// Get videos
export const getVideos = async (params?: {
  limit?: number;
  offset?: number;
  sortBy?: string;
  sortOrder?: string;
}) => {
  return api.get("/videos", { params });
};

// Search videos
export const searchVideos = async (
  searchTerm: string,
  params?: {
    limit?: number;
    offset?: number;
  }
) => {
  return api.get("/videos/search", {
    params: { q: searchTerm, ...params },
  });
};
