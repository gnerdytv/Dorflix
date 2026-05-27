// Dorflix - Movie Link Redirector
// Parses slug from URL, looks up movie in database, handles redirect

(function() {
  'use strict';

  const ZONE_URL = 'https://motionless-bus.com/CDgc7m';

  const appContainer = document.getElementById('app');
  const errorContainer = document.getElementById('error');
  const movieSection = document.getElementById('movie-section');

  /**
   * Read the slug from the URL query parameter.
   * Expects: ?slug=the-matrix-abc123
   */
  function getSlugFromURL() {
    const params = new URLSearchParams(window.location.search);
    return params.get('slug');
  }

  /**
   * Extract filecode from the slug.
   * The slug format is: movie-name-in-kebab-case-FILECODE
   * We take everything after the last hyphen as the filecode.
   */
  function extractFilecode(slug) {
    if (!slug) return null;
    const lastHyphen = slug.lastIndexOf('-');
    if (lastHyphen === -1) return null;
    return slug.substring(lastHyphen + 1);
  }

  /**
   * Fetch the database.json and find the movie by filecode.
   */
  async function findMovieByFilecode(filecode) {
    try {
      const response = await fetch('database.json');
      if (!response.ok) {
        throw new Error('Failed to load database');
      }
      const database = await response.json();
      return database.find(function(movie) {
        return movie.id === filecode;
      }) || null;
    } catch (err) {
      console.error('Error loading database:', err);
      return null;
    }
  }

  /**
   * Display movie information on the page.
   */
  function displayMovie(movie) {
    movieSection.classList.remove('hidden');
    errorContainer.classList.add('hidden');

    document.getElementById('movie-title').textContent = movie.name;
    document.getElementById('movie-id').textContent = movie.id;

    const proceedBtn = document.getElementById('proceed-btn');
    const newBtn = proceedBtn.cloneNode(true);
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
    movieSection.classList.add('hidden');
    errorContainer.classList.remove('hidden');
    errorContainer.querySelector('p').textContent = message;
  }

  /**
   * Initialize the app.
   */
  async function init() {
    const slug = getSlugFromURL();

    if (!slug) {
      showError('No movie specified. Please use a valid Dorflix link.');
      return;
    }

    const filecode = extractFilecode(slug);

    if (!filecode) {
      showError('Invalid link format. Please use a valid Dorflix link.');
      return;
    }

    const movie = await findMovieByFilecode(filecode);

    if (!movie) {
      showError('Movie not found in database.');
      return;
    }

    displayMovie(movie);
  }

  // Run when DOM is ready
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

})();