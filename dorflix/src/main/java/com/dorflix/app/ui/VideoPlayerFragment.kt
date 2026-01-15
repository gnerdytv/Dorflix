package com.dorflix.app.ui

import android.content.Context
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import android.os.Bundle
import android.os.PowerManager
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.SeekBar
import androidx.core.view.isVisible
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.lifecycle.lifecycleScope
import com.dorflix.app.R
import com.dorflix.app.databinding.FragmentVideoPlayerBinding
import com.dorflix.app.presentation.viewmodel.VideoViewModel
import com.dorflix.app.util.CrashLogger
import com.dorflix.app.video.VideoPlayerController
import com.dorflix.app.video.VideoPlayerListener
import com.dorflix.app.video.HttpsStreamingProxy
import kotlinx.coroutines.TimeoutCancellationException
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import kotlinx.coroutines.withTimeout
import kotlinx.coroutines.Job

/**
 * Fragment for video playback with C++ integration
 */
class VideoPlayerFragment : Fragment(), VideoPlayerListener {
    
    private var _binding: FragmentVideoPlayerBinding? = null
    private val binding get() = _binding!!
    
    private val viewModel: VideoViewModel by viewModels()
    
    private lateinit var videoPlayerController: VideoPlayerController
    private var videoId: String = ""
    private var currentVideoUrl: String? = null
    
    private var isPlaying = false
    private var currentPosition = 0
    private var videoDuration = 0

    // State management
    private var lastSavedPosition = 0
    private var retryCount = 0
    private var isFullscreen = false
    private var isViewDestroyed = false
    private var isVideoPrepared = false
    private var isSurfaceReady = false
    private var videoPreparationTimeoutJob: Job? = null
    private var callbackRetryCount = 0
    private var videoLoadStartTime = 0L
    private var lastCallbackReceivedTime = 0L

    // Prevent duplicate video processing
    private var lastProcessedVideoId: String? = null

    // Prevent multiple observer setups
    private var observerSetUp = false
    
    // Network monitoring
    private lateinit var connectivityManager: ConnectivityManager
    private lateinit var networkCallback: ConnectivityManager.NetworkCallback
    
    // Power management
    private var wakeLock: PowerManager.WakeLock? = null
    
    // Constants
    companion object {
        private const val ARG_VIDEO_ID = "video_id"
        private const val ARG_VIDEO_URL = "video_url"
        private const val MAX_RETRY_COUNT = 3
        private const val PROGRESS_SAVE_INTERVAL = 5000 // 5 seconds
        private const val PLAYBACK_TIMEOUT = 30000L // 30 seconds
        private const val WAKE_LOCK_TIMEOUT = 10 * 60 * 1000L // 10 minutes
        private const val VIDEO_PREPARATION_TIMEOUT = 15000L // 15 seconds timeout for video preparation
        private const val CALLBACK_RETRY_DELAY = 1000L // 1 second delay between callback retries
        private const val MAX_CALLBACK_RETRIES = 3 // Maximum callback retry attempts
        
        fun newInstance(videoId: String, videoUrl: String? = null): VideoPlayerFragment {
            android.util.Log.i("VideoPlayerFragment", "=== VideoPlayerFragment.newInstance() CALLED ===")
            android.util.Log.i("VideoPlayerFragment", "Input videoId: '$videoId' (length: ${videoId.length})")
            android.util.Log.i("VideoPlayerFragment", "Input videoUrl: '${videoUrl?.take(50) ?: "null"}...' (isNull: ${videoUrl == null})")

            val fragment = VideoPlayerFragment()
            val args = Bundle().apply {
                putString(ARG_VIDEO_ID, videoId)
                videoUrl?.let { putString(ARG_VIDEO_URL, it) }
            }

            // CRITICAL: Set the arguments on the fragment!
            fragment.arguments = args

            android.util.Log.i("VideoPlayerFragment", "Arguments bundle created and set on fragment")
            android.util.Log.i("VideoPlayerFragment", "=== VideoPlayerFragment.newInstance() COMPLETE ===")

            return fragment
        }
    }
    
