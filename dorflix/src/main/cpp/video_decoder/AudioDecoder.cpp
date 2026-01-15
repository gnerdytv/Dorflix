#include "AudioDecoder.h"
#include <android/log.h>
#include <cstring>
#include <algorithm>

// Logging macros
#define LOG_TAG "AudioDecoder"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

AudioDecoder::AudioDecoder()
    : m_audioCodecContext(nullptr)
    , m_audioResampler(nullptr)
    , m_audioFrame(nullptr)
    , m_audioStreamIndex(-1)
    , m_audioEngineObject(nullptr)
    , m_audioEngine(nullptr)
    , m_audioOutputMixObject(nullptr)
    , m_audioPlayerObject(nullptr)
    , m_audioPlayerItf(nullptr)
    , m_audioBufferQueueItf(nullptr)
    , m_audioVolumeItf(nullptr)
    , m_audioInitialized(false)
    , m_volume(1.0f)
    , m_isMuted(false)
{
    LOGI("=== AUDIO DECODER CONSTRUCTOR ===");
}

AudioDecoder::~AudioDecoder() {
    LOGI("=== AUDIO DECODER DESTRUCTOR ===");
    cleanup();
}

bool AudioDecoder::initializeAudioCodec(AVFormatContext* formatContext) {
    LOGI("=== INITIALIZING AUDIO CODEC ===");

    if (!formatContext) {
        LOGE("❌ Format context is null");
        return false;
    }

    // Find audio stream
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        AVStream* stream = formatContext->streams[i];
        AVCodecParameters* codecParams = stream->codecpar;

        LOGI("Checking stream %d: codec_type=%d, codec_id=%d", i, codecParams->codec_type, codecParams->codec_id);

        if (codecParams->codec_type == AVMEDIA_TYPE_AUDIO) {
            LOGI("✅ Found audio stream at index %d", i);
            m_audioStreamIndex = i;

            // Find decoder
            const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
            if (!codec) {
                LOGE("❌ Failed to find decoder for codec_id=%d", codecParams->codec_id);
                return false;
            }
            LOGI("✅ Found audio decoder: %s", codec->name);

            // Allocate codec context
            m_audioCodecContext = avcodec_alloc_context3(codec);
            if (!m_audioCodecContext) {
                LOGE("❌ Failed to allocate audio codec context");
                return false;
            }
            LOGI("✅ Audio codec context allocated");

            // Copy codec parameters
            int result = avcodec_parameters_to_context(m_audioCodecContext, codecParams);
            if (result < 0) {
                LOGE("❌ Failed to copy audio codec parameters: %d", result);
                return false;
            }
            LOGI("✅ Audio codec parameters copied");

            // Open codec
            result = avcodec_open2(m_audioCodecContext, codec, nullptr);
            if (result < 0) {
                LOGE("❌ Failed to open audio codec: %d", result);
                return false;
            }
            LOGI("✅ Audio codec opened successfully");

            // Allocate audio frame
            m_audioFrame = av_frame_alloc();
            if (!m_audioFrame) {
                LOGE("❌ Failed to allocate audio frame");
                return false;
            }
            LOGI("✅ Audio frame allocated");

            LOGI("=== AUDIO CODEC INITIALIZED SUCCESSFULLY ===");
            return true;
        }
    }

    LOGW("⚠️ No audio stream found in %d streams", formatContext->nb_streams);
    m_audioStreamIndex = -1;
    return false;
}

