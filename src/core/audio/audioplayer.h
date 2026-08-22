#pragma once

#include <miniaudio/miniaudio.h>

class CAudioPlayerState;
extern CAudioPlayerState g_audioPlayer;

class CAudioPlayerState
{
public:

    CAudioPlayerState() : isPlaying(false), audioCursor(0), deviceInitialised(false)
    {
    }

    bool IsInitialised() const { return deviceInitialised; };

    // i cannot figure out how to do this properly using other miniaudio interfaces, so i'm going to leave it with the really bad
    // method of teardown + reinit of the device every time a new sound is played. i will come back to this eventually
    void Setup(const std::vector<char>& audioData, size_t numSamples, uint32_t sampleRate, uint8_t numChannels, uint8_t sampleSize)
    {
        Shutdown();

        audioCursor = 0;

        _audioData = audioData;
        _numSamples = numSamples;
        _sampleRate = sampleRate;
        _numChannels = numChannels;
        _sampleSize = sampleSize;

        isPlaying = true;

        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format   = sampleSize == sizeof(float) ? ma_format_f32 : ma_format_s16;
        config.playback.channels = _numChannels;
        config.sampleRate        = _sampleRate;
        config.dataCallback      = MiniAudioDataCallback;
        config.pUserData         = this;

        if (ma_device_init(nullptr, &config, &audioDevice) != MA_SUCCESS)
        {
            deviceInitialised = false;
            return;
        }

        deviceInitialised = true;
        ma_device_start(&audioDevice);
    }

    void Shutdown()
    {
        if (deviceInitialised)
        {
            ma_device_stop(&audioDevice);
            ma_device_uninit(&audioDevice);

            deviceInitialised = false;
        }
        _audioData.clear();

        isPlaying = false;
    }

    void TogglePause() { isPlaying = !isPlaying; }
    void Play() { isPlaying = true; }
    void Pause() { isPlaying = false; }
    void Restart()
    {
        audioCursor = 0;
        Play();
    }

    void SeekToFrame(size_t frame)
    {
        // can't use the numsamples/numchannels vars until device has been initialised
        if (!deviceInitialised)
            return;

        // clamp target frame so we dont end up after the end of the audio stream!
        audioCursor = std::min(frame * _numChannels * _sampleSize, _audioData.size());
    }

    bool IsAudioFinished() const { return _audioData.empty() || audioCursor >= _audioData.size(); };

    bool IsPlaying() const { return isPlaying; }
    size_t GetCursor() const { return audioCursor; };
    float GetCursorAsTime() const { return !deviceInitialised ? 0 : (audioCursor / _numChannels / (float)_sampleSize) / (float)_sampleRate; };
    float GetSoundDurationAsTime() const { return !deviceInitialised ? 0 : _numSamples / (float)_sampleRate; };

private:
    static void MiniAudioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
    {
        assert(pDevice);

        UNUSED(pInput);

        CAudioPlayerState* state = reinterpret_cast<CAudioPlayerState*>(pDevice->pUserData);

        // fire the callback at a non-static method so i dont have to use g_audioPlayer everywhere (it's a bit ugly)
        if (state)
            state->DataCallback(pOutput, frameCount);
    }

    void DataCallback(void* pOutput, ma_uint32 frameCount)
    {
        if (!isPlaying || _audioData.empty())
            return;

        // make sure that we never end up with a negative number here
        const size_t numRemainingBytes = (audioCursor < _audioData.size()) ? (_audioData.size() - audioCursor) : 0;

        const size_t frameSizeToConsume = std::min(static_cast<size_t>(frameCount) * _numChannels * _sampleSize, numRemainingBytes);

        memcpy_s(pOutput, frameSizeToConsume, _audioData.data() + audioCursor, frameSizeToConsume);

        audioCursor += frameSizeToConsume;

        // if we've reached the end, stop playback (keeps device running but silence on subsequent callbacks)
        if (audioCursor >= _audioData.size())
            isPlaying = false;
    }

private:
    // [rexx]: members beginning with _ have come from the currently playing sound

    std::vector<char> _audioData;
    std::atomic<size_t> audioCursor;
    std::atomic<bool> isPlaying;

    size_t _numSamples;
    uint32_t _sampleRate;
    uint8_t _numChannels;
    uint8_t _sampleSize;

    ma_device audioDevice;
    bool deviceInitialised;
    std::mutex m_lock;
};

