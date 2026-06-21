
#ifndef WORLD_EXPLORER_CORE_AUDIO_ENGINE
#define WORLD_EXPLORER_CORE_AUDIO_ENGINE

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

#include "glm/fwd.hpp"
#include "miniaudio.h"

class AudioEngine {
    public:
        bool init();
        void shutdown();
        void updateListener(float px, float py, float pz, float fx, float fy, float fz);
        void playSound3D(const std::string& path, float x, float y, float z);
        void preloadSound(const std::string& path);

        void stopSound(ma_sound* handle);
        ma_sound* playLooping(const std::string& path, float x, float y, float z, bool spatial = true,
                              float playbackSpeed = 1.0);
        ma_sound* playInstance(ma_sound* master, float x, float y, float z, bool spatial, bool loop,
                               float playbackSpeed);
        void setSoundPosition(ma_sound* handle, float x, float y, float z);
        ma_engine* getEngineHandle();

    private:
        static inline void onSoundEnd(void* userData, ma_sound* sound) {
            auto* self = static_cast<AudioEngine*>(userData);
            self->mFinished.push_back(sound);
        }
        void garbageCollect();

        ma_engine mEngine;
        std::unordered_map<std::string, std::unique_ptr<ma_sound>> mSoundCache;   // loaded data, never played directly
        std::unordered_map<ma_sound*, std::unique_ptr<ma_sound>> mPlayingSounds;  // active instances
        // std::unordered_map<ma_sound*, std::unique_ptr<ma_sound>> mSounds;
        std::vector<ma_sound*> mFinished;
};

class SoundHandle {
    public:
        SoundHandle();
        SoundHandle(AudioEngine* engine, ma_sound* sound);
        void stop();
        void setPosition(const glm::vec3& position);
        void setPitch(float pitch);

    private:
        AudioEngine* mAudioEngine = nullptr;
        ma_sound* mSound = nullptr;
};

class SoundClip {
    public:
        SoundClip() {};
        SoundClip(AudioEngine* engine, const std::filesystem::path& path);
        ~SoundClip();
        SoundHandle playSound3D(const glm::vec3& position, float playbackSpeed = 1.0, bool loop = false);
        SoundHandle playSoundAmbient(float playbackSpeed = 1.0, bool loop = false);
        void stop();

        SoundClip(const SoundClip&) = delete;
        SoundClip& operator=(const SoundClip&) = delete;

        SoundClip(SoundClip&&) = default;
        SoundClip& operator=(SoundClip&&) = default;

    private:
        std::unique_ptr<ma_sound> mSound;
        AudioEngine* mSoundEngine = nullptr;
};

#endif  //! WORLD_EXPLORER_CORE_AUDIO_ENGINE