bool AudioDecoder::initializeAudioOutput() {
    LOGI("=== INITIALIZING OPENSL ES AUDIO OUTPUT ===");

    if (m_audioInitialized) {
        LOGI("✅ Audio output already initialized");
        return true;
    }

    // Create audio engine
    SLresult result = slCreateEngine(&m_audioEngineObject, 0, nullptr, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("❌ Failed to create audio engine: %d", result);
        return false;
    }
    LOGI("✅ Audio engine object created");

    result = (*m_audioEngineObject)->Realize(m_audioEngineObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("❌ Failed to realize audio engine: %d", result);
        return false;
    }
    LOGI("✅ Audio engine realized");

    result = (*m_audioEngineObject)->GetInterface(m_audioEngineObject, SL_IID_ENGINE, &m_audioEngine);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("❌ Failed to get audio engine interface: %d", result);
        return false;
    }
    LOGI("✅ Audio engine interface obtained");

    // Create audio output mix
    result = (*m_audioEngine)->CreateOutputMix(m_audioEngine, &m_audioOutputMixObject, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("❌ Failed to create audio output mix: %d", result);
        return false;
    }
    LOGI("✅ Audio output mix object created");

    result = (*m_audioOutputMixObject)->Realize(m_audioOutputMixObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("❌ Failed to realize audio output mix: %d", result);
        return false;
    }
    LOGI("✅ Audio output mix realized");

    // Configure audio source
    SLDataLocator_AndroidSimpleBufferQueue bufferQueueLocator = {
        SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
        2  // Number of buffers
    };

    SLDataFormat_PCM pcmFormat = {
        SL_DATAFORMAT_PCM,
        2,  // Channels (stereo)
        SL_SAMPLINGRATE_44_1,  // Sample rate
        SL_PCMSAMPLEFORMAT_FIXED_16,  // 16-bit PCM
        SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,  // Stereo
        SL_BYTEORDER_LITTLEENDIAN
    };

    SLDataSource audioSrc = {&bufferQueueLocator, &pcmFormat};

    // Configure audio sink
    SLDataLocator_OutputMix outputMixLocator = {
        SL_DATALOCATOR_OUTPUTMIX,
        m_audioOutputMixObject
    };

    SLDataSink audioSnk = {&outputMixLocator, nullptr};

    // Create audio player
    const SLInterfaceID ids[2] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
    const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    result = (*m_audioEngine)->CreateAudioPlayer(m_audioEngine, &m_audioPlayerObject,
                                               &audioSrc, &audioSnk, 2, ids, req);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("❌ Failed to create audio player: %d", result);
        return false;
    }
    LOGI("✅ Audio player object created");

    result = (*m_audioPlayerObject)->Realize(m_audioPlayerObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("❌ Failed to realize audio player: %d", result);
        return false;
    }
    LOGI("✅ Audio player realized");

    // Get interfaces
    result = (*m_audioPlayerObject)->GetInterface(m_audioPlayerObject, SL_IID_PLAY, &m_audioPlayerItf);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("❌ Failed to get audio player interface: %d", result);
        return false;
    }
    LOGI("✅ Audio player interface obtained");

    result = (*m_audioPlayerObject)->GetInterface(m_audioPlayerObject, SL_IID_BUFFERQUEUE, &m_audioBufferQueueItf);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("❌ Failed to get buffer queue interface: %d", result);
        return false;
    }
    LOGI("✅ Audio buffer queue interface obtained");

    result = (*m_audioPlayerObject)->GetInterface(m_audioPlayerObject, SL_IID_VOLUME, &m_audioVolumeItf);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("❌ Failed to get volume interface: %d", result);
        return false;
    }
    LOGI("✅ Audio volume interface obtained");

    // Set buffer queue callback
    result = (*m_audioBufferQueueItf)->RegisterCallback(m_audioBufferQueueItf, audioBufferCallback, this);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("❌ Failed to register buffer callback: %d", result);
        return false;
    }
    LOGI("✅ Audio buffer callback registered");

    // Start audio playback
    result = (*m_audioPlayerItf)->SetPlayState(m_audioPlayerItf, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("❌ Failed to start audio playback: %d", result);
        return false;
    }
    LOGI("✅ Audio playback started");

    m_audioInitialized = true;
    LOGI("=== OPENSL ES AUDIO OUTPUT INITIALIZED SUCCESSFULLY ===");
    return true;
}