    override fun onCreate(savedInstanceState: Bundle?) {
        android.util.Log.i("VideoPlayerFragment", "=== VideoPlayerFragment.onCreate() START ===")

        super.onCreate(savedInstanceState)

        try {
            arguments?.let {
                videoId = it.getString(ARG_VIDEO_ID, "")
                currentVideoUrl = it.getString(ARG_VIDEO_URL)
                android.util.Log.i("VideoPlayerFragment", "Arguments parsed: videoId=$videoId, currentVideoUrl=${currentVideoUrl?.take(50)}...")
            }

            // If arguments are null (fragment recreated from backstack), try to restore from savedInstanceState
            if (arguments == null) {
                savedInstanceState?.let { state ->
                    videoId = state.getString(ARG_VIDEO_ID, "")
                    currentVideoUrl = state.getString(ARG_VIDEO_URL)
                    android.util.Log.i("VideoPlayerFragment", "Arguments restored from saved state: videoId=$videoId, currentVideoUrl=${currentVideoUrl?.take(50)}...")
                }
            }

            // Restore playback state if available
            savedInstanceState?.let {
                isPlaying = it.getBoolean("is_playing", false)
                currentPosition = it.getInt("current_position", 0)
                videoDuration = it.getInt("video_duration", 0)
                // Note: video_url is handled above in arguments restoration
                android.util.Log.i("VideoPlayerFragment", "Playback state restored: playing=$isPlaying, position=$currentPosition, duration=$videoDuration")
            }

            android.util.Log.i("VideoPlayerFragment", "=== INITIALIZING C++ VIDEO PLAYER CONTROLLER ===")
            android.util.Log.i("VideoPlayerFragment", "Context available: ${context != null}")
            android.util.Log.i("VideoPlayerFragment", "Activity available: ${activity != null}")

            // Initialize C++ video player with crash protection
            try {
                android.util.Log.i("VideoPlayerFragment", "Creating VideoPlayerController instance...")
                videoPlayerController = VideoPlayerController(this)
                android.util.Log.i("VideoPlayerFragment", "✅ VideoPlayerController created successfully")
                android.util.Log.i("VideoPlayerFragment", "Controller initialized: ${::videoPlayerController.isInitialized}")
            } catch (e: UnsatisfiedLinkError) {
                android.util.Log.e("VideoPlayerFragment", "=== CRASH: UNSATISFIED LINK ERROR ===")
                android.util.Log.e("VideoPlayerFragment", "Failed to load native library: ${e.message}")
                android.util.Log.e("VideoPlayerFragment", "Stack trace:", e)
                throw e
            } catch (e: RuntimeException) {
                android.util.Log.e("VideoPlayerFragment", "=== CRASH: RUNTIME EXCEPTION IN CONTROLLER INIT ===")
                android.util.Log.e("VideoPlayerFragment", "RuntimeException: ${e.javaClass.simpleName}: ${e.message}")
                android.util.Log.e("VideoPlayerFragment", "Stack trace:", e)
                throw e
            } catch (e: Exception) {
                android.util.Log.e("VideoPlayerFragment", "=== CRASH: UNEXPECTED EXCEPTION IN CONTROLLER INIT ===")
                android.util.Log.e("VideoPlayerFragment", "Exception: ${e.javaClass.simpleName}: ${e.message}")
                android.util.Log.e("VideoPlayerFragment", "Stack trace:", e)
                throw e
            }

            // Initialize connectivity manager
            try {
                android.util.Log.i("VideoPlayerFragment", "Initializing connectivity manager...")
                connectivityManager = requireContext()
                    .getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
                android.util.Log.i("VideoPlayerFragment", "✅ Connectivity manager initialized")
            } catch (e: Exception) {
                android.util.Log.e("VideoPlayerFragment", "=== CRASH: CONNECTIVITY MANAGER INIT FAILED ===")
                android.util.Log.e("VideoPlayerFragment", "Exception: ${e.javaClass.simpleName}: ${e.message}")
                android.util.Log.e("VideoPlayerFragment", "Stack trace:", e)
                // Don't throw here - connectivity is not critical for video playback
            }

            android.util.Log.i("VideoPlayerFragment", "=== VideoPlayerFragment.onCreate() COMPLETED SUCCESSFULLY ===")

        } catch (e: Exception) {
            android.util.Log.e("VideoPlayerFragment", "=== CRASH: VideoPlayerFragment.onCreate() FAILED ===")
            android.util.Log.e("VideoPlayerFragment", "Fatal exception: ${e.javaClass.simpleName}: ${e.message}")
            android.util.Log.e("VideoPlayerFragment", "Stack trace:", e)
            throw e
        }
    }
    
    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentVideoPlayerBinding.inflate(inflater, container, false)
        return binding.root
    }
    
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        
        setupUI()
        setupVideoPlayer()
        loadVideo()
        setupNetworkMonitoring()
    }
    
    private fun setupUI() {
        // Setup play/pause button
        binding.buttonPlayPause.setOnClickListener {
            togglePlayPause()
        }
        
        // Setup fullscreen button
        binding.buttonFullscreen.setOnClickListener {
            toggleFullscreen()
        }
        
        // Setup seek bar
        binding.seekBarProgress.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                if (fromUser) {
                    // Update UI immediately for better responsiveness
                    currentPosition = progress
                    binding.textViewPosition.text = formatTimeSafe(progress)
                    videoPlayerController.seekTo(progress)
                }
            }
            
            override fun onStartTrackingTouch(seekBar: SeekBar?) {
                // Pause video while seeking for better UX
                if (isPlaying) {
                    videoPlayerController.pause()
                }
            }
            
            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                // Resume playback after seeking
                if (isPlaying) {
                    videoPlayerController.play()
                }
                // Save progress after seeking
                saveWatchProgress()
            }
        })
        
        // Setup back button
        binding.buttonBack.setOnClickListener {
            requireActivity().onBackPressedDispatcher.onBackPressed()
        }
        
        // Setup retry button
        binding.buttonRetry.setOnClickListener {
            retryVideoLoading()
        }
        
        // Hide error message initially
        binding.layoutError.isVisible = false
    }
    
    private fun setupVideoPlayer() {
        // Set up surface view for video rendering
        android.util.Log.i("VideoPlayerFragment", "Setting up SurfaceView holder callback")
        binding.surfaceView.holder.addCallback(object : android.view.SurfaceHolder.Callback {
            override fun surfaceCreated(holder: android.view.SurfaceHolder) {
                android.util.Log.i("VideoPlayerFragment", "=== SURFACE CREATED ===")
                android.util.Log.i("VideoPlayerFragment", "Surface size: ${holder.surfaceFrame.width()}x${holder.surfaceFrame.height()}")
                android.util.Log.i("VideoPlayerFragment", "isAdded: $isAdded, videoPlayerController initialized: ${::videoPlayerController.isInitialized}")

                if (isAdded && ::videoPlayerController.isInitialized) {
                    android.util.Log.i("VideoPlayerFragment", "Setting surface on video player controller")
                    videoPlayerController.setSurface(holder.surface)
                    isSurfaceReady = true
                    android.util.Log.i("VideoPlayerFragment", "Surface ready, calling tryStartPlayback()")
                    // Start playback if video is prepared and surface is ready
                    tryStartPlayback()
                } else {
                    android.util.Log.w("VideoPlayerFragment", "Cannot set surface - fragment not added or controller not initialized")
                }
            }

            override fun surfaceChanged(
                holder: android.view.SurfaceHolder,
                format: Int,
                width: Int,
                height: Int
            ) {
                android.util.Log.i("VideoPlayerFragment", "=== SURFACE CHANGED ===")
                android.util.Log.i("VideoPlayerFragment", "New size: ${width}x${height}, format: $format")
                android.util.Log.i("VideoPlayerFragment", "isAdded: $isAdded, videoPlayerController initialized: ${::videoPlayerController.isInitialized}")

                if (isAdded && ::videoPlayerController.isInitialized) {
                    android.util.Log.i("VideoPlayerFragment", "Re-setting surface on video player controller")
                    videoPlayerController.setSurface(holder.surface)
                    isSurfaceReady = true
                    android.util.Log.i("VideoPlayerFragment", "Surface ready after change, calling tryStartPlayback()")
                    // Start playback if video is prepared and surface is ready
                    tryStartPlayback()
                } else {
                    android.util.Log.w("VideoPlayerFragment", "Cannot set surface after change - fragment not added or controller not initialized")
                }
            }

            override fun surfaceDestroyed(holder: android.view.SurfaceHolder) {
                android.util.Log.i("VideoPlayerFragment", "=== SURFACE DESTROYED ===")
                android.util.Log.i("VideoPlayerFragment", "isAdded: $isAdded, videoPlayerController initialized: ${::videoPlayerController.isInitialized}")

                if (isAdded && ::videoPlayerController.isInitialized) {
                    android.util.Log.i("VideoPlayerFragment", "Clearing surface on video player controller")
                    videoPlayerController.setSurface(null)
                }
                isSurfaceReady = false
                android.util.Log.i("VideoPlayerFragment", "Surface no longer ready")
            }
        })
    }

    /**
     * Attempts to start playback when both video and surface are ready
     * TikTok-style: Auto-play immediately when ready
     */
    private fun tryStartPlayback() {
        android.util.Log.i("VideoPlayerFragment", "=== TRY START TIKTOK AUTO-PLAY ===")
        android.util.Log.i("VideoPlayerFragment", "isVideoPrepared: $isVideoPrepared, isSurfaceReady: $isSurfaceReady, isPlaying: $isPlaying, isAdded: $isAdded")
        android.util.Log.i("VideoPlayerFragment", "videoPlayerController initialized: ${::videoPlayerController.isInitialized}")

        if (isVideoPrepared && isSurfaceReady && !isPlaying && isAdded) {
            android.util.Log.i("VideoPlayerFragment", "=== TIKTOK AUTO-PLAY CONDITIONS MET - STARTING LOOPED PLAYBACK ===")
            android.util.Log.i("VideoPlayerFragment", "Calling videoPlayerController.play() for TikTok-style looping")
            videoPlayerController.play()
            isPlaying = true
            android.util.Log.i("VideoPlayerFragment", "Set isPlaying = true, video will loop continuously")
            updatePlayPauseButton()
            android.util.Log.i("VideoPlayerFragment", "Acquiring wake lock for continuous playback")
            acquireWakeLock()
            android.util.Log.i("VideoPlayerFragment", "Logging TikTok auto-started event")
            logVideoEvent("tiktok_auto_started", mapOf(
                "style" to "continuous_loop",
                "reason" to "video_and_surface_ready"
            ))
            android.util.Log.i("VideoPlayerFragment", "=== TIKTOK AUTO-PLAY STARTED SUCCESSFULLY ===")
        } else {
            android.util.Log.i("VideoPlayerFragment", "TikTok auto-play conditions not met")
            val reasons = mutableListOf<String>()
            if (!isVideoPrepared) reasons.add("video_not_prepared")
            if (!isSurfaceReady) reasons.add("surface_not_ready")
            if (isPlaying) reasons.add("already_playing")
            if (!isAdded) reasons.add("fragment_not_added")
            android.util.Log.e("VideoPlayerFragment", "🚫 BLOCKING TIKTOK AUTO-PLAY REASONS: ${reasons.joinToString(", ")}")
            android.util.Log.e("VideoPlayerFragment", "📊 Current state: videoPrepared=$isVideoPrepared, surfaceReady=$isSurfaceReady, playing=$isPlaying, fragmentAdded=$isAdded")
        }
    }
    
    private fun setupNetworkMonitoring() {
        networkCallback = object : ConnectivityManager.NetworkCallback() {
            override fun onAvailable(network: Network) {
                // Resume playback if network was lost
                if (isAdded && isPlaying) {
                    activity?.runOnUiThread {
                        showToast("Network restored")
                        if (!isPlaying) {
                            videoPlayerController.play()
                        }
                    }
                }
            }
            
            override fun onLost(network: Network) {
                if (isAdded && isPlaying) {
                    activity?.runOnUiThread {
                        showErrorMessage("Network connection lost")
                        videoPlayerController.pause()
                    }
                }
            }
            
            override fun onCapabilitiesChanged(
                network: Network,
                networkCapabilities: NetworkCapabilities
            ) {
                // Monitor network quality changes
                if (networkCapabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_VALIDATED)) {
                CrashLogger.videoEvent("Network quality changed",
                    "video_id=$videoId")
                }
            }
        }
        
        val networkRequest = NetworkRequest.Builder()
            .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
            .build()
        
        connectivityManager.registerNetworkCallback(networkRequest, networkCallback)
    }
    
    private fun loadVideo() {
        android.util.Log.i("VideoPlayerFragment", "=== LOAD VIDEO START ===")
        android.util.Log.i("VideoPlayerFragment", "Video ID: $videoId, Current URL: $currentVideoUrl")
        android.util.Log.i("VideoPlayerFragment", "Thread: ${Thread.currentThread().name} (${Thread.currentThread().id})")

        if (videoId.isEmpty()) {
            android.util.Log.e("VideoPlayerFragment", "Video ID is empty - aborting load")
            showErrorMessage("Video ID is empty")
            return
        }

        // Show loading state
        android.util.Log.i("VideoPlayerFragment", "Setting loading state - progress bar visible")
        binding.progressBar.isVisible = true
        binding.layoutError.isVisible = false

        if (currentVideoUrl != null) {
            android.util.Log.i("VideoPlayerFragment", "Using cached video URL: $currentVideoUrl")
            // Use cached URL if available
            startPlayback(currentVideoUrl!!)
        } else {
            android.util.Log.i("VideoPlayerFragment", "No cached URL - loading video details from ViewModel")
            // Load video details from view model
            viewModel.loadVideoDetails(videoId)

            // Prevent multiple observer setups
            if (observerSetUp) {
                android.util.Log.i("VideoPlayerFragment", "Observer already set up, skipping duplicate setup")
                return
            }

            observerSetUp = true
            android.util.Log.i("VideoPlayerFragment", "Setting up video details observer for first time")

            // Observe video details and start playback when available
            viewLifecycleOwner.lifecycleScope.launch {
                android.util.Log.i("VideoPlayerFragment", "=== STARTING VIDEO DETAILS OBSERVER ===")
                android.util.Log.i("VideoPlayerFragment", "Observer launched on thread: ${Thread.currentThread().name} (${Thread.currentThread().id})")
                android.util.Log.i("VideoPlayerFragment", "ViewModel instance: ${viewModel.hashCode()}")
                android.util.Log.i("VideoPlayerFragment", "Current currentVideoUrl before observer: '$currentVideoUrl'")

                viewModel.currentVideo.collectLatest { video ->
                    android.util.Log.i("VideoPlayerFragment", "=== OBSERVER RECEIVED VIDEO DATA ===")
                    android.util.Log.i("VideoPlayerFragment", "Video object: $video")
                    android.util.Log.i("VideoPlayerFragment", "Video is null: ${video == null}")

                    if (video != null) {
                        // Validate that the video has essential fields
                        if (video.id.isNullOrBlank()) {
                            android.util.Log.e("VideoPlayerFragment", "=== MALFORMED VIDEO OBJECT: Missing ID ===")
                            android.util.Log.e("VideoPlayerFragment", "Video object: $video")
                            showErrorMessage("Invalid video data: Missing video ID")
                            return@collectLatest
                        }

                        // Prevent processing the same video multiple times
                        if (lastProcessedVideoId == video.id) {
                            android.util.Log.i("VideoPlayerFragment", "=== SKIPPING DUPLICATE VIDEO PROCESSING ===")
                            android.util.Log.i("VideoPlayerFragment", "Video ID ${video.id} already processed, ignoring duplicate emission")
                            return@collectLatest
                        }

                        android.util.Log.i("VideoPlayerFragment", "=== VIDEO OBJECT RECEIVED ===")
                        android.util.Log.i("VideoPlayerFragment", "Video ID: ${video.id}")
                        android.util.Log.i("VideoPlayerFragment", "Video title: ${video.title}")
                        android.util.Log.i("VideoPlayerFragment", "Video URL: '${video.video_url}'")
                        android.util.Log.i("VideoPlayerFragment", "Video URL length: ${video.video_url.length}")
                        android.util.Log.i("VideoPlayerFragment", "Video URL isNullOrBlank: ${video.video_url.isNullOrBlank()}")
                        android.util.Log.i("VideoPlayerFragment", "Video URL isNotEmpty: ${video.video_url.isNotEmpty()}")
                        android.util.Log.i("VideoPlayerFragment", "Current currentVideoUrl before assignment: '$currentVideoUrl'")

                        // Mark this video as processed
                        lastProcessedVideoId = video.id

                        // Store video data for monitoring AFTER observer completes
                        val videoForMonitoring = video

                        if (video.video_url.isNotEmpty()) {
                            android.util.Log.i("VideoPlayerFragment", "=== VIDEO URL VALID - STARTING PLAYBACK ===")
                            android.util.Log.i("VideoPlayerFragment", "Setting currentVideoUrl = '${video.video_url}'")
                            currentVideoUrl = video.video_url
                            android.util.Log.i("VideoPlayerFragment", "Calling startPlayback('${video.video_url.take(50)}...')")
                            startPlayback(video.video_url)
                            android.util.Log.i("VideoPlayerFragment", "=== PLAYBACK STARTED SUCCESSFULLY ===")
                        } else {
                            android.util.Log.e("VideoPlayerFragment", "=== VIDEO URL INVALID ===")
                            android.util.Log.e("VideoPlayerFragment", "Video URL is empty or null: '${video.video_url}'")
                            android.util.Log.e("VideoPlayerFragment", "isNotEmpty() check failed")
                            showErrorMessage("Video URL not available")
                        }



                    } else {
                        android.util.Log.w("VideoPlayerFragment", "=== VIDEO DATA IS NULL ===")
                        android.util.Log.w("VideoPlayerFragment", "ViewModel returned null video object")
                        android.util.Log.w("VideoPlayerFragment", "This indicates API call failed or returned null")
                    }
                }
            }
        }
        android.util.Log.i("VideoPlayerFragment", "=== LOAD VIDEO END ===")
    }
    
    private fun startPlayback(videoUrl: String) {
        android.util.Log.i("VideoPlayerFragment", "=== START PLAYBACK BEGIN ===")
        android.util.Log.i("VideoPlayerFragment", "Video URL: ${videoUrl.take(50)}..., Thread: ${Thread.currentThread().name}")

        if (videoUrl.isBlank()) {
            android.util.Log.e("VideoPlayerFragment", "Invalid video URL - blank/empty")
            showErrorMessage("Invalid video URL")
            return
        }

        // Use FFmpeg's built-in HTTPS support directly (no proxy needed)
        val finalVideoUrl = videoUrl
        android.util.Log.i("VideoPlayerFragment", "Using FFmpeg direct HTTPS support: ${finalVideoUrl.take(50)}...")

        android.util.Log.i("VideoPlayerFragment", "Launching coroutine for video loading")
        viewLifecycleOwner.lifecycleScope.launch {
            android.util.Log.i("VideoPlayerFragment", "Coroutine started, setting progress bar visible")
            try {
                binding.progressBar.isVisible = true
                android.util.Log.i("VideoPlayerFragment", "About to call videoPlayerController.loadVideo() with timeout: ${PLAYBACK_TIMEOUT}ms")

                val startTime = System.currentTimeMillis()
                val success = withTimeout(PLAYBACK_TIMEOUT) {
                    android.util.Log.i("VideoPlayerFragment", "Calling videoPlayerController.loadVideo() on thread: ${Thread.currentThread().name}")
                    val result = videoPlayerController.loadVideo(finalVideoUrl)
                    android.util.Log.i("VideoPlayerFragment", "videoPlayerController.loadVideo() returned: $result")
                    result
                }
                val loadTime = System.currentTimeMillis() - startTime
                android.util.Log.i("VideoPlayerFragment", "Video loading completed in ${loadTime}ms, success: $success")

                if (success) {
                    android.util.Log.i("VideoPlayerFragment", "Video loaded successfully, hiding progress bar")
                    binding.progressBar.isVisible = false
                    retryCount = 0
                    android.util.Log.i("VideoPlayerFragment", "Logging playback started event")
                    logVideoEvent("playback_started", mapOf(
                        "video_url_length" to videoUrl.length.toString(),
                        "load_time_ms" to loadTime.toString()
                    ))
                    android.util.Log.i("VideoPlayerFragment", "=== START PLAYBACK SUCCESS ===")
                } else if (retryCount < MAX_RETRY_COUNT) {
                    retryCount++
                    android.util.Log.w("VideoPlayerFragment", "Video loading failed, retrying (attempt $retryCount/$MAX_RETRY_COUNT)")
                    showToast("Retrying... ($retryCount/$MAX_RETRY_COUNT)")
                    delay(1000L * retryCount) // Exponential backoff
                    android.util.Log.i("VideoPlayerFragment", "Starting retry attempt $retryCount")
                    startPlayback(videoUrl)
                } else {
                    android.util.Log.e("VideoPlayerFragment", "Video loading failed after $MAX_RETRY_COUNT attempts")
                    showErrorMessage("Failed to load video after $MAX_RETRY_COUNT attempts")
                }
            } catch (e: TimeoutCancellationException) {
                android.util.Log.e("VideoPlayerFragment", "=== VIDEO LOADING TIMEOUT ===")
                android.util.Log.e("VideoPlayerFragment", "Timeout after ${PLAYBACK_TIMEOUT}ms")
                showErrorMessage("Video loading timed out")
                CrashLogger.e("Video loading timeout", e)
            } catch (e: UnsatisfiedLinkError) {
                android.util.Log.e("VideoPlayerFragment", "=== NATIVE LIBRARY ERROR ===")
                android.util.Log.e("VideoPlayerFragment", "UnsatisfiedLinkError: ${e.message}")
                showErrorMessage("Video player not available")
                CrashLogger.e("Native library error", e)
            } catch (e: Exception) {
                android.util.Log.e("VideoPlayerFragment", "=== VIDEO LOADING EXCEPTION ===")
                android.util.Log.e("VideoPlayerFragment", "Exception: ${e.javaClass.simpleName}: ${e.message}")
                showErrorMessage("Video loading error")
                CrashLogger.e("Video loading exception", e)
            }
        }
        android.util.Log.i("VideoPlayerFragment", "startPlayback() method completed")
    }
    
    private fun retryVideoLoading() {
        currentVideoUrl?.let {
            binding.layoutError.isVisible = false
            binding.progressBar.isVisible = true
            startPlayback(it)
        } ?: loadVideo()
    }
    
    private fun togglePlayPause() {
        if (!isAdded || !::videoPlayerController.isInitialized) return
        
        if (isPlaying) {
            videoPlayerController.pause()
            releaseWakeLock()
        } else {
            videoPlayerController.play()
            acquireWakeLock()
        }
        isPlaying = !isPlaying
        updatePlayPauseButton()
        
        logVideoEvent("play_pause", mapOf(
            "new_state" to if (isPlaying) "playing" else "paused"
        ))
    }
    
    private fun updatePlayPauseButton() {
        try {
            if (_binding != null) {
                val iconRes = if (isPlaying) {
                    R.drawable.ic_pause
                } else {
                    R.drawable.ic_play
                }
                binding.buttonPlayPause.setImageResource(iconRes)
            }
        } catch (e: Exception) {
            CrashLogger.e("Error updating play/pause button", e)
        }
    }
    
    private fun toggleFullscreen() {
        if (!isAdded) return

        isFullscreen = !isFullscreen

        activity?.window?.let { window ->
            val windowInsetsController = androidx.core.view.WindowCompat.getInsetsController(window, window.decorView)
            if (isFullscreen) {
                // Enter fullscreen
                windowInsetsController.apply {
                    hide(androidx.core.view.WindowInsetsCompat.Type.systemBars())
                    systemBarsBehavior = androidx.core.view.WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
                }
                binding.buttonFullscreen.setImageResource(R.drawable.ic_fullscreen_exit)
                binding.buttonBack.isVisible = false
            } else {
                // Exit fullscreen
                windowInsetsController.show(androidx.core.view.WindowInsetsCompat.Type.systemBars())
                binding.buttonFullscreen.setImageResource(R.drawable.ic_fullscreen)
                binding.buttonBack.isVisible = true
            }
        }

        logVideoEvent("fullscreen", mapOf(
            "new_state" to if (isFullscreen) "entered" else "exited"
        ))
    }
    
    private fun updatePositionText(position: Int, duration: Int? = null) {
        if (!isAdded || _binding == null) return
        
        try {
            // Use provided duration or fallback to instance variable
            val safeDuration = duration ?: videoDuration
            
            // Ensure we have valid values
            val safePosition = position.coerceAtLeast(0)
            val safeDurationValue = safeDuration.coerceAtLeast(0)
            
            binding.textViewPosition.text = formatTimeSafe(safePosition)
            binding.textViewDuration.text = formatTimeSafe(safeDurationValue)
        } catch (e: Exception) {
            CrashLogger.e("Error updating position text", e)
            // Set fallback values
            try {
                binding.textViewPosition.text = "00:00"
                binding.textViewDuration.text = "00:00"
            } catch (e2: Exception) {
                CrashLogger.e("Error in position text fallback", e2)
            }
        }
    }
    
    private fun formatTimeSafe(milliseconds: Int): String {
        return try {
            // Handle null/negative values
            if (milliseconds < 0) return "00:00"
            
            val totalSeconds = milliseconds / 1000
            val hours = totalSeconds / 3600
            val minutes = (totalSeconds % 3600) / 60
            val seconds = totalSeconds % 60
            
            return if (hours > 0) {
                String.format("%02d:%02d:%02d", hours, minutes, seconds)
            } else {
                String.format("%02d:%02d", minutes, seconds)
            }
        } catch (e: Exception) {
            CrashLogger.e("Error formatting time: $milliseconds", e)
            "00:00"
        }
    }
    
    private fun showErrorMessage(message: String) {
        if (!isAdded || _binding == null) return
        
        activity?.runOnUiThread {
            try {
                binding.layoutError.visibility = View.VISIBLE
                binding.textViewErrorMessage.text = message
                binding.progressBar.visibility = View.GONE
                
                // Show toast for better UX
                showToast(message)
            } catch (e: Exception) {
                CrashLogger.e("Error showing error message", e)
            }
        }
    }
    
    private fun showToast(message: String) {
        if (isAdded) {
            android.widget.Toast.makeText(requireContext(), message, android.widget.Toast.LENGTH_SHORT).show()
        }
    }
    
    private fun saveWatchProgress() {
        if (videoId.isNotEmpty() && currentPosition > 0 && videoDuration > 0) {
            viewModel.saveWatchProgress(videoId, currentPosition, videoDuration)
            lastSavedPosition = currentPosition
        }
    }
    
    private fun acquireWakeLock() {
        try {
            if (isAdded) {
                val powerManager = requireContext().getSystemService(Context.POWER_SERVICE) as PowerManager
                wakeLock = powerManager.newWakeLock(
                    PowerManager.PARTIAL_WAKE_LOCK,
                    "DorflixApp:VideoPlayer"
                )
                wakeLock?.acquire(WAKE_LOCK_TIMEOUT)
            }
        } catch (e: Exception) {
            CrashLogger.e("Error acquiring wake lock", e)
        }
    }
    
    private fun releaseWakeLock() {
        try {
            wakeLock?.let {
                if (it.isHeld) {
                    it.release()
                }
                wakeLock = null
            }
        } catch (e: Exception) {
            CrashLogger.e("Error releasing wake lock", e)
        }
    }
    
    private fun logVideoEvent(eventName: String, properties: Map<String, Any> = emptyMap()) {
        try {
            val eventProperties = properties.toMutableMap().apply {
                put("video_id", videoId)
                put("position", currentPosition)
                put("duration", videoDuration)
                put("is_playing", isPlaying)
                put("is_fullscreen", isFullscreen)
            }
            
            // Convert Map to String for CrashLogger.videoEvent
            val propertiesString = eventProperties.entries.joinToString(", ") { "${it.key}=${it.value}" }
            CrashLogger.videoEvent("video_$eventName", propertiesString)
        } catch (e: Exception) {
            CrashLogger.e("Error logging video event", e)
        }
    }
    
    // VideoPlayerListener implementation with thread safety
    override fun onVideoPrepared(duration: Int) {
        activity?.runOnUiThread {
            if (!isAdded || _binding == null || isViewDestroyed) return@runOnUiThread

            try {
                videoDuration = duration.coerceAtLeast(0)
                binding.seekBarProgress.max = videoDuration
                binding.progressBar.isVisible = false
                updatePositionText(0, videoDuration)

                // Mark video as prepared and try to start playback
                isVideoPrepared = true
                lastCallbackReceivedTime = System.currentTimeMillis()
                android.util.Log.i("VideoPlayerFragment", "Video prepared callback received, duration: $duration")

                // Cancel timeout job since we received the callback
                videoPreparationTimeoutJob?.cancel()
                videoPreparationTimeoutJob = null

                tryStartPlaybackSafely()

                logVideoEvent("prepared", mapOf("duration_ms" to duration))
            } catch (e: Exception) {
                CrashLogger.e("Error in onVideoPrepared", e)
            }
        }
    }

    override fun onVideoStarted() {
        activity?.runOnUiThread {
            if (!isAdded) return@runOnUiThread
            isPlaying = true
            updatePlayPauseButton()
            logVideoEvent("started")
        }
    }

    override fun onVideoPaused() {
        activity?.runOnUiThread {
            if (!isAdded) return@runOnUiThread
            isPlaying = false
            updatePlayPauseButton()
            releaseWakeLock()
            logVideoEvent("paused")
        }
    }

    override fun onVideoStopped() {
        try {
            activity?.runOnUiThread {
                try {
                    if (!isAdded) return@runOnUiThread
                    isPlaying = false
                    updatePlayPauseButton()
                    releaseWakeLock()
                    logVideoEvent("stopped")
                } catch (e: Exception) {
                    android.util.Log.e("VideoPlayerFragment", "CRASH in onVideoStopped UI thread", e)
                    throw e // Re-throw to see the crash
                }
            }
        } catch (e: Exception) {
            android.util.Log.e("VideoPlayerFragment", "CRASH in onVideoStopped", e)
            throw e // Re-throw to see the crash
        }
    }
    
    override fun onVideoCompleted() {
        activity?.runOnUiThread {
            if (!isAdded) return@runOnUiThread

            // For TikTok-style app: Loop the video back to start instead of stopping
            android.util.Log.i("VideoPlayerFragment", "=== VIDEO COMPLETED - LOOPING BACK TO START ===")
            videoPlayerController.seekTo(0)  // Seek back to beginning
            videoPlayerController.play()     // Start playing again
            isPlaying = true
            updatePlayPauseButton()
            acquireWakeLock()

            logVideoEvent("looped", mapOf("action" to "restart_from_beginning"))
        }
    }
    
    override fun onVideoError(errorCode: Int, errorMessage: String) {
        activity?.runOnUiThread {
            if (!isAdded) return@runOnUiThread
            binding.progressBar.isVisible = false
            showErrorMessage("Video error: $errorMessage (Code: $errorCode)")
            logVideoEvent("error", mapOf(
                "error_code" to errorCode,
                "error_message" to errorMessage
            ))
        }
    }
    
    override fun onVideoProgressChanged(position: Int) {
        activity?.runOnUiThread {
            if (!isAdded || _binding == null) return@runOnUiThread
            
            try {
                currentPosition = position.coerceAtLeast(0)
                
                // Update seek bar (avoiding feedback loop from user interaction)
                if (!binding.seekBarProgress.isPressed) {
                    binding.seekBarProgress.progress = currentPosition
                }
                
                updatePositionText(currentPosition)
                
                // Save watch progress periodically
                if (abs(currentPosition - lastSavedPosition) >= PROGRESS_SAVE_INTERVAL) {
                    saveWatchProgress()
                }
            } catch (e: Exception) {
                CrashLogger.e("Error in onVideoProgressChanged", e)
            }
        }
    }
    
    override fun onVideoBufferingStarted() {
        activity?.runOnUiThread {
            if (!isAdded) return@runOnUiThread
            binding.progressBar.isVisible = true
            logVideoEvent("buffering_started")
        }
    }
    
    override fun onVideoBufferingEnded() {
        activity?.runOnUiThread {
            if (!isAdded) return@runOnUiThread
            binding.progressBar.isVisible = false
            logVideoEvent("buffering_ended")
        }
    }
    
    override fun onVideoSeekComplete() {
        activity?.runOnUiThread {
            if (!isAdded) return@runOnUiThread
            logVideoEvent("seek_completed")
        }
    }
    
    override fun onVideoSizeChanged(width: Int, height: Int) {
        activity?.runOnUiThread {
            if (!isAdded) return@runOnUiThread
            logVideoEvent("size_changed", mapOf(
                "width" to width,
                "height" to height
            ))
        }
    }
    
    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.apply {
            // Save original arguments for restoration
            putString(ARG_VIDEO_ID, videoId)
            putString(ARG_VIDEO_URL, currentVideoUrl)

            // Save playback state
            putBoolean("is_playing", isPlaying)
            putInt("current_position", currentPosition)
            putInt("video_duration", videoDuration)
            putBoolean("is_fullscreen", isFullscreen)
        }

        android.util.Log.i("VideoPlayerFragment", "=== SAVING INSTANCE STATE ===")
        android.util.Log.i("VideoPlayerFragment", "Saved videoId: '$videoId'")
        android.util.Log.i("VideoPlayerFragment", "Saved currentVideoUrl: '${currentVideoUrl?.take(50)}...'")
        android.util.Log.i("VideoPlayerFragment", "Saved playback state: playing=$isPlaying, position=$currentPosition")
    }
    
    override fun onPause() {
        super.onPause()
        try {
            if (isAdded && ::videoPlayerController.isInitialized && isPlaying) {
                videoPlayerController.pause()
                isPlaying = false
                updatePlayPauseButton()
            }
            releaseWakeLock()
        } catch (e: Exception) {
            CrashLogger.e("Error in onPause", e)
        }
    }
    
    override fun onResume() {
        super.onResume()
        try {
            if (isAdded && ::videoPlayerController.isInitialized && isPlaying) {
                videoPlayerController.play()
                acquireWakeLock()
            }
        } catch (e: Exception) {
            CrashLogger.e("Error in onResume", e)
        }
    }
    
    override fun onStop() {
        super.onStop()
        // Save progress when stopping
        saveWatchProgress()
        releaseWakeLock()
    }
    
    override fun onDestroyView() {
        super.onDestroyView()
        isViewDestroyed = true
        
        // Clean up network monitoring
        try {
            connectivityManager.unregisterNetworkCallback(networkCallback)
        } catch (e: Exception) {
            CrashLogger.e("Error unregistering network callback", e)
        }
        
        // Clean up video player
        try {
            if (::videoPlayerController.isInitialized) {
                videoPlayerController.release()
            }
        } catch (e: Exception) {
            CrashLogger.e("Error releasing video player", e)
        }
        
        // Release wake lock
        releaseWakeLock()
        
        // Clear the binding reference
        _binding = null
    }
    
    override fun onConfigurationChanged(newConfig: android.content.res.Configuration) {
        super.onConfigurationChanged(newConfig)
        // Handle configuration changes gracefully
        CrashLogger.videoEvent("configuration_changed", 
            "orientation=${newConfig.orientation.toString()}")
    }
    
    /**
     * Monitor video URL processing in the fragment
     */
    private fun monitorVideoUrlInFragment(videoId: String, videoUrl: String) {
        val isUrlPresent = videoUrl.isNotBlank()
        val isMp4Format = videoUrl.lowercase().endsWith(".mp4")
        val urlLength = videoUrl.length

        // Log video URL processing in fragment
        android.util.Log.i("VideoPlayerFragment", "=== FRAGMENT VIDEO URL PROCESSING ===")
        android.util.Log.i("VideoPlayerFragment", "Video ID: $videoId")
        android.util.Log.i("VideoPlayerFragment", "Video URL received: $isUrlPresent")
        android.util.Log.i("VideoPlayerFragment", "Video URL length: $urlLength")
        android.util.Log.i("VideoPlayerFragment", "MP4 format validation: $isMp4Format")
        android.util.Log.i("VideoPlayerFragment", "Video URL content: ${if (isUrlPresent) videoUrl.take(100) + if (videoUrl.length > 100) "..." else "" else "NULL/EMPTY"}")

        // Validate video URL requirements
        if (isUrlPresent) {
            if (!isMp4Format) {
                android.util.Log.e("VideoPlayerFragment", "❌ CRITICAL: Video URL must be in MP4 format but received: $videoUrl")
                showErrorMessage("Video format error: Only MP4 videos are supported")
                return
            }

            // Additional validation
            if (urlLength < 10) {
                android.util.Log.w("VideoPlayerFragment", "⚠ WARNING: Video URL seems too short: $videoUrl")
            }

            if (!videoUrl.startsWith("http://") && !videoUrl.startsWith("https://") &&
                !videoUrl.startsWith("file://")) {
                android.util.Log.w("VideoPlayerFragment", "⚠ WARNING: Video URL doesn't start with valid protocol: $videoUrl")
            }

            android.util.Log.i("VideoPlayerFragment", "✓ Video URL validation passed, proceeding with playback")
        } else {
            android.util.Log.e("VideoPlayerFragment", "❌ ERROR: No video URL received from backend for video ID: $videoId")
            showErrorMessage("Video streaming URL not available from server")
        }

        android.util.Log.i("VideoPlayerFragment", "=== END FRAGMENT VIDEO URL PROCESSING ===")
    }

    private fun abs(value: Int): Int {
        return if (value < 0) -value else value
    }

    /**
     * Start video preparation timeout to detect when callbacks fail
     */
    private fun startVideoPreparationTimeout() {
        android.util.Log.i("VideoPlayerFragment", "=== STARTING VIDEO PREPARATION TIMEOUT ===")
        android.util.Log.i("VideoPlayerFragment", "Timeout duration: $VIDEO_PREPARATION_TIMEOUT ms")

        // Cancel any existing timeout job
        videoPreparationTimeoutJob?.cancel()
        videoPreparationTimeoutJob = null

        // Reset callback tracking
        callbackRetryCount = 0
        videoLoadStartTime = System.currentTimeMillis()
        lastCallbackReceivedTime = 0L

        // Start timeout coroutine
        videoPreparationTimeoutJob = viewLifecycleOwner.lifecycleScope.launch {
            try {
                delay(VIDEO_PREPARATION_TIMEOUT)

                // Check if we're still waiting for preparation
                if (!isVideoPrepared && isSurfaceReady && !isPlaying) {
                    android.util.Log.e("VideoPlayerFragment", "=== VIDEO PREPARATION TIMEOUT TRIGGERED ===")
                    android.util.Log.e("VideoPlayerFragment", "Video preparation took longer than $VIDEO_PREPARATION_TIMEOUT ms")
                    android.util.Log.e("VideoPlayerFragment", "Current state: prepared=$isVideoPrepared, surfaceReady=$isSurfaceReady, playing=$isPlaying")

                    // Attempt callback retry
                    retryCallbackIfNeeded()

                    if (!isVideoPrepared) {
                        // Still not prepared - show error
                        android.util.Log.e("VideoPlayerFragment", "Video preparation failed after timeout")
                        showErrorMessage("Video preparation timed out")
                        CrashLogger.e("Video preparation timeout", Exception("Video preparation took longer than $VIDEO_PREPARATION_TIMEOUT ms"))
                    }
                } else {
                    android.util.Log.i("VideoPlayerFragment", "Video preparation completed before timeout")
                }
            } catch (e: Exception) {
                android.util.Log.e("VideoPlayerFragment", "Error in video preparation timeout", e)
                CrashLogger.e("Video preparation timeout error", e)
            }
        }
    }

    /**
     * Reset playback state for new attempts
     */
    private fun resetPlaybackState() {
        android.util.Log.i("VideoPlayerFragment", "=== RESETTING PLAYBACK STATE ===")

        // Cancel any pending timeout jobs
        videoPreparationTimeoutJob?.cancel()
        videoPreparationTimeoutJob = null

        // Reset state flags
        isVideoPrepared = false
        isPlaying = false
        retryCount = 0
        callbackRetryCount = 0

        // Reset timing
        videoLoadStartTime = 0L
        lastCallbackReceivedTime = 0L

        android.util.Log.i("VideoPlayerFragment", "Playback state reset complete")
    }

    /**
     * Attempt to retry callback if JNI communication failed
     */
    private fun retryCallbackIfNeeded() {
        if (callbackRetryCount >= MAX_CALLBACK_RETRIES) {
            android.util.Log.e("VideoPlayerFragment", "Max callback retries ($MAX_CALLBACK_RETRIES) reached, giving up")
            return
        }

        callbackRetryCount++
        android.util.Log.w("VideoPlayerFragment", "Attempting callback retry $callbackRetryCount/$MAX_CALLBACK_RETRIES")

        // Try to manually check if video is actually prepared in native layer
        try {
            val currentPosition = videoPlayerController.getCurrentPosition()
            val duration = videoPlayerController.getDuration()

            android.util.Log.i("VideoPlayerFragment", "Manual state check - position: $currentPosition, duration: $duration")

            if (duration > 0 && !isVideoPrepared) {
                android.util.Log.i("VideoPlayerFragment", "Video appears to be prepared but callback was lost")
                android.util.Log.i("VideoPlayerFragment", "Manually triggering onVideoPrepared with duration: $duration")

                // Manually trigger the preparation callback
                onVideoPrepared(duration)
                return
            }
        } catch (e: Exception) {
            android.util.Log.e("VideoPlayerFragment", "Manual state check failed", e)
        }

        // If we get here, schedule another retry
        android.util.Log.i("VideoPlayerFragment", "Scheduling callback retry in $CALLBACK_RETRY_DELAY ms")
        viewLifecycleOwner.lifecycleScope.launch {
            delay(CALLBACK_RETRY_DELAY)
            retryCallbackIfNeeded()
        }
    }

    /**
     * Enhanced tryStartPlayback with additional safety checks
     */
    private fun tryStartPlaybackSafely() {
        android.util.Log.i("VideoPlayerFragment", "=== SAFE PLAYBACK ATTEMPT ===")

        // Additional safety checks
        if (!isAdded || !::videoPlayerController.isInitialized) {
            android.util.Log.w("VideoPlayerFragment", "Cannot start playback - fragment not ready")
            return
        }

        // Check if we should retry callback detection
        if (!isVideoPrepared && isSurfaceReady && (System.currentTimeMillis() - videoLoadStartTime) > (VIDEO_PREPARATION_TIMEOUT / 2)) {
            android.util.Log.i("VideoPlayerFragment", "Video preparation taking long, checking native state")
            retryCallbackIfNeeded()
        }

        // Proceed with normal playback attempt
        tryStartPlayback()
    }
}