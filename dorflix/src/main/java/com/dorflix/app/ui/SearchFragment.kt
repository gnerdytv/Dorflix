package com.dorflix.app.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.fragment.app.viewModels
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.GridLayoutManager
import com.dorflix.app.MainActivity
import com.dorflix.app.R
import com.dorflix.app.databinding.FragmentSearchBinding
import com.dorflix.app.presentation.viewmodel.VideoViewModel
import com.dorflix.app.domain.model.Video
import com.dorflix.app.ui.SimpleVideoAdapter
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

/**
 * Fragment for searching videos
 */
class SearchFragment : Fragment() {
    
    private var _binding: FragmentSearchBinding? = null
    private val binding get() = _binding!!
    
    private val viewModel: VideoViewModel by lazy { VideoViewModel() }
    private lateinit var videoAdapter: SimpleVideoAdapter
    
    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentSearchBinding.inflate(inflater, container, false)
        return binding.root
    }
    
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        
        setupRecyclerView()
        setupSearchView()
        setupObservers()
    }
    
    private fun setupRecyclerView() {
        videoAdapter = SimpleVideoAdapter { video ->
            // Handle video click - navigate to player
            (activity as? MainActivity)?.openVideoPlayer(video.id)
        }
        
        binding.recyclerView.apply {
            adapter = videoAdapter
            layoutManager = GridLayoutManager(context, 2)
        }
    }
    
    private fun setupSearchView() {
        binding.searchView.apply {
            queryHint = "Search videos..."
            setOnQueryTextListener(object : androidx.appcompat.widget.SearchView.OnQueryTextListener {
                override fun onQueryTextSubmit(query: String?): Boolean {
                    query?.let { viewModel.searchVideos(it) }
                    return true
                }
                
                override fun onQueryTextChange(newText: String?): Boolean {
                    if (newText.isNullOrEmpty()) {
                        viewModel.clearSearch()
                    } else if (newText.length >= 3) {
                        viewModel.searchVideos(newText)
                    }
                    return true
                }
            })
        }
    }
    
    private fun setupObservers() {
        viewLifecycleOwner.lifecycleScope.launch {
            viewModel.searchResults.collectLatest { videos ->
                videoAdapter.submitList(videos)
            }
        }
    }
    
    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
