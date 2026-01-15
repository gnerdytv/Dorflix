import React, { useState } from "react";
import VideoUpload from "./components/VideoUpload";
import VideoManagement from "./components/VideoManagement";
import "./App.css";

function App() {
  const [activeTab, setActiveTab] = useState<"upload" | "manage">("upload");

  const handleUploadSuccess = (video: any) => {
    console.log("Video uploaded successfully:", video);
    // You could show a success message or refresh the video list
  };

  return (
    <div className="App">
      <header className="app-header">
        <h1>Dorflix Admin Panel</h1>
        <nav className="nav-tabs">
          <button
            className={`tab ${activeTab === "upload" ? "active" : ""}`}
            onClick={() => setActiveTab("upload")}
          >
            Upload Video
          </button>
          <button
            className={`tab ${activeTab === "manage" ? "active" : ""}`}
            onClick={() => setActiveTab("manage")}
          >
            Manage Videos
          </button>
        </nav>
      </header>

      <main className="app-main">
        {activeTab === "upload" && (
          <VideoUpload onUploadSuccess={handleUploadSuccess} />
        )}
        {activeTab === "manage" && <VideoManagement />}
      </main>

      <footer className="app-footer">
        <p>© 2025 Dorflix Admin Panel</p>
      </footer>
    </div>
  );
}

export default App;