bool AudioDecoder::decodeAudioPacket(AVPacket* packet) {
    LOGI("=== DECODE AUDIO PACKET START ===");

    if (!packet) {
        LOGE("❌ AUDIO ERROR: Packet is null!");
        return false;
    }

    if (!hasAudioStream()) {
        LOGW("⚠️ No audio stream available, skipping packet");
        return false;
    }

    if (m_isMuted) {
        LOGI("🔇 Audio is muted, skipping packet");
        return true; // Not an error, just muted
    }

    // Initialize audio output if not already done
    if (!m_audioInitialized) {
        LOGI("🔄 Audio output not initialized, calling initializeAudioOutput()");
        if (!initializeAudioOutput()) {
            LOGE("❌ AUDIO ERROR: Failed to initialize audio output, audio will be muted");
            return false;
        }
        LOGI("✅ Audio output initialized successfully");
    }

    // Decode audio frame using modern FFmpeg 8+ API
    int result = avcodec_send_packet(m_audioCodecContext, packet);
    if (result < 0 && result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
        LOGE("❌ AUDIO ERROR: Failed to send audio packet: %d", result);
        return false;
    }

    while (result >= 0) {
        if (!m_audioFrame) {
            m_audioFrame = av_frame_alloc();
            if (!m_audioFrame) {
                LOGE("❌ AUDIO ERROR: Failed to allocate audio frame");
                return false;
            }
        }

        result = avcodec_receive_frame(m_audioCodecContext, m_audioFrame);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
            break;
        } else if (result < 0) {
            LOGE("❌ AUDIO ERROR: Failed to receive audio frame: %d", result);
            return false;
        }

        LOGI("✅ Decoded audio frame: channels=%d, sample_rate=%d, format=%d, nb_samples=%d",
             m_audioFrame->ch_layout.nb_channels, m_audioFrame->sample_rate,
             m_audioFrame->format, m_audioFrame->nb_samples);

        // Convert audio frame to S16 PCM if needed
        AVFrame* pcmFrame = m_audioFrame;
        bool needsConversion = (m_audioFrame->format != AV_SAMPLE_FMT_S16);

        if (needsConversion) {
            LOGI("🔄 Converting audio format %d to S16 PCM", m_audioFrame->format);

            // Initialize audio resampler if not already done
            if (!m_audioResampler) {
                // Use proper FFmpeg API for SwrContext configuration with channel layouts
                AVChannelLayout out_layout, in_layout;
                av_channel_layout_from_mask(&out_layout, AV_CH_LAYOUT_STEREO);
                av_channel_layout_from_mask(&in_layout, m_audioFrame->ch_layout.u.mask);

                int result = swr_alloc_set_opts2(&m_audioResampler,
                    &out_layout, AV_SAMPLE_FMT_S16, 44100,  // output: stereo, S16, 44.1kHz
                    &in_layout, (AVSampleFormat)m_audioFrame->format, m_audioFrame->sample_rate, // input
                    0, nullptr);

                if (!m_audioResampler) {
                    LOGE("❌ AUDIO ERROR: Failed to allocate audio resampler");
                    return false;
                }

                int resampleResult = swr_init(m_audioResampler);
                if (resampleResult < 0) {
                    LOGE("❌ AUDIO ERROR: Failed to initialize audio resampler: %d", resampleResult);
                    swr_free(&m_audioResampler);
                    m_audioResampler = nullptr;
                    return false;
                }
                LOGI("✅ Audio resampler initialized successfully (input: %d ch, %d Hz, fmt:%d -> output: stereo, 44.1kHz, S16)",
                     m_audioFrame->ch_layout.nb_channels, m_audioFrame->sample_rate, m_audioFrame->format);
            }

            // Allocate output frame
            pcmFrame = av_frame_alloc();
            if (!pcmFrame) {
                LOGE("❌ AUDIO ERROR: Failed to allocate PCM frame");
                return false;
            }

            // Set up output frame parameters (FFmpeg 8+ compliant)
            av_channel_layout_from_mask(&pcmFrame->ch_layout, AV_CH_LAYOUT_STEREO);
            pcmFrame->sample_rate = 44100;
            pcmFrame->format = AV_SAMPLE_FMT_S16;
            pcmFrame->nb_samples = swr_get_delay(m_audioResampler, m_audioFrame->sample_rate) + m_audioFrame->nb_samples;

            int bufferResult = av_frame_get_buffer(pcmFrame, 0);
            if (bufferResult < 0) {
                LOGE("❌ AUDIO ERROR: Failed to allocate PCM frame buffer: %d", bufferResult);
                av_frame_free(&pcmFrame);
                return false;
            }

            // Convert the audio
            int convertResult = swr_convert(m_audioResampler,
                                           pcmFrame->data, pcmFrame->nb_samples,
                                           (const uint8_t**)m_audioFrame->data, m_audioFrame->nb_samples);
            if (convertResult < 0) {
                LOGE("❌ AUDIO ERROR: Audio conversion failed: %d", convertResult);
                av_frame_free(&pcmFrame);
                return false;
            }

            pcmFrame->nb_samples = convertResult;
            LOGI("✅ Audio converted successfully: %d samples", pcmFrame->nb_samples);
        }

        // Apply volume scaling if needed
        if (m_volume < 1.0f && pcmFrame->format == AV_SAMPLE_FMT_S16) {
            int16_t* samples = (int16_t*)pcmFrame->data[0];
            int numSamples = pcmFrame->nb_samples * pcmFrame->ch_layout.nb_channels;

            for (int i = 0; i < numSamples; i++) {
                samples[i] = (int16_t)(samples[i] * m_volume);
            }
            LOGI("🔊 Applied volume scaling: %.2f", m_volume);
        }

        // Queue audio data for playback
        std::lock_guard<std::mutex> lock(m_audioBufferMutex);

        // Calculate buffer size for the PCM frame (always S16 stereo at 44.1kHz)
        int dataSize = av_samples_get_buffer_size(nullptr, pcmFrame->ch_layout.nb_channels,
                                                 pcmFrame->nb_samples, (AVSampleFormat)pcmFrame->format, 1);

        LOGI("Calculated PCM audio buffer size: %d bytes (%d channels, %d samples, format: %d)",
             dataSize, pcmFrame->ch_layout.nb_channels, pcmFrame->nb_samples, pcmFrame->format);

        if (dataSize > 0) {
            uint8_t* buffer = new uint8_t[dataSize];
            memcpy(buffer, pcmFrame->data[0], dataSize);

            m_audioBufferQueue.push(buffer);
            LOGI("✅ PCM Audio buffer queued, queue size: %zu", m_audioBufferQueue.size());

            // Enqueue to OpenSL ES buffer queue
            if (m_audioBufferQueueItf) {
                SLresult enqueueResult = (*m_audioBufferQueueItf)->Enqueue(m_audioBufferQueueItf, buffer, dataSize);
                if (enqueueResult == SL_RESULT_SUCCESS) {
                    LOGI("✅ PCM Audio buffer enqueued successfully");
                } else {
                    LOGE("❌ AUDIO ERROR: Failed to enqueue audio buffer (error: %d)", enqueueResult);
                    delete[] buffer;
                    m_audioBufferQueue.pop();
                    if (needsConversion && pcmFrame != m_audioFrame) {
                        av_frame_free(&pcmFrame);
                    }
                    return false;
                }
            } else {
                LOGE("❌ AUDIO ERROR: Buffer queue interface is null!");
                delete[] buffer;
                m_audioBufferQueue.pop();
                if (needsConversion && pcmFrame != m_audioFrame) {
                    av_frame_free(&pcmFrame);
                }
                return false;
            }
        } else {
            LOGW("⚠️ PCM Audio buffer size is 0 or negative, skipping frame");
        }

        // Free temporary PCM frame if it was allocated for conversion
        if (needsConversion && pcmFrame != m_audioFrame) {
            av_frame_free(&pcmFrame);
        }

        // Frame is ready, break to read next packet
        break;
    }

    LOGI("=== DECODE AUDIO PACKET COMPLETE ===\n");
    return true;
}

