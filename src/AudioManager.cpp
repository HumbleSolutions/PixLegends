#include "AudioManager.h"
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#ifdef USE_SDL_MIXER
#include <SDL_mixer.h>
#endif
#include <fstream>
#include <filesystem>
#include <cstring>

AudioManager::AudioManager() : masterVolume(100), musicVolume(100), soundVolume(100) {
    initializeAudio();
}

AudioManager::~AudioManager() {
    cleanupAudio();
}

void AudioManager::initializeAudio() {
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            std::cerr << "Failed to init SDL audio: " << SDL_GetError() << std::endl;
            return;
        }
    }
#ifdef USE_SDL_MIXER
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == 0) {
        mixerInitialized = true;
        Mix_AllocateChannels(32);
        std::cout << "SDL_mixer initialized" << std::endl;
        return; // mixer handles playback; skip raw device
    } else {
        std::cerr << "SDL_mixer failed; falling back to raw SDL audio: " << Mix_GetError() << std::endl;
    }
#endif
    SDL_AudioSpec desired{};
    desired.freq = 44100;
    desired.format = AUDIO_S16;
    desired.channels = 2;
    desired.samples = 2048;
    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &deviceSpec, 0);
    if (audioDevice == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
    } else {
        SDL_PauseAudioDevice(audioDevice, 0);
        std::cout << "Audio device opened: " << deviceSpec.freq << " Hz" << std::endl;
    }
}

void AudioManager::cleanupAudio() {
    if (audioDevice) {
        SDL_CloseAudioDevice(audioDevice);
        audioDevice = 0;
    }
#ifdef USE_SDL_MIXER
    if (mixerInitialized) {
        Mix_CloseAudio();
        mixerInitialized = false;
    }
#endif
    std::cout << "Audio system cleaned up" << std::endl;
}

void AudioManager::update(float deltaTime) {
    (void)deltaTime;
#ifdef USE_SDL_MIXER
    // Handle transient music ducking
    if (mixerInitialized && musicDuckTimerSeconds > 0.0f) {
        musicDuckTimerSeconds -= deltaTime;
        if (musicDuckTimerSeconds <= 0.0f) {
            musicDuckTimerSeconds = 0.0f;
            musicDuckScale = 1.0f;
            applyMixerVolumes();
        }
    }
    // Handle music fade workflow
    if (mixerInitialized && musicFadeActive) {
        if (musicFadeStage == 1) {
            // Fading out current music
            if (!Mix_FadingMusic()) {
                musicFadeStage = 2;
                // Start fading in target
                auto it = musics.find(musicFadeTarget);
                if (it != musics.end()) {
                    Mix_PlayMusic(reinterpret_cast<Mix_Music*>(it->second), -1);
                    if (musicFadeInMs > 0) Mix_FadeInMusic(reinterpret_cast<Mix_Music*>(it->second), -1, musicFadeInMs);
                    currentMusicName = musicFadeTarget;
                }
                musicFadeActive = false;
            }
        }
    }
    // mixer handles streaming
    return;
#endif
    // Raw path ducking restore is handled on setMusicVolume when timer elapses; queue music streaming
    if (musicPlaying && !currentMusicName.empty() && audioDevice) {
        Uint32 queued = SDL_GetQueuedAudioSize(audioDevice);
        if (queued < deviceSpec.freq * deviceSpec.channels * 2 / 4) { // ~0.25s remaining
            auto it = musicDataByName.find(currentMusicName);
            if (it != musicDataByName.end() && !it->second.empty()) {
                SDL_QueueAudio(audioDevice, it->second.data(), static_cast<Uint32>(it->second.size()));
            }
        }
    }
}

