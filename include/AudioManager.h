#pragma once

#include <SDL2/SDL.h>
#include <unordered_map>
#include <string>

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
    
    // Volume control
    void setMasterVolume(int volume);
    void setMusicVolume(int volume);
    void setSoundVolume(int volume);
    
    // Audio loading
    void loadSound(const std::string& name, const std::string& filename);
    void loadMusic(const std::string& name, const std::string& filename);

private:
    // Audio data storage
    std::unordered_map<std::string, void*> sounds; // Mix_Chunk*
    std::unordered_map<std::string, std::string> musicFiles;
    
    // Volume settings
    int masterVolume;
    int musicVolume;
    int soundVolume;
    
    // Helper functions
    void initializeAudio();
    void cleanupAudio();
};
