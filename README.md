# Dorflix - Movie Link Redirector

A static GitHub Pages site that generates per-movie shareable links. Each link shows the movie name in the URL. When clicked, it redirects to the vidoza.net movie page (same tab) and opens the zone advertisement URL (new tab).

## URL Format

```
https://gnerdytv.github.io/Dorflix/?slug=tomorrow-war-2021-3xn34w5bwdk5
```

- `slug` = movie-name-in-kebab-case + `-` + vidoza filecode
- Example: `?slug=tomorrow-war-2021-3xn34w5bwdk5` → Movie: "Tomorrow War 2021", Filecode: `3xn34w5bwdk5`

## How to Deploy

1. Push this repository to `github.com/gnerdytv/Dorflix`
2. Go to **Settings → Pages** in your GitHub repo
3. Set source to **Deploy from a branch**, branch `main`, folder `/ (root)`
4. Your site will be live at `https://gnerdytv.github.io/Dorflix/`

## How to Add a Movie

1. Upload your movie to [vidoza.net](https://vidoza.net) → get the filecode (e.g., `3xn34w5bwdk5`)
2. Edit `database.json` and add a new entry:

```json
{
  "id": "3xn34w5bwdk5",
  "name": "Tomorrow War 2021",
  "slug": "tomorrow-war-2021-3xn34w5bwdk5",
  "vidozaUrl": "https://vidoza.net/3xn34w5bwdk5.html",
  "zoneUrl": "https://motionless-bus.com/CDgc7m"
}
```

3. Commit and push to GitHub
4. Post your link: `https://gnerdytv.github.io/Dorflix/?slug=tomorrow-war-2021-3xn34w5bwdk5`

## Vidoza API

API key: `ovtgyju9e9dhb2g7565tsdqgdfnxpadhtksxktq4cbqkzt1rghwwtdcr0ing`

- **Base URL**: `https://api.vidoza.net/v1`
- **Auth**: `Authorization: Bearer <API_KEY>`
- **Endpoints**:
  - `GET /files/check?f[]=filecode` — Check file status
  - `GET /folders` — List folders
  - `GET /folders/{id}` — List folder contents