void AudioDecoder::setVolume(float volume) {
    m_volume = std::max(0.0f, std::min(1.0f, volume)); // Clamp to [0, 1]

    if (m_audioVolumeItf) {
        // Convert volume to millibels (0 = full volume, -∞ = silence)
        SLmillibel level = (m_volume > 0.0f) ?
            (SLmillibel)(2000.0f * log10f(m_volume)) : SL_MILLIBEL_MIN;

        SLresult result = (*m_audioVolumeItf)->SetVolumeLevel(m_audioVolumeItf, level);
        if (result == SL_RESULT_SUCCESS) {
            LOGI("🔊 Volume set to %.2f (millibels: %d)", m_volume, level);
        } else {
            LOGE("❌ Failed to set volume: %d", result);
        }
    } else {
        LOGW("⚠️ Volume interface not available");
    }
}

void AudioDecoder::setMute(bool mute) {
    m_isMuted = mute;

    if (m_audioVolumeItf) {
        SLmillibel level = mute ? SL_MILLIBEL_MIN : SL_MILLIBEL_MAX;
        SLresult result = (*m_audioVolumeItf)->SetVolumeLevel(m_audioVolumeItf, level);
        if (result == SL_RESULT_SUCCESS) {
            LOGI("🔇 Audio %s", mute ? "muted" : "unmuted");
        } else {
            LOGE("❌ Failed to %s audio: %d", mute ? "mute" : "unmute", result);
        }
    } else {
        LOGW("⚠️ Volume interface not available for mute control");
    }
}

