package com.dorflix.app.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import com.dorflix.app.MainActivity
import com.dorflix.app.R
import com.dorflix.app.databinding.FragmentLibraryBinding
import com.dorflix.app.presentation.viewmodel.VideoViewModel
import com.dorflix.app.domain.model.Video
import com.dorflix.app.ui.SimpleVideoAdapter
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

/**
 * Fragment for displaying user's library (continue watching, liked videos, etc.)
 */
class LibraryFragment : Fragment() {
    
    private var _binding: FragmentLibraryBinding? = null
    private val binding get() = _binding!!
    
    private val viewModel: VideoViewModel by lazy { VideoViewModel() }
    private lateinit var videoAdapter: SimpleVideoAdapter

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentLibraryBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        setupRecyclerView()
        setupObservers()

        // Load continue watching videos
        viewModel.loadContinueWatching()
    }

    private fun setupRecyclerView() {
        videoAdapter = SimpleVideoAdapter { video ->
            // Handle video click - navigate to player
            (activity as? MainActivity)?.openVideoPlayer(video.id)
        }
        
        binding.recyclerView.apply {
            adapter = videoAdapter
            layoutManager = LinearLayoutManager(context, LinearLayoutManager.VERTICAL, false)
        }
    }
    
    private fun setupObservers() {
        viewLifecycleOwner.lifecycleScope.launch {
            viewModel.continueWatching.collectLatest { videos ->
                videoAdapter.submitList(videos)
            }
        }
    }
    
    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
