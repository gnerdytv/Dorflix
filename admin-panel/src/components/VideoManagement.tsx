import React, { useState, useCallback } from "react";
import { getVideos, deleteVideo } from "../services/api";
import "./VideoManagement.css";

interface Video {
  id: string;
  title: string;
  description?: string;
  video_url: string;
  thumbnail_url?: string;
  duration: number;
  uploader_name: string;
  uploader_avatar_url?: string;
  views_count: number;
  likes_count: number;
  comments_count: number;
  shares_count: number;
  created_at: string;
  is_active: boolean;
}

const VideoManagement: React.FC = () => {
  const [videos, setVideos] = useState<Video[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [searchTerm, setSearchTerm] = useState("");
  const [currentPage, setCurrentPage] = useState(1);
  const [totalVideos, setTotalVideos] = useState(0);
  const [deleteConfirmation, setDeleteConfirmation] = useState<string | null>(
    null
  );
  const videosPerPage = 10;

  const fetchVideos = useCallback(async () => {
    setLoading(true);
    setError(null);
    try {
      const response = await getVideos({
        limit: videosPerPage,
        offset: (currentPage - 1) * videosPerPage,
        sortBy: "created_at",
        sortOrder: "desc",
      });
      setVideos(response.data.videos);
      setTotalVideos(response.data.total);
    } catch (err: any) {
      console.error("Fetch videos error:", err);
      setError("Failed to fetch videos");
    } finally {
      setLoading(false);
    }
  }, [currentPage, videosPerPage]);

  const handleDelete = async (videoId: string) => {
    setDeleteConfirmation(videoId);
  };

  const confirmDelete = async () => {
    if (!deleteConfirmation) return;

    try {
      await deleteVideo(deleteConfirmation);
      setVideos(videos.filter((video) => video.id !== deleteConfirmation));
      setDeleteConfirmation(null);
    } catch (err: any) {
      console.error("Delete video error:", err);
      setError("Failed to delete video");
    }
  };

  const cancelDelete = () => {
    setDeleteConfirmation(null);
  };

  const handleSearch = async () => {
    if (!searchTerm.trim()) {
      fetchVideos();
      return;
    }

    setLoading(true);
    setError(null);
    try {
      const response = await getVideos({
        limit: videosPerPage,
        offset: 0,
        sortBy: "created_at",
        sortOrder: "desc",
      });

      // Filter videos on client side for now
      const filteredVideos = response.data.videos.filter((video: Video) =>
        video.title.toLowerCase().includes(searchTerm.toLowerCase())
      );

      setVideos(filteredVideos);
      setTotalVideos(filteredVideos.length);
      setCurrentPage(1);
    } catch (err: any) {
      console.error("Search videos error:", err);
      setError("Failed to search videos");
    } finally {
      setLoading(false);
    }
  };

  const formatDuration = (seconds: number): string => {
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = Math.floor(seconds % 60);

    if (hours > 0) {
      return `${hours}:${minutes.toString().padStart(2, "0")}:${secs
        .toString()
        .padStart(2, "0")}`;
    }
    return `${minutes}:${secs.toString().padStart(2, "0")}`;
  };

  const formatDate = (dateString: string): string => {
    return new Date(dateString).toLocaleDateString("en-US", {
      year: "numeric",
      month: "long",
      day: "numeric",
      hour: "2-digit",
      minute: "2-digit",
    });
  };

  const totalPages = Math.ceil(totalVideos / videosPerPage);

  return (
    <div className="video-management">
      <div className="management-header">
        <h2>Video Management</h2>
        <div className="search-container">
          <input
            type="text"
            placeholder="Search videos by title..."
            value={searchTerm}
            onChange={(e) => setSearchTerm(e.target.value)}
            onKeyPress={(e) => e.key === "Enter" && handleSearch()}
          />
          <button onClick={handleSearch} disabled={loading}>
            Search
          </button>
          <button
            onClick={() => {
              setSearchTerm("");
              fetchVideos();
            }}
            disabled={loading}
          >
            Clear
          </button>
        </div>
      </div>

      {error && (
        <div className="error-message">
          <span>{error}</span>
          <button onClick={() => setError(null)}>×</button>
        </div>
      )}

      {deleteConfirmation && (
        <div className="delete-confirmation">
          <div className="confirmation-modal">
            <h3>Confirm Delete</h3>
            <p>
              Are you sure you want to delete this video? This action cannot be
              undone.
            </p>
            <div className="confirmation-actions">
              <button onClick={confirmDelete} className="confirm-btn">
                Yes, Delete
              </button>
              <button onClick={cancelDelete} className="cancel-btn">
                Cancel
              </button>
            </div>
          </div>
        </div>
      )}

      {loading ? (
        <div className="loading">
          <div className="spinner"></div>
          <p>Loading videos...</p>
        </div>
      ) : (
        <>
          <div className="videos-grid">
            {videos.length === 0 ? (
              <div className="no-videos">
                <p>No videos found</p>
              </div>
            ) : (
              videos.map((video) => (
                <div key={video.id} className="video-card">
                  <div className="video-thumbnail">
                    {video.thumbnail_url ? (
                      <img src={video.thumbnail_url} alt={video.title} />
                    ) : (
                      <div className="placeholder-thumbnail">
                        <span>Video</span>
                      </div>
                    )}
                    <div className="duration-overlay">
                      {formatDuration(video.duration)}
                    </div>
                  </div>

                  <div className="video-info">
                    <h3>{video.title}</h3>
                    {video.description && (
                      <p className="description">{video.description}</p>
                    )}

                    <div className="video-stats">
                      <span>👤 {video.uploader_name}</span>
                      <span>👁️ {video.views_count} views</span>
                      <span>❤️ {video.likes_count} likes</span>
                      <span>💬 {video.comments_count} comments</span>
                      <span>📤 {video.shares_count} shares</span>
                    </div>

                    <div className="video-meta">
                      <span>📅 {formatDate(video.created_at)}</span>
                      <span>⏱️ {formatDuration(video.duration)}</span>
                      <span
                        className={`status ${
                          video.is_active ? "active" : "inactive"
                        }`}
                      >
                        {video.is_active ? "Active" : "Inactive"}
                      </span>
                    </div>

                    <div className="video-actions">
                      <button
                        className="view-btn"
                        onClick={() => window.open(video.video_url, "_blank")}
                      >
                        View Video
                      </button>
                      <button
                        className="delete-btn"
                        onClick={() => handleDelete(video.id)}
                      >
                        Delete
                      </button>
                    </div>
                  </div>
                </div>
              ))
            )}
          </div>

          {videos.length > 0 && (
            <div className="pagination">
              <button
                onClick={() => setCurrentPage(currentPage - 1)}
                disabled={currentPage === 1 || loading}
              >
                Previous
              </button>

              <span className="page-info">
                Page {currentPage} of {totalPages} • Showing {videos.length} of{" "}
                {totalVideos} videos
              </span>

              <button
                onClick={() => setCurrentPage(currentPage + 1)}
                disabled={currentPage === totalPages || loading}
              >
                Next
              </button>
            </div>
          )}
        </>
      )}
    </div>
  );
};

export default VideoManagement;
