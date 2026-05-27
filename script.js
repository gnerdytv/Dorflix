// Dorflix - Movie Link Redirector
// Two modes:
//   1. Homepage (no ?slug=) - lists all movies from database in a grid
//   2. Redirect (?slug=...) - shows movie info and handles redirect

(function() {
  'use strict';

  const ZONE_URL = 'https://motionless-bus.com/CDgc7m';

  const homepageEl = document.getElementById('homepage');
  const movieGridEl = document.getElementById('movie-grid');
  const appContainer = document.getElementById('app');
  const errorContainer = document.getElementById('error');
  const movieSection = document.getElementById('movie-section');

  /**
   * Read the slug from the URL query parameter.
   */
  function getSlugFromURL() {
    var params = new URLSearchParams(window.location.search);
    return params.get('slug');
  }

  /**
   * Extract filecode from the slug.
   * Slug format: movie-name-in-kebab-case-FILECODE
   */
  function extractFilecode(slug) {
    if (!slug) return null;
    var lastHyphen = slug.lastIndexOf('-');
    if (lastHyphen === -1) return null;
    return slug.substring(lastHyphen + 1);
  }

  /**
   * Fetch the entire database.
   */
  async function fetchDatabase() {
    try {
      var response = await fetch('database.json');
      if (!response.ok) {
        throw new Error('Failed to load database');
      }
      return await response.json();
    } catch (err) {
      console.error('Error loading database:', err);
      return [];
    }
  }

  /**
   * Find a movie by filecode in the database.
   */
  async function findMovieByFilecode(filecode) {
    var database = await fetchDatabase();
    return database.find(function(movie) {
      return movie.id === filecode;
    }) || null;
  }

  // ================================================================
  // HOMEPAGE MODE - render movie grid
  // ================================================================

  /**
   * Render the movie grid with all movies from the database.
   */
  async function renderHomepage() {
    var database = await fetchDatabase();

    homepageEl.classList.remove('hidden');
    appContainer.classList.add('hidden');

    if (!database || database.length === 0) {
      movieGridEl.innerHTML =
        '<div class="empty-state">' +
          '<div class="empty-icon">&#127916;</div>' +
          '<p>No movies in the database yet.</p>' +
        '</div>';
      return;
    }

    var html = '';
    for (var i = 0; i < database.length; i++) {
      var movie = database[i];
      var url = window.location.pathname.replace(/\/?$/, '/') + '?slug=' + encodeURIComponent(movie.slug);
      html +=
        '<a href="' + url + '" class="movie-card">' +
          '<div class="card-title">' + escapeHtml(movie.name) + '</div>' +
          '<div class="card-id">' + escapeHtml(movie.id) + '</div>' +
          '<div class="card-link">' + escapeHtml(movie.vidozaUrl) + '</div>' +
          '<div class="card-arrow">&#8594;</div>' +
        '</a>';
    }
    movieGridEl.innerHTML = html;
  }

  /**
   * Simple HTML escaping.
   */
  function escapeHtml(str) {
    var div = document.createElement('div');
    div.appendChild(document.createTextNode(str));
    return div.innerHTML;
  }

  // ================================================================
  // REDIRECT MODE - show movie and handle redirect
  // ================================================================

  /**
   * Display movie information and handle the proceed button.
   */
  function displayMovie(movie) {
    homepageEl.classList.add('hidden');
    appContainer.classList.remove('hidden');
    movieSection.classList.remove('hidden');
    errorContainer.classList.add('hidden');

    document.getElementById('movie-title').textContent = movie.name;
    document.getElementById('movie-id').textContent = movie.id;

    var proceedBtn = document.getElementById('proceed-btn');
    var newBtn = proceedBtn.cloneNode(true);
    proceedBtn.parentNode.replaceChild(newBtn, proceedBtn);

    newBtn.addEventListener('click', function(e) {
      e.preventDefault();
      // Open zone URL in new tab
      window.open(movie.zoneUrl || ZONE_URL, '_blank');
      // Navigate current tab to vidoza movie URL
      window.location.href = movie.vidozaUrl;
    });

    document.getElementById('movie-link').textContent = movie.vidozaUrl;
  }

  /**
   * Show an error message.
   */
  function showError(message) {
    homepageEl.classList.add('hidden');
    appContainer.classList.remove('hidden');
    movieSection.classList.add('hidden');
    errorContainer.classList.remove('hidden');
    errorContainer.querySelector('p').textContent = message;
  }

  /**
   * Handle the redirect (slug) flow.
   */
  async function handleRedirect(slug) {
    var filecode = extractFilecode(slug);

    if (!filecode) {
      showError('Invalid link format. Please use a valid Dorflix link.');
      return;
    }

    var movie = await findMovieByFilecode(filecode);

    if (!movie) {
      showError('Movie not found in database.');
      return;
    }

    displayMovie(movie);
  }

  // ================================================================
  // INIT
  // ================================================================

  /**
   * Initialize the app - decide which mode based on URL.
   */
  async function init() {
    var slug = getSlugFromURL();

    if (slug) {
      await handleRedirect(slug);
    } else {
      await renderHomepage();
    }
  }

  // Run when DOM is ready
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

})();