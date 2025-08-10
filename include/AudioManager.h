#pragma once

#include <SDL2/SDL.h>
#include <unordered_map>
#include <string>
#include <vector>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    // Core functions
    void update(float deltaTime);
    
    // Audio playback
    void playSound(const std::string& soundName);
    void playMusic(const std::string& musicName);
    void stopMusic();
    void pauseMusic();
    void resumeMusic();
    void fadeToMusic(const std::string& musicName, int fadeOutMs, int fadeInMs);
    
    // Volume control
    void setMasterVolume(int volume);
    void setMusicVolume(int volume);
    void setSoundVolume(int volume);

    // Getters for UI
    int getMasterVolume() const { return masterVolume; }
    int getMusicVolume() const { return musicVolume; }
    int getSoundVolume() const { return soundVolume; }
    
    // Audio loading
    void loadSound(const std::string& name, const std::string& filename);
    void loadMusic(const std::string& name, const std::string& filename);
    bool hasMusic(const std::string& name) const;
    // Temporarily duck music volume to let SFX stand out
    void startMusicDuck(float seconds, float musicScale01);

private:
    // Audio data storage
    std::unordered_map<std::string, std::vector<Uint8>> soundDataByName; // converted to deviceSpec (fallback without mixer)
    std::unordered_map<std::string, std::vector<Uint8>> musicDataByName; // converted to deviceSpec (fallback without mixer)
    std::unordered_map<std::string, std::string> musicPathByName; // original file paths by logical name
    std::string currentMusicName;
    bool musicPlaying = false;
    
    // Volume settings
    int masterVolume;
    int musicVolume;
    int soundVolume;
    
    // Helper functions
    void initializeAudio();
    void cleanupAudio();
    void applyMixerVolumes();

    // SDL Audio device/state
    SDL_AudioDeviceID audioDevice = 0;
    SDL_AudioSpec deviceSpec{};

#ifdef USE_SDL_MIXER
    bool mixerInitialized = false;
    std::unordered_map<std::string, void*> chunks; // SFX (Mix_Chunk*)
    std::unordered_map<std::string, void*> musics; // Music (Mix_Music*)
    // Temp chunk management for tail playback
    static AudioManager* mixerInstance;
    std::unordered_map<int, void*> tempChunksByChannel; // channel -> Mix_Chunk*
    void onChannelFinished(int channel);
    // Music fade management
    bool musicFadeActive = false;
    int musicFadeStage = 0; // 0 idle, 1 fading out started, 2 waiting to fade in
    int musicFadeOutMs = 0;
    int musicFadeInMs = 0;
    std::string musicFadeTarget;
#endif

    // Ducking state (applies to both mixer and raw paths)
    float musicDuckTimerSeconds = 0.0f;
    float musicDuckScale = 1.0f; // 1.0 = no duck, <1.0 = quieter music
    int previousMusicVolumeBeforeDuck = -1; // 0-100, for raw path restore
};
