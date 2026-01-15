const { Pool } = require("pg");
const fs = require("fs");
const path = require("path");

// Load environment variables
require("dotenv").config();

const pool = new Pool({
  connectionString: process.env.DATABASE_URL,
});

async function seed() {
  try {
    console.log("Starting database seeding...");

    // Insert test videos
    const videos = [
      {
        title: "Test Video 1",
        description: "This is a test video for the catalog",
        video_url: "https://example.com/video1.mp4",
        thumbnail_url: "https://example.com/thumb1.jpg",
        duration: 180,
        uploader_name: "Test User",
        uploader_avatar_url: "https://example.com/avatar1.jpg",
        likes_count: 10,
        comments_count: 5,
        shares_count: 3,
        views_count: 100,
      },
      {
        title: "Test Video 2",
        description: "Another test video for the catalog",
        video_url: "https://example.com/video2.mp4",
        thumbnail_url: "https://example.com/thumb2.jpg",
        duration: 240,
        uploader_name: "Test User",
        uploader_avatar_url: "https://example.com/avatar1.jpg",
        likes_count: 15,
        comments_count: 8,
        shares_count: 5,
        views_count: 150,
      },
      {
        title: "Test Video 3",
        description: "Third test video for the catalog",
        video_url: "https://example.com/video3.mp4",
        thumbnail_url: "https://example.com/thumb3.jpg",
        duration: 300,
        uploader_name: "Test User",
        uploader_avatar_url: "https://example.com/avatar1.jpg",
        likes_count: 20,
        comments_count: 12,
        shares_count: 8,
        views_count: 200,
      },
    ];

    for (const video of videos) {
      await pool.query(
        `INSERT INTO videos (
          title, description, video_url, thumbnail_url, duration, 
          uploader_name, uploader_avatar_url, likes_count, comments_count, 
          shares_count, views_count
        ) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)`,
        [
          video.title,
          video.description,
          video.video_url,
          video.thumbnail_url,
          video.duration,
          video.uploader_name,
          video.uploader_avatar_url,
          video.likes_count,
          video.comments_count,
          video.shares_count,
          video.views_count,
        ]
      );
    }

    console.log("Database seeding completed successfully!");
  } catch (error) {
    console.error("Seeding failed:", error);
    process.exit(1);
  } finally {
    await pool.end();
  }
}

seed();
