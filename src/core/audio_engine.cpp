
#include <iostream>
#define MINIAUDIO_IMPLEMENTATION
#include "audio_engine.h"
#include "glm/ext/vector_float3.hpp"

bool AudioEngine::init() { return ma_engine_init(nullptr, &mEngine) == MA_SUCCESS; }

void AudioEngine::shutdown() {
    for (auto& [ptr, sound] : mPlayingSounds) {
        ma_sound_uninit(sound.get());
    }
    mPlayingSounds.clear();

    for (auto& [path, sound] : mSoundCache) {
        ma_sound_uninit(sound.get());
    }
    mSoundCache.clear();

    ma_engine_uninit(&mEngine);
}

void AudioEngine::updateListener(float px, float py, float pz, float fx, float fy, float fz) {
    ma_engine_listener_set_position(&mEngine, 0, px, py, pz);
    ma_engine_listener_set_direction(&mEngine, 0, fx, fy, fz);
}

void AudioEngine::preloadSound(const std::string& path) {
    if (mSoundCache.contains(path)) return;

    auto baseSound = std::make_unique<ma_sound>();
    auto res = ma_sound_init_from_file(&mEngine, path.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, baseSound.get());
    if (res != MA_SUCCESS) {
        std::cout << "Failed to preload " << path << " (" << res << ")\n";
        return;
    }
    mSoundCache[path] = std::move(baseSound);
}

void AudioEngine::playSound3D(const std::string& path, float x, float y, float z) {
    auto it = mSoundCache.find(path);
    if (it == mSoundCache.end()) {
        preloadSound(path);  // lazy-load on first use
        it = mSoundCache.find(path);
        if (it == mSoundCache.end()) return;  // load failed
    }

    auto instance = std::make_unique<ma_sound>();
    auto res = ma_sound_init_copy(&mEngine, it->second.get(), 0, NULL, instance.get());
    if (res != MA_SUCCESS) {
        std::cout << "Failed to spawn instance of " << path << " (" << res << ")\n";
        return;
    }

    ma_sound_set_position(instance.get(), x, y, z);
    ma_sound_start(instance.get());
    ma_sound_set_end_callback(instance.get(), onSoundEnd, this);

    mPlayingSounds[instance.get()] = std::move(instance);
}

ma_sound* AudioEngine::playLooping(const std::string& path, float x, float y, float z, bool spatial,
                                   float playbackSpeed) {
    auto it = mSoundCache.find(path);
    if (it == mSoundCache.end()) {
        preloadSound(path);
        it = mSoundCache.find(path);
        if (it == mSoundCache.end()) return nullptr;
    }

    auto instance = std::make_unique<ma_sound>();
    auto res = ma_sound_init_copy(&mEngine, it->second.get(), 0, NULL, instance.get());
    if (res != MA_SUCCESS) {
        std::cout << "Failed to spawn looping instance of " << path << " (" << res << ")\n";
        return nullptr;
    }

    ma_sound_set_looping(instance.get(), MA_TRUE);
    ma_sound_set_pitch(instance.get(), playbackSpeed);

    if (spatial) {
        ma_sound_set_position(instance.get(), x, y, z);
    } else {
        ma_sound_set_spatialization_enabled(instance.get(), MA_FALSE);  // ambient, no falloff/panning
    }

    ma_sound_start(instance.get());
    // NOTE: no onSoundEnd callback — looping sounds never naturally "end"

    ma_sound* key = instance.get();
    mPlayingSounds[key] = std::move(instance);
    return key;  // hand this back so caller can stop it later
}

ma_sound* AudioEngine::playInstance(ma_sound* master, float x, float y, float z, bool spatial, bool loop,
                                    float playbackSpeed) {
    auto instance = std::make_unique<ma_sound>();
    auto res = ma_sound_init_copy(&mEngine, master, 0, NULL, instance.get());
    if (res != MA_SUCCESS) {
        std::cout << "Failed to spawn sound instance (" << res << ")\n";
        return nullptr;
    }

    ma_sound_set_looping(instance.get(), loop ? MA_TRUE : MA_FALSE);
    ma_sound_set_pitch(instance.get(), playbackSpeed);

    if (spatial) {
        ma_sound_set_position(instance.get(), x, y, z);
    } else {
        ma_sound_set_spatialization_enabled(instance.get(), MA_FALSE);
    }

    if (!loop) {
        // Only one-shots need the end callback — looping sounds never naturally end
        ma_sound_set_end_callback(instance.get(), onSoundEnd, this);
    }

    ma_sound_start(instance.get());

    ma_sound* key = instance.get();
    mPlayingSounds[key] = std::move(instance);
    return key;
}

void AudioEngine::setSoundPosition(ma_sound* handle, float x, float y, float z) {
    auto it = mPlayingSounds.find(handle);
    if (it == mPlayingSounds.end()) return;  // stale handle, sound already stopped/removed
    ma_sound_set_position(it->second.get(), x, y, z);
}
ma_engine* AudioEngine::getEngineHandle() { return &mEngine; }

void AudioEngine::stopSound(ma_sound* handle) {
    auto it = mPlayingSounds.find(handle);
    if (it == mPlayingSounds.end()) return;
    ma_sound_uninit(it->second.get());
    mPlayingSounds.erase(it);
}

void AudioEngine::garbageCollect() {
    // for (auto* sound : mFinished) {
    //     ma_sound_uninit(sound);
    //     mSounds.erase(sound);
    // }
    // mFinished.clear();
}

SoundHandle::SoundHandle() = default;

SoundHandle::SoundHandle(AudioEngine* engine, ma_sound* sound) : mAudioEngine(engine), mSound(sound) {}

void SoundHandle::stop() { mAudioEngine->stopSound(mSound); }

void SoundHandle::setPosition(const glm::vec3& position) {
    mAudioEngine->setSoundPosition(mSound, position.x, position.y, position.z);
}

void SoundHandle::setPitch(float pitch) {}

SoundClip::SoundClip(AudioEngine* engine, const std::filesystem::path& path) : mSoundEngine(engine) {
    mSound = std::make_unique<ma_sound>();
    ma_result res =
        ma_sound_init_from_file(mSoundEngine->getEngineHandle(),  // see note below — needs a raw ma_engine* accessor
                                path.string().c_str(),
                                MA_SOUND_FLAG_DECODE,  // decode fully into memory — good for SFX-sized clips
                                nullptr, nullptr, mSound.get());

    if (res != MA_SUCCESS) {
        std::cout << "Failed to load SoundClip: " << path << " (" << res << ")\n";
        mSound.reset();
    }
}

SoundClip::~SoundClip() {
    if (mSound) {
        ma_sound_uninit(mSound.get());
    }
}

SoundHandle SoundClip::playSound3D(const glm::vec3& position, float playbackSpeed, bool loop) {
    SoundHandle handle{};
    if (!mSound || !mSoundEngine) return handle;
    ma_sound* raw_instance_handle =
        mSoundEngine->playInstance(mSound.get(), position.x, position.y, position.z, true, loop, playbackSpeed);
    return {mSoundEngine, raw_instance_handle};
}

SoundHandle SoundClip::playSoundAmbient(float playbackSpeed, bool loop) {
    SoundHandle handle{};
    if (!mSound || !mSoundEngine) return handle;
    ma_sound* raw_instance_handle = mSoundEngine->playInstance(mSound.get(), 0, 0, 0, false, loop, playbackSpeed);
    return {mSoundEngine, raw_instance_handle};
}

void SoundClip::stop() {}
