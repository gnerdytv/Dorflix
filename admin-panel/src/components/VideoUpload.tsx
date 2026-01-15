import React, { useState } from "react";
import { useDropzone } from "react-dropzone";
import axios from "axios";
import "./VideoUpload.css";

interface VideoUploadProps {
  onUploadSuccess: (video: any) => void;
}

const VideoUpload: React.FC<VideoUploadProps> = ({ onUploadSuccess }) => {
  const [isUploading, setIsUploading] = useState(false);
  const [uploadProgress, setUploadProgress] = useState(0);
  const [formData, setFormData] = useState({
    title: "",
    description: "",
    uploaderName: "Admin",
    uploaderAvatarUrl: "",
  });
  const [selectedFile, setSelectedFile] = useState<File | null>(null);
  const [error, setError] = useState<string | null>(null);

  const onDrop = (acceptedFiles: File[]) => {
    if (acceptedFiles.length > 0) {
      setSelectedFile(acceptedFiles[0]);
      setError(null);
    }
  };

  const { getRootProps, getInputProps, isDragActive } = useDropzone({
    onDrop,
    accept: {
      "video/*": [".mp4", ".mov", ".avi", ".mkv", ".webm", ".flv"],
    },
    maxSize: 1024 * 1024 * 1024, // 1GB limit
    multiple: false,
  });

  const handleInputChange = (
    e: React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>
  ) => {
    const { name, value } = e.target;
    setFormData((prev) => ({
      ...prev,
      [name]: value,
    }));
  };

  const handleUpload = async () => {
    if (!selectedFile) {
      setError("Please select a video file to upload");
      return;
    }

    if (!formData.title || !formData.uploaderName) {
      setError("Title and uploader name are required");
      return;
    }

    setIsUploading(true);
    setUploadProgress(0);
    setError(null);

    try {
      const formDataObj = new FormData();
      formDataObj.append("video", selectedFile);
      formDataObj.append("title", formData.title);
      formDataObj.append("description", formData.description);
      formDataObj.append("uploaderName", formData.uploaderName);
      formDataObj.append("uploaderAvatarUrl", formData.uploaderAvatarUrl);

      const response = await axios.post(
        "http://localhost:3001/api/upload/video",
        formDataObj,
        {
          headers: {
            "Content-Type": "multipart/form-data",
          },
          onUploadProgress: (progressEvent) => {
            const progress = Math.round(
              (progressEvent.loaded * 100) / progressEvent.total!
            );
            setUploadProgress(progress);
          },
        }
      );

      onUploadSuccess(response.data.video);
      // Reset form
      setFormData({
        title: "",
        description: "",
        uploaderName: "Admin",
        uploaderAvatarUrl: "",
      });
      setSelectedFile(null);
      setUploadProgress(0);
    } catch (err: any) {
      console.error("Upload error:", err);
      setError(err.response?.data?.message || "Upload failed");
    } finally {
      setIsUploading(false);
    }
  };

  const formatFileSize = (bytes: number): string => {
    if (bytes === 0) return "0 Bytes";
    const k = 1024;
    const sizes = ["Bytes", "KB", "MB", "GB"];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + " " + sizes[i];
  };

  return (
    <div className="video-upload">
      <h2>Upload Video</h2>

      {error && (
        <div className="error-message">
          <span>{error}</span>
          <button onClick={() => setError(null)}>×</button>
        </div>
      )}

      <div className="form-group">
        <label htmlFor="title">Video Title *</label>
        <input
          type="text"
          id="title"
          name="title"
          value={formData.title}
          onChange={handleInputChange}
          placeholder="Enter video title"
          disabled={isUploading}
        />
      </div>

      <div className="form-group">
        <label htmlFor="description">Description</label>
        <textarea
          id="description"
          name="description"
          value={formData.description}
          onChange={handleInputChange}
          placeholder="Enter video description (optional)"
          rows={4}
          disabled={isUploading}
        />
      </div>

      <div className="form-group">
        <label htmlFor="uploaderName">Uploader Name *</label>
        <input
          type="text"
          id="uploaderName"
          name="uploaderName"
          value={formData.uploaderName}
          onChange={handleInputChange}
          placeholder="Enter uploader name"
          disabled={isUploading}
        />
      </div>

      <div className="form-group">
        <label htmlFor="uploaderAvatarUrl">Uploader Avatar URL</label>
        <input
          type="url"
          id="uploaderAvatarUrl"
          name="uploaderAvatarUrl"
          value={formData.uploaderAvatarUrl}
          onChange={handleInputChange}
          placeholder="Enter avatar URL (optional)"
          disabled={isUploading}
        />
      </div>

      <div
        {...getRootProps()}
        className={`dropzone ${isDragActive ? "drag-active" : ""} ${
          selectedFile ? "file-selected" : ""
        }`}
      >
        <input {...getInputProps()} />
        {selectedFile ? (
          <div className="file-info">
            <div className="file-details">
              <span className="file-name">{selectedFile.name}</span>
              <span className="file-size">
                {formatFileSize(selectedFile.size)}
              </span>
            </div>
            <button
              type="button"
              className="remove-file-btn"
              onClick={(e) => {
                e.stopPropagation();
                setSelectedFile(null);
              }}
              disabled={isUploading}
            >
              Remove
            </button>
          </div>
        ) : (
          <div className="dropzone-content">
            <div className="upload-icon">📁</div>
            <p>
              {isDragActive
                ? "Drop the video file here..."
                : "Drag & drop a video file here, or click to select one"}
            </p>
            <p className="file-hint">
              Supported formats: MP4, MOV, AVI, MKV, WebM, FLV (Max 1GB)
            </p>
          </div>
        )}
      </div>

      {selectedFile && (
        <div className="upload-controls">
          <div className="progress-bar">
            <div
              className="progress-fill"
              style={{ width: `${uploadProgress}%` }}
            ></div>
          </div>
          <div className="progress-text">{uploadProgress}%</div>
        </div>
      )}

      <div className="upload-actions">
        <button
          type="button"
          className="upload-btn"
          onClick={handleUpload}
          disabled={isUploading || !selectedFile || !formData.title}
        >
          {isUploading ? (
            <>
              <span className="spinner"></span>
              Uploading...
            </>
          ) : (
            "Upload Video"
          )}
        </button>
      </div>

      {isUploading && (
        <div className="upload-status">
          <p>Uploading video to Cloudinary and processing...</p>
          <p>Please don't close this window until the upload is complete.</p>
        </div>
      )}
    </div>
  );
};

export default VideoUpload;
