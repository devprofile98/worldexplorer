
#ifndef WORLD_EXPLORER_CORE_AUDIO_ENGINE
#define WORLD_EXPLORER_CORE_AUDIO_ENGINE

#include <memory>
#include <unordered_map>
#include <vector>

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
        void setSoundPosition(ma_sound* handle, float x, float y, float z);

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

#endif  //! WORLD_EXPLORER_CORE_AUDIO_ENGINE
