package com.dorflix.app.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.SeekBar
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.lifecycle.lifecycleScope
import com.dorflix.app.R
import com.dorflix.app.databinding.FragmentVideoPlayerBinding
import com.dorflix.app.presentation.viewmodel.VideoViewModel
import com.dorflix.app.video.VideoPlayerController
import com.dorflix.app.video.VideoPlayerListener
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import java.util.*

/**
 * Fragment for video playback with C++ integration
 */
class VideoPlayerFragment : Fragment(), VideoPlayerListener {
    
    private var _binding: FragmentVideoPlayerBinding? = null
    private val binding get() = _binding!!
    
    private val viewModel: VideoViewModel by viewModels()
    
    private lateinit var videoPlayerController: VideoPlayerController
    private var videoId: String = ""
    
    private var isPlaying = false
    private var currentPosition = 0
    private var videoDuration = 0
    
    companion object {
        private const val ARG_VIDEO_ID = "video_id"
        
        fun newInstance(videoId: String): VideoPlayerFragment {
            val fragment = VideoPlayerFragment()
            val args = Bundle()
            args.putString(ARG_VIDEO_ID, videoId)
            fragment.arguments = args
            return fragment
        }
    }
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        arguments?.let {
            videoId = it.getString(ARG_VIDEO_ID, "")
        }
        
        // Initialize C++ video player
        videoPlayerController = VideoPlayerController(this)
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
    }
    
    private fun setupUI() {
        // Setup play/pause button
        binding.buttonPlayPause.setOnClickListener {
            togglePlayPause()
        }
        
        // Setup fullscreen button
        binding.buttonFullscreen.setOnClickListener {
            // Handle fullscreen toggle
            toggleFullscreen()
        }
        
        // Setup seek bar
        binding.seekBarProgress.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                if (fromUser) {
                    videoPlayerController.seekTo(progress)
                    updatePositionText(progress)
                }
            }
            
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })
        
        // Setup back button
        binding.buttonBack.setOnClickListener {
            requireActivity().onBackPressed()
        }
    }
    
    private fun setupVideoPlayer() {
        // Set up surface view for video rendering
        binding.surfaceView.holder.addCallback(object : android.view.SurfaceHolder.Callback {
            override fun surfaceCreated(holder: android.view.SurfaceHolder) {
                videoPlayerController.setSurface(holder.surface)
            }
            
            override fun surfaceChanged(
                holder: android.view.SurfaceHolder,
                format: Int,
                width: Int,
                height: Int
            ) {
                videoPlayerController.setSurface(holder.surface)
            }
            
            override fun surfaceDestroyed(holder: android.view.SurfaceHolder) {
                videoPlayerController.setSurface(null)
            }
        })
    }
    
    private fun loadVideo() {
        if (videoId.isNotEmpty()) {
            viewModel.loadVideoDetails(videoId)
            
            // Observe video details
            viewLifecycleOwner.lifecycleScope.launch {
                viewModel.currentVideo.collectLatest { video ->
                    video?.let {
                        startPlayback(it.videoUrl)
                    }
                }
            }
        }
    }
    
    private fun startPlayback(videoUrl: String) {
        try {
            videoPlayerController.loadVideo(videoUrl)
            binding.progressBar.visibility = View.VISIBLE
        } catch (e: Exception) {
            // Handle error
            showErrorMessage("Failed to load video")
        }
    }
    
    private fun togglePlayPause() {
        if (isPlaying) {
            videoPlayerController.pause()
            binding.buttonPlayPause.setImageResource(com.dorflix.app.R.drawable.ic_play)
        } else {
            videoPlayerController.play()
            binding.buttonPlayPause.setImageResource(com.dorflix.app.R.drawable.ic_pause)
        }
        isPlaying = !isPlaying
    }
    
    private fun toggleFullscreen() {
        // Handle fullscreen toggle
        // This would typically involve changing the fragment's layout
        // or navigating to a dedicated fullscreen activity
    }
    
    private fun updatePositionText(position: Int) {
        binding.textViewPosition.text = formatTime(position)
        binding.textViewDuration.text = formatTime(videoDuration)
    }
    
    private fun formatTime(milliseconds: Int): String {
        val seconds = milliseconds / 1000
        val minutes = seconds / 60
        val hours = minutes / 60
        
        return if (hours > 0) {
            String.format("%02d:%02d:%02d", hours, minutes % 60, seconds % 60)
        } else {
            String.format("%02d:%02d", minutes, seconds % 60)
        }
    }
    
    private fun showErrorMessage(message: String) {
        // Show error message to user
    }
    
    // VideoPlayerListener implementation
    override fun onVideoPrepared(duration: Int) {
        videoDuration = duration
        binding.seekBarProgress.max = duration
        binding.progressBar.visibility = View.GONE
        updatePositionText(0)
        
        // Auto-play when video is ready
        videoPlayerController.play()
        isPlaying = true
        binding.buttonPlayPause.setImageResource(com.dorflix.app.R.drawable.ic_pause)
    }
    
    override fun onVideoStarted() {
        isPlaying = true
        binding.buttonPlayPause.setImageResource(com.dorflix.app.R.drawable.ic_pause)
    }
    
    override fun onVideoPaused() {
        isPlaying = false
        binding.buttonPlayPause.setImageResource(com.dorflix.app.R.drawable.ic_play)
    }
    
    override fun onVideoStopped() {
        isPlaying = false
        binding.buttonPlayPause.setImageResource(com.dorflix.app.R.drawable.ic_play)
    }
    
    override fun onVideoCompleted() {
        isPlaying = false
        binding.buttonPlayPause.setImageResource(com.dorflix.app.R.drawable.ic_play)
        
        // Save watch progress as completed
        viewModel.saveWatchProgress(videoId, videoDuration, videoDuration)
    }
    
    override fun onVideoError(errorCode: Int, errorMessage: String) {
        binding.progressBar.visibility = View.GONE
        showErrorMessage("Video playback error: $errorMessage")
    }
    
    override fun onVideoProgressChanged(position: Int) {
        currentPosition = position
        binding.seekBarProgress.progress = position
        updatePositionText(position)
        
        // Save watch progress periodically (every 5 seconds)
        if (position % 5000 == 0) {
            viewModel.saveWatchProgress(videoId, position / 1000, videoDuration / 1000)
        }
    }
    
    override fun onVideoBufferingStarted() {
        binding.progressBar.visibility = View.VISIBLE
    }
    
    override fun onVideoBufferingEnded() {
        binding.progressBar.visibility = View.GONE
    }
    
    override fun onVideoSeekComplete() {
        // Seek completed
    }
    
    override fun onVideoSizeChanged(width: Int, height: Int) {
        // Handle video size changes
    }
    
    override fun onDestroyView() {
        super.onDestroyView()
        
        // Clean up video player
        videoPlayerController.release()
        _binding = null
    }
    
    override fun onPause() {
        super.onPause()
        if (isPlaying) {
            videoPlayerController.pause()
        }
    }
    
    override fun onResume() {
        super.onResume()
        if (isPlaying) {
            videoPlayerController.play()
        }
    }
}
