#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/frame.h>
}

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    // Initialization
    bool initializeAudioCodec(AVFormatContext* formatContext);
    bool initializeAudioOutput();

    // Playback control
    bool decodeAudioPacket(AVPacket* packet);
    void setVolume(float volume);
    void setMute(bool mute);
    float getVolume() const { return m_volume; }
    bool isMuted() const { return m_isMuted; }

    // State management
    void cleanup();

    // Audio stream info
    bool hasAudioStream() const { return m_audioStreamIndex >= 0; }
    int getAudioStreamIndex() const { return m_audioStreamIndex; }

private:
    // FFmpeg audio components
    AVCodecContext* m_audioCodecContext;
    SwrContext* m_audioResampler;
    AVFrame* m_audioFrame;

    // Audio stream info
    int m_audioStreamIndex;

    // OpenSL ES components
    SLObjectItf m_audioEngineObject;
    SLEngineItf m_audioEngine;
    SLObjectItf m_audioOutputMixObject;
    SLObjectItf m_audioPlayerObject;
    SLPlayItf m_audioPlayerItf;
    SLBufferQueueItf m_audioBufferQueueItf;
    SLVolumeItf m_audioVolumeItf;

    // Audio buffer management
    std::queue<uint8_t*> m_audioBufferQueue;
    std::mutex m_audioBufferMutex;
    std::atomic<bool> m_audioInitialized;

    // Audio settings
    float m_volume;
    bool m_isMuted;

    // Static callback
    static void audioBufferCallback(SLBufferQueueItf caller, void* context);
    void onAudioBufferFinished();
};

#endif // AUDIO_DECODER_H