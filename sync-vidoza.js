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
 * Recursively list all files from all folders via the Vidoza API.
 */
async function listAllFiles() {
  var allFiles = [];

  // Step 1: Get all folders
  console.log('Fetching folders...');
  var foldersRes = await apiGet('/folders');
  var folders = [];

  if (foldersRes.data && Array.isArray(foldersRes.data)) {
    folders = foldersRes.data;
    console.log('  Found ' + folders.length + ' folders.');
  } else {
    console.log('  No folders found or could not list them.');
  }

  // Step 2: Get files from root (folder_id = 0 or empty)
  console.log('Fetching root files...');
  var rootRes = await apiGet('/files/check');
  // The API requires specific file codes to check, so we need to handle differently.
  // Instead, use /folders endpoint to list folder contents.

  // Step 3: Get files from each folder
  for (var i = 0; i < folders.length; i++) {
    var folder = folders[i];
    var folderId = folder.id || folder.folder_id;
    if (!folderId) continue;

    console.log('  Fetching folder: ' + (folder.name || folderId) + ' (id: ' + folderId + ')');
    try {
      var folderRes = await apiGet('/folders/' + folderId);
      if (folderRes.data && Array.isArray(folderRes.data)) {
        for (var j = 0; j < folderRes.data.length; j++) {
          var file = folderRes.data[j];
          // Files in folder listing have: id, name, size, created, etc.
          if (file.id) {
            allFiles.push(file);
          }
        }
        console.log('    -> ' + folderRes.data.length + ' files');
      }
    } catch (e) {
      console.error('    Error fetching folder ' + folderId + ': ' + e.message);
    }
  }

  return allFiles;
}

/**
 * Check the status of a list of file codes via the API.
 * The API only allows checking specific file codes, not listing all.
 * So we batch the files we found from folder listings.
 */
async function checkFileStatus(filecodes) {
  var results = [];

  // API accepts multiple file codes: ?f[]=code1&f[]=code2
  // But to avoid huge URLs, batch in groups of 50
  var batchSize = 50;
  for (var i = 0; i < filecodes.length; i += batchSize) {
    var batch = filecodes.slice(i, i + batchSize);
    var query = batch.map(function(code) {
      return 'f[]=' + encodeURIComponent(code);
    }).join('&');

    try {
      var res = await apiGet('/files/check?' + query);
      if (res.data && Array.isArray(res.data)) {
        results = results.concat(res.data);
        console.log('  Checked ' + results.length + ' / ' + filecodes.length + ' files');
      }
    } catch (e) {
      console.error('  Error checking batch: ' + e.message);
    }
  }

  return results;
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

  // Get files from folders (the API doesn't have a "list all files" endpoint)
  var folderFiles = await listAllFiles();

  if (folderFiles.length === 0) {
    console.log('\nNo files found in folders.');
    console.log('If you have files in your Vidoza account, try organizing them into folders first.');
    console.log('Then run this script again.');
  } else {
    // Get detailed status for these files
    var filecodes = folderFiles.map(function(f) { return f.id; });
    console.log('\nChecking file status for ' + filecodes.length + ' files...');
    var checkedFiles = await checkFileStatus(filecodes);

    // Find new files
    var newEntries = [];
    for (var k = 0; k < checkedFiles.length; k++) {
      var file = checkedFiles[k];
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