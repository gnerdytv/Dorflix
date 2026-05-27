#!/usr/bin/env node

/**
 * sync-vidoza.js
 *
 * Scans the Vidoza API for all files, compares with database.json,
 * and appends new movies that don't exist yet. Then auto-commits
 * and pushes to GitHub.
 *
 * Usage:
 *   VIDOZA_API_KEY=your_key node sync-vidoza.js
 *
 * Environment variables:
 *   VIDOZA_API_KEY  - (required) Your Vidoza API key
 *   GITHUB_TOKEN    - (required) GitHub personal access token for push
 *   ZONE_URL        - (optional) Zone advertisement URL (has default)
 */

'use strict';

const https = require('https');
const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

/**
 * Load .env file and set process.env variables.
 * This avoids needing external dotenv package.
 */
function loadEnvFile() {
  var envPath = path.join(__dirname, '.env');
  try {
    if (!fs.existsSync(envPath)) {
      return;
    }
    var content = fs.readFileSync(envPath, 'utf-8');
    var lines = content.split('\n');
    for (var i = 0; i < lines.length; i++) {
      var line = lines[i].trim();
      // Skip empty lines and comments
      if (line === '' || line.startsWith('#')) {
        continue;
      }
      var eqIndex = line.indexOf('=');
      if (eqIndex === -1) {
        continue;
      }
      var key = line.substring(0, eqIndex).trim();
      var val = line.substring(eqIndex + 1).trim();
      // Only set if not already set in environment
      if (!process.env[key]) {
        process.env[key] = val;
      }
    }
  } catch (e) {
    // Silently ignore .env file errors
  }
}

// Load .env file before reading config
loadEnvFile();

// ---- CONFIG ----
const API_KEY = process.env.VIDOZA_API_KEY;
const GITHUB_TOKEN = process.env.GITHUB_TOKEN;
const ZONE_URL = process.env.ZONE_URL || 'https://motionless-bus.com/CDgc7m';
const API_BASE = 'api.vidoza.net';
const DB_PATH = path.join(__dirname, 'database.json');
const REPO_DIR = __dirname;
// ----------------

/**
 * Make an HTTPS GET request and parse JSON response.
 */
function apiGet(path) {
  return new Promise((resolve, reject) => {
    const options = {
      hostname: API_BASE,
      path: '/v1' + path,
      method: 'GET',
      headers: {
        'Authorization': 'Bearer ' + API_KEY,
        'Accept': 'application/json',
      },
    };

    const req = https.request(options, (res) => {
      let data = '';
      res.on('data', (chunk) => { data += chunk; });
      res.on('end', () => {
        try {
          const parsed = JSON.parse(data);
          resolve(parsed);
        } catch (e) {
          reject(new Error('Failed to parse API response: ' + data.substring(0, 200)));
        }
      });
    });

    req.on('error', reject);
    req.end();
  });
}

/**
 * Load the current database.json file.
 * Returns an empty array if file doesn't exist or is invalid.
 */
function loadDatabase() {
  try {
    if (!fs.existsSync(DB_PATH)) {
      return [];
    }
    const raw = fs.readFileSync(DB_PATH, 'utf-8');
    return JSON.parse(raw);
  } catch (e) {
    console.error('Warning: Could not read database.json, starting fresh.');
    return [];
  }
}

/**
 * Save the database to database.json.
 */
function saveDatabase(db) {
  fs.writeFileSync(DB_PATH, JSON.stringify(db, null, 2) + '\n', 'utf-8');
}

/**
 * Generate a slug from a movie name and filecode.
 * Converts "Tomorrow War 2021" + "3xn34w5bwdk5" -> "tomorrow-war-2021-3xn34w5bwdk5"
 */
function generateSlug(name, filecode) {
  var slug = name
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, '-')
    .replace(/^-|-$/g, '');
  return slug + '-' + filecode;
}

/**
 * List all files via the Vidoza API with pagination.
 * Uses GET /v1/files which supports ?page=N for pagination.
 */
