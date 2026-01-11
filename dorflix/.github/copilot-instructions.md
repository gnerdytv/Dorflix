<!-- Copilot instructions for contributors and AI assistants -->

## Quick summary

- Expo app (SDK ~54) using **expo-router** (file-based routing). Main app lives in `app/`; `app-example/` contains canonical examples (tabs, modal, color scheme, etc.).
- Primary patterns: file-based routes (`app/`), small reusable UI in `components/`, and lightweight hooks (no global store).
- TypeScript is enabled with **strict** mode and a path alias `@/*` → project root (see `tsconfig.json`).

## Key files & places to check 🔎

- Routes & navigation: `app/_layout.tsx`, `app/index.tsx`, `app/video/[id].tsx` (example). Use `app-example/` as the canonical pattern.
- UI components: `components/` — e.g. `components/VideoCard.tsx` (video lifecycle logic, status listener, playback retry).
- Example hooks: `app-example/hooks/` (e.g., `use-color-scheme.web.ts` handles hydration and web specifics).
- Config & build: `package.json`, `eas.json` (EAS build profiles), `tsconfig.json` (alias `@`), `README.md`.

## Developer workflows & commands ⚙️

- Install dependencies: `npm install`
- Local dev server: `npx expo start` (opens Metro / Expo Dev Tools)
- Run on device/emulator: `npm run android` / `npm run ios` / `npm run web`
- Linting: `npm run lint` (uses Expo/ESLint config)
- Dev clients & EAS: project includes `expo-dev-client` and `eas.json` (CLI >= 16.28.0 required). Use `eas build` / `eas submit` per EAS docs when working with custom native modules.
- Note: `package.json` includes `reset-project` script, but the actual file is at `app-example/scripts/reset-project.js` in this repo — check that file for its behavior before running.

## Project-specific conventions & gotchas ⚠️

- Routing: add a file under `app/` (e.g., `app/video/[id].tsx`) and export the default screen component. Use `app-example/` to copy patterns (modal presentation, tab nesting, `unstable_settings.anchor`).
- Alias imports: source files may use `@/path` (configured in `tsconfig.json`). Keep imports consistent to avoid resolver issues.
- Reanimated: `react-native-reanimated` is present; note the codebase imports `'react-native-reanimated'` in top-level layout (see `app-example/app/_layout.tsx`) — ensure it's imported early when adding Reanimated-based animations.
- Video playback pattern: prefer the lifecycle used in `components/VideoCard.tsx`:
  - Use `useVideoPlayer(videoUri)` and `VideoView`.
  - Mute on web (autoplay policies) with `Platform.OS === 'web'` and set `player.loop = true` where appropriate.
  - Watch `statusChange` listener for `readyToPlay` and `error` events; call `player.play()` only when ready. Use `replaceAsync` for retries.
  - The component already logs status changes — follow that pattern for debugging.
- State & data flow: the app uses local state + hooks (no global Redux/Context found). For cross-screen data, prefer route params and lightweight hooks.

## Debugging tips 🔧

- For video playback issues, check Metro logs (terminal), device logs (`adb logcat` for Android), and the `console.log` lines in `components/VideoCard.tsx` that output player `status` and `error` objects.
- If adding native modules, build a development client with `eas build --profile development` or use `expo run:android` / `expo run:ios` locally.

## Example prompt templates for small tasks 💡

- "Add `app/video/[id].tsx` route that fetches metadata (mock fetch) and renders `components/VideoCard`. Follow `app-example/(tabs)/index.tsx` for layout and navigation."
- "Investigate intermittent playback error in `components/VideoCard.tsx`. Reproduce with web build, log `payload.error`, and add a graceful retry/backoff when `replaceAsync` fails."
- "Refactor `components/VideoCard.tsx` to extract playback lifecycle into a `useVideoPlayback` hook that exposes `play`, `pause`, `error`, and `isReady` while keeping current external API the same."

## What not to assume 🚫

- There is no test runner or CI test config present — do not add tests or CI steps without confirmation.
- Avoid suggestions that require ejecting from Expo unless explicitly asked (use `expo-dev-client` / EAS for native modules).

---

If you'd like, I can: (1) add a one-line PR checklist (lint + run locally + smoke test video playback) or (2) open a PR that fixes the `reset-project` script path discrepancy. Tell me which you'd prefer and I'll proceed.
