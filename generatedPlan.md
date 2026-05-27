# Dorflix - Movie Link Redirector

## Overview
A static GitHub Pages site hosted at `gnerdytv.github.io/Dorflix` that generates per-movie shareable links. Each link contains the movie name in the URL for readability. When clicked, it redirects the user to the actual vidoza.net movie URL (same tab) and opens the zone advertisement URL (new tab).

## URL Format
```
gnerdytv.github.io/Dorflix/?slug=tomorrow-war-2021-3xn34w5bwdk5
```
- `slug` = movie-name-in-kebab-case + "-" + vidoza filecode
- User sees the movie name in the URL before clicking
- JavaScript parses the filecode from the slug end to look up in database

## How It Works
1. User posts link under their short video/poster: `gnerdytv.github.io/Dorflix/?slug=tomorrow-war-2021-3xn34w5bwdk5`
2. Viewer clicks the link
3. Page loads, JavaScript reads `?slug=` from URL
4. Extracts filecode from slug, looks up in `database.json`
5. Displays movie info with a "Proceed" button
6. When user clicks "Proceed":
   - **Same tab**: navigates to `https://vidoza.net/3xn34w5bwdk5.html`
   - **New tab**: opens `https://motionless-bus.com/CDgc7m`

## File Structure
```
Dorflix-pages/
├── generatedPlan.md       # This plan document
├── index.html             # Main page - looks up movie, displays info, handles redirect
├── style.css              # Dark Netflix-style theme
├── script.js              # Slug parsing, database lookup, redirect logic
├── database.json          # Movie database (user edits to add movies)
└── README.md              # Setup instructions
```

## Database Format (`database.json`)
```json
[
  {
    "id": "3xn34w5bwdk5",
    "name": "Tomorrow War 2021",
    "slug": "tomorrow-war-2021-3xn34w5bwdk5",
    "vidozaUrl": "https://vidoza.net/3xn34w5bwdk5.html",
    "zoneUrl": "https://motionless-bus.com/CDgc7m"
  }
]
```

## Workflow to Add a Movie
1. Upload movie to vidoza.net → get filecode (e.g., `3xn34w5bwdk5`)
2. Use Vidoza API key (`ovtgyju9e9dhb2g7565tsdqgdfnxpadhtksxktq4cbqkzt1rghwwtdcr0ing`) to get file info
3. Add entry to `database.json` with the filecode, movie name, slug, vidoza URL, and zone URL
4. Push changes to GitHub → site auto-updates via GitHub Pages
5. Post the link under your content

## Vidoza API (for reference)
- **Base URL**: `https://api.vidoza.net/v1`
- **Auth**: `Authorization: Bearer <API_KEY>`
- **Key**: `ovtgyju9e9dhb2g7565tsdqgdfnxpadhtksxktq4cbqkzt1rghwwtdcr0ing`
- **Endpoints**:
  - `GET /files/check?f[]=filecode` - Check file status
  - `GET /folders` - List folders
  - `GET /folders/{id}` - List folder contents