async function listAllFiles() {
  var allFiles = [];
  var page = 1;
  var lastPage = 1;

  console.log('Fetching files from Vidoza...');

  do {
    try {
      var res = await apiGet('/files?page=' + page);
      if (res.data && Array.isArray(res.data)) {
        for (var j = 0; j < res.data.length; j++) {
          if (res.data[j].id) {
            allFiles.push(res.data[j]);
          }
        }
        if (res.meta) {
          lastPage = res.meta.last_page || page;
        }
        console.log('  Page ' + page + '/' + lastPage + ': found ' + res.data.length + ' files');
      } else {
        console.log('  No files found on page ' + page);
        break;
      }
    } catch (e) {
      console.error('  Error fetching page ' + page + ': ' + e.message);
      break;
    }
    page++;
  } while (page <= lastPage);

  return allFiles;
}

/**
 * Main sync function.
 */
async function sync() {
  console.log('=== Dorflix Vidoza Sync ===');
  console.log('');

  // Load existing database
  var db = loadDatabase();
  var existingIds = {};
  for (var i = 0; i < db.length; i++) {
    existingIds[db[i].id] = true;
  }
  console.log('Existing movies in database.json: ' + db.length);

  // Get all files from Vidoza via /v1/files (paginated)
  var allVidozaFiles = await listAllFiles();

  if (allVidozaFiles.length === 0) {
    console.log('\nNo files found in your Vidoza account.');
  } else {
    // Find new files not yet in the database
    var newEntries = [];
    for (var k = 0; k < allVidozaFiles.length; k++) {
      var file = allVidozaFiles[k];
      if (!file.id) continue;

      if (!existingIds[file.id]) {
        var movieName = file.title || file.name || 'Unknown Movie';
        // Remove file extension from name if present
        if (file.name) {
          movieName = file.name.replace(/\.[^.]+$/, '');
        }
        if (file.title) {
          movieName = file.title;
        }

        var slug = generateSlug(movieName, file.id);
        var vidozaUrl = 'https://vidoza.net/' + file.id + '.html';

        newEntries.push({
          id: file.id,
          name: movieName,
          slug: slug,
          vidozaUrl: vidozaUrl,
          zoneUrl: ZONE_URL,
        });

        existingIds[file.id] = true; // prevent duplicates in same run
      }
    }

    // Report results
    if (newEntries.length > 0) {
      console.log('\nNew movies found: ' + newEntries.length);
      for (var m = 0; m < newEntries.length; m++) {
        console.log('  [' + (m + 1) + '] ' + newEntries[m].name + ' (id: ' + newEntries[m].id + ')');
      }

      // Append to database
      db = db.concat(newEntries);
      saveDatabase(db);
      console.log('\nAdded ' + newEntries.length + ' movie(s) to database.json');
    } else {
      console.log('\nNo new movies found. Database is up to date.');
    }
  }

  console.log('\nTotal movies in database.json: ' + db.length);

  // ---- GIT COMMIT & PUSH ----
  if (newEntries.length > 0 && GITHUB_TOKEN) {
    console.log('\n=== Pushing to GitHub ===');
    try {
      // Stage the database file
      execSync('git add database.json', { cwd: REPO_DIR });
      // Commit
      var count = newEntries.length;
      var commitMsg = 'Sync ' + count + ' new movie' + (count > 1 ? 's' : '') + ' from Vidoza';
      execSync('git commit -m "' + commitMsg + '"', { cwd: REPO_DIR });
      console.log('  Committed: ' + commitMsg);
      // Push using token as password
      var remoteUrl = 'https://dorflix:' + GITHUB_TOKEN + '@github.com/gnerdytv/Dorflix.git';
      execSync('git push ' + remoteUrl + ' main', { cwd: REPO_DIR });
      console.log('  Pushed to GitHub successfully.');
    } catch (e) {
      console.error('  Git error: ' + e.message);
      console.log('  You can manually push with: git push');
    }
  } else if (newEntries.length > 0 && !GITHUB_TOKEN) {
    console.log('\nGITHUB_TOKEN not set. Skipping git push.');
    console.log('Manually push with:');
    console.log('  git add database.json');
    console.log('  git commit -m "Add new movies from Vidoza"');
    console.log('  git push');
  }

  console.log('\n=== Sync complete ===');
}

// Run
sync().catch(function(err) {
  console.error('Sync failed:', err.message);
  process.exit(1);
});