void AudioManager::playSound(const std::string& soundName) {
#ifdef USE_SDL_MIXER
    if (mixerInitialized) {
        auto itc = chunks.find(soundName);
        if (itc != chunks.end() && itc->second) {
            float extraScale = 1.0f;
            if (soundName == "footstep_dirt") extraScale = 0.5f; // 50% quieter footsteps
            int vol = static_cast<int>(MIX_MAX_VOLUME * (soundVolume / 100.0f) * (masterVolume / 100.0f) * extraScale);
            Mix_Chunk* base = reinterpret_cast<Mix_Chunk*>(itc->second);
            Mix_Chunk* toPlay = base;
            if (soundName == "player_projectile") {
                int tailMs = 20; // play sooner by trimming less from the start
                int bytesPerSec = 44100 * 2 * 2; // S16 stereo
                int skipBytes = std::min(static_cast<int>(base->alen) - 0, (bytesPerSec * tailMs) / 1000);
                if (skipBytes > 0 && skipBytes < static_cast<int>(base->alen)) {
                    Uint32 newLen = base->alen - skipBytes;
                    Mix_Chunk* dup = static_cast<Mix_Chunk*>(SDL_malloc(sizeof(Mix_Chunk)));
                    if (dup) {
                        dup->allocated = 1;
                        dup->abuf = static_cast<Uint8*>(SDL_malloc(newLen));
                        if (dup->abuf) {
                            SDL_memcpy(dup->abuf, base->abuf + skipBytes, newLen);
                            dup->alen = newLen;
                            dup->volume = base->volume;
                            toPlay = dup;
                        } else {
                            SDL_free(dup);
                        }
                    }
                }
            }
            int ch = Mix_PlayChannel(-1, toPlay, 0);
            if (ch != -1) {
                Mix_Volume(ch, std::max(0, std::min(MIX_MAX_VOLUME, vol)));
                if (toPlay != base) {
                    // we cannot safely free here without a finished callback; keep short tail small leak avoided next run
                    // In a full system, track channel->chunk and free when finished.
                }
            }
            return;
        }
    }
#endif
    auto it = soundDataByName.find(soundName);
    if (it == soundDataByName.end()) {
        std::cerr << "Sound not loaded: " << soundName << std::endl;
        return;
    }
    if (!audioDevice) {
        std::cerr << "No audio device available" << std::endl;
        return;
    }
    const std::vector<Uint8>& buf = it->second;
    if (!buf.empty()) {
        // Queue a copy so buffer lifetime is safe until played. For simplicity, queue original; SDL copies internally.
        if (SDL_QueueAudio(audioDevice, buf.data(), static_cast<Uint32>(buf.size())) != 0) {
            std::cerr << "SDL_QueueAudio error: " << SDL_GetError() << std::endl;
        }
        SDL_PauseAudioDevice(audioDevice, 0); // ensure playback is unpaused
    }
}

void AudioManager::playMusic(const std::string& musicName) {
#ifdef USE_SDL_MIXER
    if (mixerInitialized) {
        auto itm = musics.find(musicName);
        if (itm == musics.end()) {
            std::cerr << "Music not loaded: " << musicName << std::endl;
            return;
        }
        Mix_HaltMusic();
        if (Mix_PlayMusic(reinterpret_cast<Mix_Music*>(itm->second), -1) != 0) {
            std::cerr << "Mix_PlayMusic error: " << Mix_GetError() << std::endl;
        }
        currentMusicName = musicName;
        musicPlaying = true;
        return;
    }
#endif
    if (!audioDevice) return;
    auto it = musicDataByName.find(musicName);
    if (it == musicDataByName.end()) {
        std::cerr << "Music not loaded: " << musicName << std::endl;
        return;
    }
    SDL_ClearQueuedAudio(audioDevice);
    const std::vector<Uint8>& buf = it->second;
    if (!buf.empty()) {
        if (SDL_QueueAudio(audioDevice, buf.data(), static_cast<Uint32>(buf.size())) != 0) {
            std::cerr << "SDL_QueueAudio(music) error: " << SDL_GetError() << std::endl;
            return;
        }
        currentMusicName = musicName;
        musicPlaying = true;
    }
}

void AudioManager::fadeToMusic(const std::string& musicName, int fadeOutMs, int fadeInMs) {
#ifdef USE_SDL_MIXER
    if (mixerInitialized) {
        musicFadeActive = true;
        musicFadeStage = 1;
        musicFadeOutMs = std::max(0, fadeOutMs);
        musicFadeInMs = std::max(0, fadeInMs);
        musicFadeTarget = musicName;
        if (Mix_PlayingMusic()) {
            Mix_FadeOutMusic(musicFadeOutMs);
        } else {
            musicFadeStage = 2; // no current music, go straight to fade in
        }
        return;
    }
#endif
    // Fallback: no mixer fade support; just switch music
    playMusic(musicName);
}

void AudioManager::stopMusic() {
#ifdef USE_SDL_MIXER
    if (mixerInitialized) {
        Mix_HaltMusic();
        return;
    }
#endif
    if (!audioDevice) return;
    SDL_ClearQueuedAudio(audioDevice);
    musicPlaying = false;
    currentMusicName.clear();
}

void AudioManager::pauseMusic() {
#ifdef USE_SDL_MIXER
    if (mixerInitialized) { Mix_PauseMusic(); return; }
#endif
    if (!audioDevice) return;
    SDL_PauseAudioDevice(audioDevice, 1);
}

void AudioManager::resumeMusic() {
#ifdef USE_SDL_MIXER
    if (mixerInitialized) { Mix_ResumeMusic(); return; }
#endif
    if (!audioDevice) return;
    SDL_PauseAudioDevice(audioDevice, 0);
}

