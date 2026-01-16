package com.dorflix.app.ui

import android.animation.Animator
import android.animation.AnimatorListenerAdapter
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.Surface
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.lifecycle.lifecycleScope
import androidx.viewpager2.widget.ViewPager2
import com.dorflix.app.DorflixApplication
import com.dorflix.app.MainActivity
import com.dorflix.app.R
import com.dorflix.app.databinding.FragmentFeedBinding
import com.dorflix.app.databinding.ItemVideoBinding
import com.dorflix.app.domain.model.Video
import com.dorflix.app.presentation.viewmodel.VideoViewModel
import com.dorflix.app.video.VideoPlayerController
import com.dorflix.app.video.VideoPlayerListener
import com.google.android.material.snackbar.Snackbar
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.launch

/**
 * Fragment for displaying full-screen video feed (Reels style) with auto-play scroll effect
 */
class FeedFragment : Fragment() {

    private var _binding: FragmentFeedBinding? = null
    private val binding get() = _binding!!

    private val viewModel: VideoViewModel by viewModels()
    private lateinit var videoAdapter: VideoAdapter
    private var hasAutoPlayed = false
    private var pagerInitialized = false  // Track when ViewPager has been initialized

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ): View {
        _binding = FragmentFeedBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        android.util.Log.i("FeedFragment", "=== FEED FRAGMENT VIEW CREATED ===")

        setupViewPager()
        setupObservers()

        viewModel.loadInitialCatalog()

        android.util.Log.i("FeedFragment", "=== FEED FRAGMENT INITIALIZATION COMPLETE ===")
    }

    private fun setupViewPager() {
        videoAdapter = VideoAdapter(binding.viewPager) { video ->
            // Handle video click (e.g., toggle play/pause)
        }

        binding.viewPager.apply {
            adapter = videoAdapter
            orientation = ViewPager2.ORIENTATION_VERTICAL

            // Handle auto-play on scroll
            registerOnPageChangeCallback(object : ViewPager2.OnPageChangeCallback() {
                override fun onPageSelected(position: Int) {
                    super.onPageSelected(position)
                    android.util.Log.d("VIEWPAGER_DEBUG", "=== onPageSelected called: position=$position ===")
                    android.util.Log.d("VIEWPAGER_DEBUG", "pagerInitialized: $pagerInitialized, hasAutoPlayed: $hasAutoPlayed")

                    // Ignore until ViewPager is initialized
                    if (!pagerInitialized) {
                        android.util.Log.d("VIEWPAGER_DEBUG", "❌ IGNORING - ViewPager not initialized yet")
                        return
                    }
                    android.util.Log.d("VIEWPAGER_DEBUG", "✅ ViewPager initialized, proceeding...")

                    // Handle initial auto-play for first video
                    if (position == 0 && !hasAutoPlayed) {
                        hasAutoPlayed = true
                        android.util.Log.i("VIEWPAGER_DEBUG", "🎬 Starting initial auto-play for first video")
                    }

                    // Start playback for current position, stop others
                    android.util.Log.d("VIEWPAGER_DEBUG", "Calling playVideoAtPosition($position)")
                    playVideoAtPosition(position)

                    // Trigger pagination load if near the end
                    if (position >= videoAdapter.itemCount - 2 && !viewModel.isLoading.value) {
                        android.util.Log.d("VIEWPAGER_DEBUG", "Triggering pagination load")
                        viewModel.loadVideoCatalog(videoAdapter.itemCount)
                    }
                    android.util.Log.d("VIEWPAGER_DEBUG", "=== onPageSelected complete ===")
                }

                override fun onPageScrollStateChanged(state: Int) {
                    super.onPageScrollStateChanged(state)
                    android.util.Log.d("VIEWPAGER_DEBUG", "onPageScrollStateChanged: state=$state")
                }

                override fun onPageScrolled(position: Int, positionOffset: Float, positionOffsetPixels: Int) {
                    super.onPageScrolled(position, positionOffset, positionOffsetPixels)
                    // Log less frequently to avoid spam
                    if (positionOffset == 0f) {
                        android.util.Log.d("VIEWPAGER_DEBUG", "onPageScrolled: position=$position, offset=$positionOffset")
                    }
                }
            })
        }
    }

    private fun playVideoAtPosition(position: Int) {
        try {
            android.util.Log.d("PLAY_DEBUG", "=== playVideoAtPosition($position) ENTERED ===")
            android.util.Log.d("PLAY_DEBUG", "hasAutoPlayed: $hasAutoPlayed")

            // Allow first playback to proceed, but block conflicting scroll playbacks
            if (hasAutoPlayed && position != 0) {
                android.util.Log.d("PLAY_DEBUG", "❌ Blocking scroll auto-play for position $position")
                return
            }

            android.util.Log.d("PLAY_DEBUG", "✅ Proceeding with playback for position $position")
            android.util.Log.d("PLAY_DEBUG", "videoAdapter: $videoAdapter")
            android.util.Log.d("PLAY_DEBUG", "videoAdapter.currentList.size: ${videoAdapter.currentList.size}")

            // Get video at current scroll position
            val video = videoAdapter.currentList.getOrNull(position)
            android.util.Log.d("PLAY_DEBUG", "video at position $position: $video")
            if (video != null) {
                android.util.Log.d("PLAY_DEBUG", "video.id: ${video.id}")
                android.util.Log.d("PLAY_DEBUG", "video.video_url: '${video.video_url}'")
                android.util.Log.d("PLAY_DEBUG", "video.video_url.isNotBlank(): ${video.video_url.isNotBlank()}")
            }

            if (video != null && video.video_url.isNotBlank()) {
                android.util.Log.i("PLAY_DEBUG", "✅ Found valid video at position $position: ${video.id}")
                android.util.Log.d("PLAY_DEBUG", "About to call videoAdapter.startPlaybackAtPosition($position)")
                videoAdapter.startPlaybackAtPosition(position)
                android.util.Log.d("PLAY_DEBUG", "✅ startPlaybackAtPosition($position) called successfully")
            } else {
                android.util.Log.w("PLAY_DEBUG", "❌ No valid video at position $position")
                if (video == null) android.util.Log.w("PLAY_DEBUG", "  - video is null")
                if (video != null && !video.video_url.isNotBlank()) android.util.Log.w("PLAY_DEBUG", "  - video URL is blank")
            }
            android.util.Log.d("PLAY_DEBUG", "=== playVideoAtPosition($position) EXIT SUCCESS ===")
        } catch (e: Exception) {
            android.util.Log.e("PLAY_DEBUG", "❌ EXCEPTION in playVideoAtPosition($position): ${e.message}")
            android.util.Log.e("PLAY_DEBUG", "Stack trace: ${e.stackTraceToString()}")
        } catch (e: Throwable) {
            android.util.Log.e("PLAY_DEBUG", "❌ THROWABLE in playVideoAtPosition($position): ${e.message}")
        }
    }

    private fun setupObservers() {
        android.util.Log.i("FeedFragment", "=== SETTING UP SINGLE OBSERVER ===")

        viewLifecycleOwner.lifecycleScope.launch {
            android.util.Log.i("FeedFragment", "=== LAUNCHING COMBINED OBSERVER ===")

            // Single observer combining all three flows: videoCatalog, isLoading, error
            combine(
                viewModel.videoCatalog,
                viewModel.isLoading,
                viewModel.error
            ) { videos, isLoading, error ->
                Triple(videos, isLoading, error)
            }.collectLatest { (videos, isLoading, error) ->

                // Handle loading state
                android.util.Log.i("FeedFragment", "=== LOADING STATE: $isLoading ===")
                binding.swipeRefresh.isRefreshing = isLoading

                // Handle errors
                android.util.Log.i("FeedFragment", "=== ERROR RECEIVED: $error ===")
                error?.let {
                    Snackbar.make(binding.root, it, Snackbar.LENGTH_LONG).show()
                    viewModel.clearError()
                }

                // Handle video catalog updates
                android.util.Log.i("FeedFragment", "=== VIDEO CATALOG RECEIVED: ${videos.size} videos ===")
                android.util.Log.i("FeedFragment", "hasAutoPlayed=$hasAutoPlayed, videos.isNotEmpty()=${videos.isNotEmpty()}")
                android.util.Log.i("FeedFragment", "pagerInitialized=$pagerInitialized")
                videoAdapter.submitList(videos) {
                    android.util.Log.i("FeedFragment", "submitList callback executed")
                    if (!pagerInitialized && videos.isNotEmpty()) {
                        android.util.Log.i("FeedFragment", "Setting pagerInitialized = true")
                        pagerInitialized = true
                        android.util.Log.i("FeedFragment", "ViewPager initialized")

                        // Initialize MediaCodec enumeration when feed is ready for autoplay
                        // This ensures JNI is loaded (via VideoPlayerController classes) before codec enumeration
                        initializeCodecEnumerationWhenReady()

                        // Trigger immediate auto-play for first video on app launch
                        if (!hasAutoPlayed) {
                            hasAutoPlayed = true
                            android.util.Log.i("FeedFragment", "Triggering immediate auto-play for first video")
                            playVideoAtPosition(0)
                        }
                    } else {
                        android.util.Log.i("FeedFragment", "Not initializing ViewPager - pagerInitialized=$pagerInitialized, videos.isNotEmpty()=${videos.isNotEmpty()}")
                    }
                }

                // Auto-play will be handled by ViewPager2 onPageSelected callback
            }
        }

        android.util.Log.i("FeedFragment", "=== SINGLE OBSERVER SETUP COMPLETE ===")
    }

    /**
     * Initialize MediaCodec enumeration when feed is ready for autoplay
     * This ensures JNI is loaded (via VideoPlayerController classes) before codec enumeration
     */
    private fun initializeCodecEnumerationWhenReady() {
        try {
            android.util.Log.i("FeedFragment", "🎯 Initializing MediaCodec enumeration when feed ready for autoplay")

            // Check JNI readiness before proceeding
            val isJniReady = isJNILibraryReady()
            android.util.Log.i("FeedFragment", "JNI readiness check: $isJniReady")

            if (!isJniReady) {
                android.util.Log.w("FeedFragment", "⚠️ JNI not ready yet, deferring codec enumeration")
                // Retry after a short delay
                view?.postDelayed({
                    initializeCodecEnumerationWhenReady()
                }, 500) // 500ms delay
                return
            }

            android.util.Log.i("FeedFragment", "✅ JNI is ready, proceeding with codec enumeration")

            // Trigger codec enumeration in background thread to avoid blocking UI
            Thread {
                try {
                    com.dorflix.app.CodecEnumerator.enumerateAllCodecs()
                    android.util.Log.i("FeedFragment", "✅ MediaCodec enumeration completed successfully")
                } catch (e: Exception) {
                    android.util.Log.e("FeedFragment", "❌ Failed to enumerate MediaCodecs: ${e.message}", e)
                }
            }.start()

        } catch (e: Exception) {
            android.util.Log.e("FeedFragment", "❌ Failed to initialize codec enumeration: ${e.message}", e)
        }
    }

    /**
     * Check if JNI library is ready
     * @return true if JNI_OnLoad has completed successfully
     */
    private fun isJNILibraryReady(): Boolean {
        return (requireActivity().application as DorflixApplication).isJNILibraryReadySafe()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}