void AudioDecoder::audioBufferCallback(SLBufferQueueItf caller, void* context) {
    AudioDecoder* decoder = static_cast<AudioDecoder*>(context);
    if (decoder) {
        decoder->onAudioBufferFinished();
    }
}

void AudioDecoder::onAudioBufferFinished() {
    LOGI("🔄 Audio buffer finished callback");

    std::lock_guard<std::mutex> lock(m_audioBufferMutex);

    if (!m_audioBufferQueue.empty()) {
        uint8_t* buffer = m_audioBufferQueue.front();
        m_audioBufferQueue.pop();
        delete[] buffer;
        LOGI("🗑️ Freed audio buffer, queue size now: %zu", m_audioBufferQueue.size());
    } else {
        LOGW("⚠️ Audio buffer finished but queue is empty");
    }
}

void AudioDecoder::cleanup() {
    LOGI("=== AUDIO DECODER CLEANUP START ===");

    // Stop audio playback
    if (m_audioPlayerItf) {
        SLresult result = (*m_audioPlayerItf)->SetPlayState(m_audioPlayerItf, SL_PLAYSTATE_STOPPED);
        if (result != SL_RESULT_SUCCESS) {
            LOGW("⚠️ Failed to stop audio playback during cleanup: %d", result);
        }
        m_audioPlayerItf = nullptr;
        LOGI("✅ Audio playback stopped");
    }

    // Clear audio buffer queue
    {
        std::lock_guard<std::mutex> lock(m_audioBufferMutex);
        while (!m_audioBufferQueue.empty()) {
            uint8_t* buffer = m_audioBufferQueue.front();
            m_audioBufferQueue.pop();
            delete[] buffer;
        }
        LOGI("✅ Audio buffer queue cleared");
    }

    // Destroy OpenSL ES objects
    if (m_audioPlayerObject) {
        (*m_audioPlayerObject)->Destroy(m_audioPlayerObject);
        m_audioPlayerObject = nullptr;
        LOGI("✅ Audio player destroyed");
    }

    if (m_audioOutputMixObject) {
        (*m_audioOutputMixObject)->Destroy(m_audioOutputMixObject);
        m_audioOutputMixObject = nullptr;
        LOGI("✅ Audio output mix destroyed");
    }

    if (m_audioEngineObject) {
        (*m_audioEngineObject)->Destroy(m_audioEngineObject);
        m_audioEngineObject = nullptr;
        LOGI("✅ Audio engine destroyed");
    }

    // Clean up FFmpeg resources
    if (m_audioFrame) {
        av_frame_free(&m_audioFrame);
        m_audioFrame = nullptr;
        LOGI("✅ Audio frame freed");
    }

    if (m_audioResampler) {
        swr_free(&m_audioResampler);
        m_audioResampler = nullptr;
        LOGI("✅ Audio resampler freed");
    }

    if (m_audioCodecContext) {
        avcodec_free_context(&m_audioCodecContext);
        m_audioCodecContext = nullptr;
        LOGI("✅ Audio codec context freed");
    }

    // Reset state
    m_audioStreamIndex = -1;
    m_audioInitialized = false;
    m_volume = 1.0f;
    m_isMuted = false;

    LOGI("=== AUDIO DECODER CLEANUP COMPLETE ===");
}