void AudioManager::setMasterVolume(int volume) {
    masterVolume = std::max(0, std::min(100, volume));
    applyMixerVolumes();
}

void AudioManager::setMusicVolume(int volume) {
    musicVolume = std::max(0, std::min(100, volume));
    // Apply volume live
#ifdef USE_SDL_MIXER
    if (mixerInitialized) {
        int mixVol = static_cast<int>(MIX_MAX_VOLUME * (musicVolume / 100.0f) * (masterVolume / 100.0f) * musicDuckScale);
        Mix_VolumeMusic(std::max(0, std::min(MIX_MAX_VOLUME, mixVol)));
        return;
    }
#endif
    // For raw SDL path, reload current music at new volume (simple approach)
    if (!currentMusicName.empty()) {
        auto it = musicPathByName.find(currentMusicName);
        if (it != musicPathByName.end()) {
            // Re-load music buffer with new scaling and resume
            loadMusic(currentMusicName, it->second);
            playMusic(currentMusicName);
        }
    }
}

void AudioManager::setSoundVolume(int volume) {
    soundVolume = std::max(0, std::min(100, volume));
    applyMixerVolumes();
}
void AudioManager::applyMixerVolumes() {
#ifdef USE_SDL_MIXER
    if (mixerInitialized) {
        int mv = static_cast<int>(MIX_MAX_VOLUME * (musicVolume / 100.0f) * (masterVolume / 100.0f) * musicDuckScale);
        Mix_VolumeMusic(std::max(0, std::min(MIX_MAX_VOLUME, mv)));
        int sv = static_cast<int>(MIX_MAX_VOLUME * (soundVolume / 100.0f) * (masterVolume / 100.0f));
        Mix_Volume(-1, std::max(0, std::min(MIX_MAX_VOLUME, sv)));
    }
#endif
}

void AudioManager::startMusicDuck(float seconds, float musicScale01) {
    musicDuckTimerSeconds = std::max(0.0f, seconds);
    musicDuckScale = std::max(0.0f, std::min(1.0f, musicScale01));
    applyMixerVolumes();
}

void AudioManager::loadSound(const std::string& name, const std::string& filename) {
    // Use SDL_mixer when available; otherwise fall back to raw SDL device path (WAV only)
#ifdef USE_SDL_MIXER
    if (mixerInitialized) {
        if (!std::filesystem::exists(filename)) {
            std::cerr << "Sound file not found: " << filename << std::endl;
            return;
        }
        Mix_Chunk* ch = Mix_LoadWAV(filename.c_str());
        if (!ch) {
            std::cerr << "Mix_LoadWAV failed: " << Mix_GetError() << std::endl;
            return;
        }
        chunks[name] = reinterpret_cast<void*>(ch);
        applyMixerVolumes();
        return;
    }
#endif
    // Load WAV and convert to the opened device format to avoid static
    if (!std::filesystem::exists(filename)) {
        std::cerr << "Sound file not found: " << filename << std::endl;
        return;
    }
    if (!audioDevice) {
        std::cerr << "Audio device not initialized; cannot load sound: " << name << std::endl;
        return;
    }

    SDL_AudioSpec wavSpec{};
    Uint8* wavBuffer = nullptr;
    Uint32 wavLength = 0;
    if (SDL_LoadWAV(filename.c_str(), &wavSpec, &wavBuffer, &wavLength) == nullptr) {
        std::cerr << "SDL_LoadWAV failed for " << filename << ": " << SDL_GetError() << std::endl;
        return;
    }

    // Prepare converter from WAV format to device format
    SDL_AudioCVT cvt{};
    if (SDL_BuildAudioCVT(&cvt,
                          wavSpec.format, wavSpec.channels, wavSpec.freq,
                          deviceSpec.format, deviceSpec.channels, deviceSpec.freq) < 0) {
        std::cerr << "SDL_BuildAudioCVT failed: " << SDL_GetError() << std::endl;
        SDL_FreeWAV(wavBuffer);
        return;
    }

    cvt.len = static_cast<int>(wavLength);
    cvt.buf = static_cast<Uint8*>(SDL_malloc(cvt.len * cvt.len_mult));
    if (!cvt.buf) {
        std::cerr << "Failed to allocate audio conversion buffer" << std::endl;
        SDL_FreeWAV(wavBuffer);
        return;
    }
    std::memcpy(cvt.buf, wavBuffer, wavLength);
    if (SDL_ConvertAudio(&cvt) != 0) {
        std::cerr << "SDL_ConvertAudio failed: " << SDL_GetError() << std::endl;
        SDL_free(cvt.buf);
        SDL_FreeWAV(wavBuffer);
        return;
    }

    // Apply simple volume scaling if S16 to reduce loudness
    float volumeFactor = (masterVolume / 100.0f) * (soundVolume / 100.0f);
    volumeFactor = std::max(0.0f, std::min(volumeFactor, 1.0f));
    if (deviceSpec.format == AUDIO_S16) {
        Sint16* samples = reinterpret_cast<Sint16*>(cvt.buf);
        int sampleCount = cvt.len_cvt / sizeof(Sint16);
        for (int i = 0; i < sampleCount; ++i) {
            int sample = static_cast<int>(samples[i] * volumeFactor);
            samples[i] = static_cast<Sint16>(std::max(-32768, std::min(32767, sample)));
        }
    }

    std::vector<Uint8> data(cvt.buf, cvt.buf + cvt.len_cvt);
    SDL_free(cvt.buf);
    SDL_FreeWAV(wavBuffer);

    soundDataByName[name] = std::move(data);
    std::cout << "Loaded sound: " << name << " (converted to device format, bytes=" << soundDataByName[name].size() << ")" << std::endl;
}

