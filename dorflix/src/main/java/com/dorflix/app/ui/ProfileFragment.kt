package com.dorflix.app.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.lifecycle.lifecycleScope
import com.dorflix.app.R
import com.dorflix.app.databinding.FragmentFeedBinding
import com.dorflix.app.presentation.viewmodel.VideoViewModel
import com.dorflix.app.domain.model.VideoStatistics
import com.dorflix.app.domain.model.SocialActivityResponse
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

/**
 * Fragment for displaying user profile and statistics
 */
class ProfileFragment : Fragment() {
    
    private var _binding: FragmentFeedBinding? = null
    private val binding get() = _binding!!
    
    private val viewModel: VideoViewModel by lazy { VideoViewModel() }
    
    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentFeedBinding.inflate(inflater, container, false)
        return binding.root
    }
    
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        
        setupObservers()
        
        // Load user statistics and social activity
        loadProfileData()
    }
    
    private fun loadProfileData() {
        // Load watch statistics
        viewModel.loadWatchStatistics()
        
        // Load social activity
        viewModel.getSocialActivity()
    }
    
    private fun setupObservers() {
        viewLifecycleOwner.lifecycleScope.launch {
            viewModel.watchStatistics.collectLatest { statistics ->
                // Update UI with statistics
                updateStatisticsUI(statistics)
            }
        }
        
        viewLifecycleOwner.lifecycleScope.launch {
            viewModel.socialActivity.collectLatest { activity ->
                // Update UI with social activity
                updateSocialActivityUI(activity)
            }
        }
    }
    
    private fun updateStatisticsUI(statistics: VideoStatistics?) {
        statistics?.let {
            // Update statistics UI elements
            // This would typically update TextViews with the statistics data
        }
    }
    
    private fun updateSocialActivityUI(activity: SocialActivityResponse?) {
        activity?.let {
            // Update social activity UI elements
            // This would typically update a list or grid with recent activity
        }
    }
    
    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