class VideoAdapter(
    private val viewPager: ViewPager2,
    private val onVideoClick: (Video) -> Unit
) : androidx.recyclerview.widget.ListAdapter<Video, VideoAdapter.VideoViewHolder>(VideoDiffCallback()) {

    private var currentPlayingPosition = -1
    private val SCROLL_BUFFER = 2 // Keep 2 videos alive around current position
    private val playbackState = mutableMapOf<String, Int>() // videoId -> current position (ms)

    // Deferred playback tracking
    private var pendingPlaybackPosition = -1
    private var pendingPlaybackSeekPosition = 0
    private var playbackStartedForPosition = -1  // Track if playback actually started

    fun startPlaybackAtPosition(position: Int) {
        try {
            android.util.Log.d("START_DEBUG", "=== startPlaybackAtPosition($position) START ===")

            // Stop videos that are outside the buffer
            for (i in 0 until itemCount) {
                if (kotlin.math.abs(i - position) > SCROLL_BUFFER) {
                    stopPlaybackAtPosition(i)
                }
            }
            android.util.Log.d("START_DEBUG", "Stopped videos outside buffer")

            // Preload videos in the buffer zone (current + 2 ahead)
            preloadVideosInBuffer(position)
            android.util.Log.d("START_DEBUG", "Preloaded videos in buffer")

            // Store pending playback info
            android.util.Log.d("START_DEBUG", "Setting pendingPlaybackPosition = $position")
            pendingPlaybackPosition = position
            android.util.Log.d("START_DEBUG", "pendingPlaybackPosition set to: $pendingPlaybackPosition")

            val video = getItem(position)
            val seekPosition = getSavedPosition(video.id)
            pendingPlaybackSeekPosition = seekPosition
            android.util.Log.d("START_DEBUG", "pendingPlaybackSeekPosition set to: $seekPosition")

            // Try immediate playback
            android.util.Log.d("START_DEBUG", "Calling tryStartPlaybackForPosition($position)")
            tryStartPlaybackForPosition(position)

            // Fallback: retry after short delay if ViewHolder not found and playback hasn't started
            viewPager.postDelayed({
                if (pendingPlaybackPosition == position && playbackStartedForPosition != position) {
                    android.util.Log.d("START_DEBUG", "Fallback: retrying tryStartPlaybackForPosition($position)")
                    tryStartPlaybackForPosition(position)
                } else {
                    android.util.Log.d("START_DEBUG", "Fallback skipped - pendingPlaybackPosition: $pendingPlaybackPosition, playbackStartedForPosition: $playbackStartedForPosition")
                }
            }, 100)

            android.util.Log.d("START_DEBUG", "=== startPlaybackAtPosition($position) SUCCESS ===")
        } catch (e: Exception) {
            android.util.Log.e("START_DEBUG", "❌ EXCEPTION in startPlaybackAtPosition($position): ${e.message}")
            android.util.Log.e("START_DEBUG", "Stack trace: ${e.stackTraceToString()}")
        } catch (e: Throwable) {
            android.util.Log.e("START_DEBUG", "❌ THROWABLE in startPlaybackAtPosition($position): ${e.message}")
        }
    }

    private fun tryStartPlaybackForPosition(position: Int) {
        android.util.Log.d("START_DEBUG", "tryStartPlaybackForPosition($position) called")
        val recyclerView = viewPager.getChildAt(0) as? androidx.recyclerview.widget.RecyclerView
        val holder = recyclerView?.findViewHolderForAdapterPosition(position) as? VideoViewHolder
        android.util.Log.d("START_DEBUG", "holder: $holder, pendingPlaybackPosition: $pendingPlaybackPosition")

        if (holder != null && position == pendingPlaybackPosition) {
            android.util.Log.d("START_DEBUG", "✅ Starting immediate playback for position $position")
            val video = getItem(position)
            holder.startPlayback(video, holder.binding.surfaceViewVideo.holder.surface, pendingPlaybackSeekPosition)
            currentPlayingPosition = position
            playbackStartedForPosition = position  // Mark playback as started
            android.util.Log.d("START_DEBUG", "playbackStartedForPosition set to $position")
            // Don't reset pendingPlaybackPosition here - let surfaceCreated handle it
        } else {
            android.util.Log.d("START_DEBUG", "❌ Cannot start immediate playback")
            if (holder == null) android.util.Log.d("START_DEBUG", "  - holder is null")
            if (position != pendingPlaybackPosition) android.util.Log.d("START_DEBUG", "  - position ($position) != pendingPlaybackPosition ($pendingPlaybackPosition)")
        }
    }

    private fun preloadVideosInBuffer(currentPosition: Int) {
        // Preload videos from current position up to buffer size ahead
        val preloadStart = currentPosition
        val preloadEnd = minOf(currentPosition + SCROLL_BUFFER + 1, itemCount - 1)

        for (i in preloadStart..preloadEnd) {
            if (i != currentPosition) { // Don't preload currently playing video
                val video = getItem(i)
                // Preload in background if not already cached
                if (!video.video_url.isBlank()) {
                    android.util.Log.d("VideoAdapter", "Preloading video at position $i: ${video.id}")
                    // Call VideoDownloader preload method
                    com.dorflix.app.video.VideoDownloader.nativePreloadVideo(video.video_url)
                }
            }
        }
    }

    fun stopPlaybackAtPosition(position: Int) {
        val recyclerView = viewPager.getChildAt(0) as? androidx.recyclerview.widget.RecyclerView
        val holder = recyclerView?.findViewHolderForAdapterPosition(position) as? VideoViewHolder
        if (holder != null) {
            val video = getItem(position)
            val currentPosition = holder.getCurrentPosition()
            savePosition(video.id, currentPosition)
            holder.stopPlayback()
        }
        if (currentPlayingPosition == position) {
            currentPlayingPosition = -1
        }
    }

    fun pausePlaybackAtPosition(position: Int) {
        val recyclerView = viewPager.getChildAt(0) as? androidx.recyclerview.widget.RecyclerView
        val holder = recyclerView?.findViewHolderForAdapterPosition(position) as? VideoViewHolder
        if (holder != null) {
            val video = getItem(position)
            val currentPosition = holder.getCurrentPosition()
            savePosition(video.id, currentPosition)
            holder.pausePlayback()
        }
    }

    fun getSavedPosition(videoId: String): Int = playbackState[videoId] ?: 0
    private fun savePosition(videoId: String, position: Int) {
        playbackState[videoId] = position
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VideoViewHolder {
        val binding = ItemVideoBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        return VideoViewHolder(binding)
    }

    override fun onBindViewHolder(holder: VideoViewHolder, position: Int) {
        holder.bind(getItem(position), currentPlayingPosition)
    }

    override fun onViewAttachedToWindow(holder: VideoViewHolder) {
        super.onViewAttachedToWindow(holder)
        holder.onViewAttachedToWindow()
    }

    override fun onViewDetachedFromWindow(holder: VideoViewHolder) {
        super.onViewDetachedFromWindow(holder)
        holder.onViewDetachedFromWindow()
    }

    inner class VideoViewHolder(val binding: ItemVideoBinding) :
        androidx.recyclerview.widget.RecyclerView.ViewHolder(binding.root) {

        private var videoPlayerController: VideoPlayerController? = null
        private var currentVideo: Video? = null
        private var isPlaying = false
        private var isViewAttached = false
        private var pendingPlaybackResume = false
        private var isVideoLoaded = false
        private var isSurfaceReady = false
        private var pendingSeekPosition = 0

        private val playerListener = object : VideoPlayerListener {
            override fun onVideoPrepared(duration: Int) {
                // Synchronous loading - video is already playing by this point
            }

            override fun onVideoStarted() {
                isPlaying = true
            }

            override fun onVideoPaused() {
                isPlaying = false
            }

            override fun onVideoStopped() {
                isPlaying = false
            }

            override fun onVideoCompleted() {
                isPlaying = false
                // Loop the video by restarting playback
                videoPlayerController?.seekTo(0)
                videoPlayerController?.play()
            }

            override fun onVideoError(errorCode: Int, errorMessage: String) {
                android.util.Log.e("VideoViewHolder", "Video error: $errorCode - $errorMessage")
                isPlaying = false
            }

            override fun onVideoProgressChanged(position: Int) {
                // Handle progress updates if needed
            }

            override fun onVideoBufferingStarted() {
                android.util.Log.d("VideoViewHolder", "Buffering started")
            }

            override fun onVideoBufferingEnded() {
                android.util.Log.d("VideoViewHolder", "Buffering ended")
            }

            override fun onVideoSeekComplete() {
                android.util.Log.d("VideoViewHolder", "Seek complete")
            }

            override fun onVideoSizeChanged(width: Int, height: Int) {
                android.util.Log.d("VideoViewHolder", "Video size changed: ${width}x${height}")
            }
        }

        init {
            // Handle SurfaceView lifecycle with proper state management
            binding.surfaceViewVideo.holder.addCallback(object : android.view.SurfaceHolder.Callback {
                override fun surfaceCreated(holder: android.view.SurfaceHolder) {
                    android.util.Log.d("SURFACE_DEBUG", "=== SURFACE CREATED ===")
                    android.util.Log.d("SURFACE_DEBUG", "adapterPosition: $adapterPosition")
                    isSurfaceReady = true

                    // Check if this ViewHolder should start playback
                    val recyclerView = binding.root.parent as? androidx.recyclerview.widget.RecyclerView
                    val adapter = recyclerView?.adapter as? VideoAdapter
                    android.util.Log.d("SURFACE_DEBUG", "recyclerView: $recyclerView")
                    android.util.Log.d("SURFACE_DEBUG", "adapter: $adapter")
                    android.util.Log.d("SURFACE_DEBUG", "pendingPlaybackPosition: ${adapter?.pendingPlaybackPosition}")

                    // Determine if deferred playback is needed
                    val needsDeferredPlayback = (adapterPosition >= 0 && adapter != null &&
                                               adapter.pendingPlaybackPosition == adapterPosition &&
                                               adapter.playbackStartedForPosition != adapterPosition)

                    if (needsDeferredPlayback) {
                        android.util.Log.d("SURFACE_DEBUG", "✅ DEFERRED PLAYBACK NEEDED - Starting playback")
                        android.util.Log.d("SURFACE_DEBUG", "playbackStartedForPosition: ${adapter.playbackStartedForPosition}")
                        val video = adapter.getItem(adapterPosition)
                        val seekPosition = adapter.pendingPlaybackSeekPosition
                        android.util.Log.d("SURFACE_DEBUG", "Video: ${video.id}, seekPosition: $seekPosition")
                        startPlayback(video, holder.surface, seekPosition)
                        adapter.currentPlayingPosition = adapterPosition
                        adapter.pendingPlaybackPosition = -1  // Clear pending
                        adapter.playbackStartedForPosition = adapterPosition  // Mark as started
                        android.util.Log.d("SURFACE_DEBUG", "✅ DEFERRED PLAYBACK STARTED")
                    } else {
                        android.util.Log.d("SURFACE_DEBUG", "ℹ️ DEFERRED PLAYBACK NOT NEEDED")
                        if (adapterPosition < 0) android.util.Log.d("SURFACE_DEBUG", "  - adapterPosition < 0")
                        if (adapter == null) android.util.Log.d("SURFACE_DEBUG", "  - adapter is null")
                        if (adapter != null && adapter.pendingPlaybackPosition != adapterPosition) {
                            android.util.Log.d("SURFACE_DEBUG", "  - pendingPlaybackPosition (${adapter.pendingPlaybackPosition}) != adapterPosition ($adapterPosition)")
                        }
                        if (adapter != null && adapter.playbackStartedForPosition == adapterPosition) {
                            android.util.Log.d("SURFACE_DEBUG", "  - playback already started for position $adapterPosition")
                        }

                        // Only call checkAndStartPlayback if we didn't do deferred playback
                        // This handles cases where video loading and surface creation happen in different order
                        android.util.Log.d("SURFACE_DEBUG", "Calling checkAndStartPlayback for immediate playback check")
                        checkAndStartPlayback(holder.surface)
                    }
                    android.util.Log.d("SURFACE_DEBUG", "=== SURFACE CREATED COMPLETE ===")
                }

                override fun surfaceChanged(holder: android.view.SurfaceHolder, format: Int, width: Int, height: Int) {
                    // Handle surface changes if needed
                }

                override fun surfaceDestroyed(holder: android.view.SurfaceHolder) {
                    isSurfaceReady = false
                    stopPlayback()
                }
            })

            // Handle tap on SurfaceView for play/pause button
            binding.surfaceViewVideo.setOnClickListener {
                togglePlayPause()
            }

            // Handle play/pause button click
            binding.imageViewPlayPause.setOnClickListener {
                togglePlayPause()
            }
        }

        private fun togglePlayPause() {
            if (isPlaying) {
                pausePlayback()
                showPlayPauseButton(true)
            } else {
                if (currentVideo != null) {
                    startPlayback(currentVideo!!, binding.surfaceViewVideo.holder.surface)
                    showPlayPauseButton(true)
                }
            }
        }

        private fun showPlayPauseButton(shouldFadeOut: Boolean = true) {
            // Update button icon
            val iconRes = if (isPlaying) R.drawable.ic_pause else R.drawable.ic_play
            binding.imageViewPlayPause.setImageResource(iconRes)

            // Show button
            binding.imageViewPlayPause.visibility = View.VISIBLE
            binding.imageViewPlayPause.alpha = 1.0f

            if (shouldFadeOut) {
                // Auto-hide after 3 seconds
                binding.imageViewPlayPause.postDelayed({
                    fadeOutPlayPauseButton()
                }, 3000)
            }
        }

        private fun fadeOutPlayPauseButton() {
            binding.imageViewPlayPause.animate()
                .alpha(0.0f)
                .setDuration(500)
                .setListener(object : AnimatorListenerAdapter() {
                    override fun onAnimationEnd(animation: Animator) {
                        binding.imageViewPlayPause.visibility = View.GONE
                    }
                })
                .start()
        }

        fun bind(video: Video, currentPlayingPosition: Int) {
            currentVideo = video
            binding.apply {
                textViewTitle.text = video.title
                textViewUploader.text = "by ${video.uploader_name} • 1h ago"
                textViewLikes.text = formatCount(video.likes_count)
                textViewComments.text = formatCount(video.comments_count)
                textViewShares.text = formatCount(video.shares_count)
                textViewMusic.text = "Original Sound - ${video.uploader_name}"

                // Video is now the main focus - SurfaceView always visible
                surfaceViewVideo.visibility = View.VISIBLE

                root.setOnClickListener { onVideoClick(video) }

                // Start/stop playback based on adapter's current position
                if (adapterPosition == currentPlayingPosition) {
                    if (!isPlaying) {
                        val savedPosition = (binding.root.parent as? androidx.recyclerview.widget.RecyclerView)?.adapter as? VideoAdapter
                        val seekPosition = savedPosition?.getSavedPosition(video.id) ?: 0
                        startPlayback(video, surfaceViewVideo.holder.surface, seekPosition)
                    }
                } else {
                    if (isPlaying) {
                        stopPlayback()
                    }
                }
            }
        }

        fun startPlayback(video: Video, surface: Surface, seekToPosition: Int = 0) {
            if (isPlaying && currentVideo?.id == video.id) {
                return
            }

            // Stop any existing playback
            stopPlayback()
            resetPlaybackState()

            currentVideo = video
            pendingSeekPosition = seekToPosition

            // Check if video is cached
            val cachedPath = com.dorflix.app.video.VideoDownloader.nativeGetCachedPath(video.video_url)
            android.util.Log.d("CACHE_DEBUG", "Video: ${video.id}")
            android.util.Log.d("CACHE_DEBUG", "Original URL: ${video.video_url}")
            android.util.Log.d("CACHE_DEBUG", "Cached path: '$cachedPath'")

            if (cachedPath.isBlank()) {
                android.util.Log.w("CACHE_DEBUG", "⚠️ Video not cached yet: ${video.id} - downloading now...")

                // Download video synchronously (blocks until download completes)
                try {
                    val downloadResult = com.dorflix.app.video.VideoDownloader.nativeDownloadVideo(video.video_url)
                    android.util.Log.d("CACHE_DEBUG", "Download result: '$downloadResult'")

                    if (downloadResult.isNotBlank()) {
                        android.util.Log.d("CACHE_DEBUG", "✅ Video downloaded successfully: $downloadResult")
                        val videoUrl = downloadResult
                        createAndStartPlayback(videoUrl, surface)
                    } else {
                        android.util.Log.e("CACHE_DEBUG", "❌ Video download failed - cannot play")
                        resetPlaybackState()
                    }
                } catch (e: Exception) {
                    android.util.Log.e("CACHE_DEBUG", "❌ Exception downloading video: ${e.message}")
                    resetPlaybackState()
                }
            } else {
                android.util.Log.d("CACHE_DEBUG", "✅ Using cached video: $cachedPath")
                val videoUrl = cachedPath  // Already cached
                createAndStartPlayback(videoUrl, surface)
            }
        }

        private fun createAndStartPlayback(videoUrl: String, surface: Surface) {
            // Create new controller
            videoPlayerController = VideoPlayerController(playerListener)

            // Load video synchronously (blocks until fully prepared)
            try {
                val loaded = videoPlayerController?.loadVideo(videoUrl) ?: false
                if (loaded) {
                    android.util.Log.d("CACHE_DEBUG", "✅ Video loaded successfully")
                    isVideoLoaded = true
                    // Video is now the main focus - SurfaceView always visible
                    binding.surfaceViewVideo.visibility = View.VISIBLE
                    checkAndStartPlayback(surface)
                } else {
                    android.util.Log.e("CACHE_DEBUG", "❌ Failed to load video: $videoUrl")
                    resetPlaybackState()
                }
            } catch (e: Exception) {
                android.util.Log.e("CACHE_DEBUG", "❌ Exception loading video: ${e.message}")
                resetPlaybackState()
            }
        }

        private fun checkAndStartPlayback(surface: Surface) {
            android.util.Log.d("PLAYBACK_DEBUG", "checkAndStartPlayback called - isVideoLoaded: $isVideoLoaded, isSurfaceReady: $isSurfaceReady, isPlaying: $isPlaying, currentVideo: $currentVideo")

            if (isVideoLoaded && isSurfaceReady && !isPlaying && currentVideo != null) {
                android.util.Log.d("PLAYBACK_DEBUG", "✅ All conditions met - starting playback")
                android.util.Log.d("PLAYBACK_DEBUG", "Setting surface: $surface")

                // VALIDATE SURFACE BEFORE USING
                if (!surface.isValid) {
                    android.util.Log.e("PLAYBACK_DEBUG", "❌ SURFACE VALIDATION FAILED - Surface is not valid!")
                    return
                }
                android.util.Log.d("PLAYBACK_DEBUG", "✅ Surface validation passed")

                // Try to lock canvas to further validate surface
                try {
                    val canvas = surface.lockCanvas(null)
                    if (canvas != null) {
                        android.util.Log.d("PLAYBACK_DEBUG", "Surface canvas size: ${canvas.width}x${canvas.height}")
                        surface.unlockCanvasAndPost(canvas)
                        android.util.Log.d("PLAYBACK_DEBUG", "✅ Surface canvas test passed")
                    } else {
                        android.util.Log.e("PLAYBACK_DEBUG", "❌ SURFACE VALIDATION FAILED - Cannot lock surface canvas!")
                        return
                    }
                } catch (e: Exception) {
                    android.util.Log.e("PLAYBACK_DEBUG", "❌ SURFACE VALIDATION FAILED - Exception testing surface: ${e.message}")
                    return
                }

                // Set surface now that it's ready and validated
                videoPlayerController?.setSurface(surface)
                android.util.Log.d("PLAYBACK_DEBUG", "Surface set successfully")

                // Apply seek position if specified
                if (pendingSeekPosition > 0) {
                    android.util.Log.d("PLAYBACK_DEBUG", "Seeking to position: $pendingSeekPosition")
                    videoPlayerController?.seekTo(pendingSeekPosition)
                    pendingSeekPosition = 0
                }

                // Start playback
                android.util.Log.d("PLAYBACK_DEBUG", "Calling play()")
                videoPlayerController?.play()
                isPlaying = true
                android.util.Log.d("PLAYBACK_DEBUG", "✅ Playback started, isPlaying set to true")
            } else {
                android.util.Log.d("PLAYBACK_DEBUG", "❌ Conditions not met for playback")
                if (!isVideoLoaded) android.util.Log.d("PLAYBACK_DEBUG", "  - video not loaded")
                if (!isSurfaceReady) android.util.Log.d("PLAYBACK_DEBUG", "  - surface not ready")
                if (isPlaying) android.util.Log.d("PLAYBACK_DEBUG", "  - already playing")
                if (currentVideo == null) android.util.Log.d("PLAYBACK_DEBUG", "  - no current video")
            }
        }

        private fun resetPlaybackState() {
            isVideoLoaded = false
            isSurfaceReady = false
            pendingSeekPosition = 0
            currentVideo = null
        }

        fun stopPlayback() {
            if (!isPlaying) return

            android.util.Log.d("VideoViewHolder", "Stopping playback for video: ${currentVideo?.id}")

            videoPlayerController?.stop()
            videoPlayerController?.release()
            videoPlayerController = null
            isPlaying = false

            // Video focus maintained - SurfaceView stays visible
            binding.surfaceViewVideo.visibility = View.VISIBLE
        }

        fun pausePlayback() {
            videoPlayerController?.pause()
        }

        fun resumePlayback() {
            if (!isPlaying && videoPlayerController != null) {
                android.util.Log.d("VideoViewHolder", "Resuming playback for video: ${currentVideo?.id}")
                videoPlayerController?.resume()
                isPlaying = true
            }
        }



        fun getCurrentPosition(): Int {
            return videoPlayerController?.getCurrentPosition() ?: 0
        }

        // View lifecycle callbacks for visibility management
        fun onViewAttachedToWindow() {
            android.util.Log.d("VideoViewHolder", "View attached to window for position $adapterPosition")
            isViewAttached = true

            // Resume playback if it was pending (paused when view was detached)
            if (pendingPlaybackResume && currentVideo != null) {
                android.util.Log.d("VideoViewHolder", "Resuming pending playback for position $adapterPosition")
                pendingPlaybackResume = false
                resumePlayback()
            }
            // If this was the currently playing video and should auto-resume
            else if (adapterPosition == viewPager.currentItem && currentVideo != null && !isPlaying && videoPlayerController != null) {
                android.util.Log.d("VideoViewHolder", "Auto-resuming playback for current item at position $adapterPosition")
                resumePlayback()
            }
        }

        fun onViewDetachedFromWindow() {
            android.util.Log.d("VideoViewHolder", "View detached from window for position $adapterPosition")
            isViewAttached = false

            // Pause playback when view is not visible, but keep resources for quick resume
            if (isPlaying) {
                android.util.Log.d("VideoViewHolder", "Pausing playback for detached view at position $adapterPosition")
                videoPlayerController?.pause()
                pendingPlaybackResume = true
                isPlaying = false
            }
        }
    }
}

class VideoDiffCallback : androidx.recyclerview.widget.DiffUtil.ItemCallback<Video>() {
    override fun areItemsTheSame(oldItem: Video, newItem: Video): Boolean = oldItem.id == newItem.id
    override fun areContentsTheSame(oldItem: Video, newItem: Video): Boolean = oldItem == newItem
}

// Simple VideoAdapter for LibraryFragment and SearchFragment (no TikTok-style playback)
class SimpleVideoAdapter(
    private val onVideoClick: (Video) -> Unit
) : androidx.recyclerview.widget.ListAdapter<Video, SimpleVideoAdapter.SimpleVideoViewHolder>(VideoDiffCallback()) {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): SimpleVideoViewHolder {
        val binding = ItemVideoBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        return SimpleVideoViewHolder(binding)
    }

    override fun onBindViewHolder(holder: SimpleVideoViewHolder, position: Int) {
        holder.bind(getItem(position))
    }

    inner class SimpleVideoViewHolder(private val binding: ItemVideoBinding) :
        androidx.recyclerview.widget.RecyclerView.ViewHolder(binding.root) {

        fun bind(video: Video) {
            binding.apply {
                textViewTitle.text = video.title
                textViewUploader.text = "by ${video.uploader_name} • 1h ago"
                textViewLikes.text = formatCount(video.likes_count)
                textViewComments.text = formatCount(video.comments_count)
                textViewShares.text = formatCount(video.shares_count)
                textViewMusic.text = "Original Sound - ${video.uploader_name}"

                // No video playback in library/search - SurfaceView hidden
                surfaceViewVideo.visibility = View.GONE

                root.setOnClickListener { onVideoClick(video) }
            }
        }
    }
}

private fun formatCount(count: Int): String {
    return when {
        count >= 1000000 -> String.format("%.1fM", count / 1000000.0)
        count >= 1000 -> String.format("%.1fK", count / 1000.0)
        else -> count.toString()
    }
}