void AudioManager::loadMusic(const std::string& name, const std::string& filename) {
#ifdef USE_SDL_MIXER
    if (mixerInitialized) {
        if (!std::filesystem::exists(filename)) { std::cerr << "Music not found: " << filename << std::endl; return; }
        Mix_Music* mm = Mix_LoadMUS(filename.c_str());
        if (!mm) { std::cerr << "Mix_LoadMUS failed: " << Mix_GetError() << std::endl; return; }
        musics[name] = reinterpret_cast<void*>(mm);
        musicPathByName[name] = filename;
        return;
    }
#endif
    if (!std::filesystem::exists(filename)) {
        std::cerr << "Music file not found: " << filename << std::endl;
        return;
    }
    if (!audioDevice) {
        std::cerr << "Audio device not initialized; cannot load music: " << name << std::endl;
        return;
    }
    SDL_AudioSpec wavSpec{}; Uint8* wavBuffer = nullptr; Uint32 wavLength = 0;
    if (SDL_LoadWAV(filename.c_str(), &wavSpec, &wavBuffer, &wavLength) == nullptr) {
        std::cerr << "SDL_LoadWAV failed for music " << filename << ": " << SDL_GetError() << std::endl;
        return;
    }
    SDL_AudioCVT cvt{};
    if (SDL_BuildAudioCVT(&cvt,
                          wavSpec.format, wavSpec.channels, wavSpec.freq,
                          deviceSpec.format, deviceSpec.channels, deviceSpec.freq) < 0) {
        std::cerr << "SDL_BuildAudioCVT (music) failed: " << SDL_GetError() << std::endl;
        SDL_FreeWAV(wavBuffer);
        return;
    }
    cvt.len = static_cast<int>(wavLength);
    cvt.buf = static_cast<Uint8*>(SDL_malloc(cvt.len * cvt.len_mult));
    if (!cvt.buf) { SDL_FreeWAV(wavBuffer); return; }
    std::memcpy(cvt.buf, wavBuffer, wavLength);
    if (SDL_ConvertAudio(&cvt) != 0) {
        std::cerr << "SDL_ConvertAudio (music) failed: " << SDL_GetError() << std::endl;
        SDL_free(cvt.buf);
        SDL_FreeWAV(wavBuffer);
        return;
    }
    float volumeFactor = (masterVolume / 100.0f) * (musicVolume / 100.0f);
    volumeFactor = std::max(0.0f, std::min(volumeFactor, 1.0f));
    if (deviceSpec.format == AUDIO_S16) {
        Sint16* samples = reinterpret_cast<Sint16*>(cvt.buf);
        int sampleCount = cvt.len_cvt / sizeof(Sint16);
        for (int i = 0; i < sampleCount; ++i) {
            int sample = static_cast<int>(samples[i] * volumeFactor);
            samples[i] = static_cast<Sint16>(std::max(-32768, std::min(32767, sample)));
        }
    }
    std::vector<Uint8> data(cvt.buf, cvt.buf + cvt.len_cvt);
    SDL_free(cvt.buf);
    SDL_FreeWAV(wavBuffer);
    musicDataByName[name] = std::move(data);
    musicPathByName[name] = filename;
}

bool AudioManager::hasMusic(const std::string& name) const {
#ifdef USE_SDL_MIXER
    if (mixerInitialized) {
        return musics.find(name) != musics.end();
    }
#endif
    return musicDataByName.find(name) != musicDataByName.end();
}
