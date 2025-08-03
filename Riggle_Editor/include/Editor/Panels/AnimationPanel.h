#pragma once

#include "BasePanel.h"
#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

namespace Riggle {
    class Character;
    class Animation;
    class Bone;
}

namespace Riggle {

struct TimelineState {
    float currentTime = 0.0f;
    float zoom = 1.0f;
    float scrollX = 0.0f;
    bool isPlaying = false;
    bool isRecording = false;
    float playbackSpeed = 1.0f;
    bool autoKeyframe = true;
    
    // Selection
    std::string selectedBone;
    float selectedKeyframeTime = -1.0f;
    
    // Timeline metrics
    float pixelsPerSecond = 60.0f;
    float timelineHeight = 300.0f;
    float trackHeight = 25.0f;
    float headerWidth = 150.0f;
};

class AnimationPanel : public BasePanel {
public:
    AnimationPanel();
    ~AnimationPanel() = default;
    
    // BasePanel overrides
    void render() override;
    void update(sf::RenderWindow& window) override;
    
    // Animation-specific methods
    void setCharacter(Character* character) { m_character = character; }
    Character* getCharacter() const { return m_character; }
    
    // State management
    void setCurrentTime(float time);
    float getCurrentTime() const { return m_state.currentTime; }
    void applyLastKeyframeToAllBones(Animation* animation);
    
    void setRecording(bool recording) { m_state.isRecording = recording; }
    bool isRecording() const { return m_state.isRecording; }
    bool isAutoKeyEnabled() const { return m_state.autoKeyframe; }
    void createKeyframeForBone(const std::string& boneName);
    
private:
    Character* m_character;
    TimelineState m_state;
    
    // UI Components
    void renderAnimationControls();
    void renderTimeline();
    void renderTimelineHeader(ImVec2 timelineStart);
    void renderTimelineRuler(float duration, ImVec2 timelineStart);
    void renderBoneTracks(Animation* animation);
    void renderBoneTrack(const std::string& boneName, Animation* animation, int trackIndex);
    void renderKeyframes(const std::string& boneName, Animation* animation, float trackY);
    void renderPlayhead(float duration, ImVec2 timelineStart);
    
    // Interaction
    void handleTimelineInteraction(float duration);
    void handleKeyframeInteraction(const std::string& boneName, Animation* animation);
    
    // Keyframe operations
    void addKeyframeAtCurrentTime(const std::string& boneName, Animation* animation);
    void deleteSelectedKeyframe(Animation* animation);
    void moveKeyframe(const std::string& boneName, Animation* animation, float oldTime, float newTime);
    
    // Utility
    float timeToPixel(float time) const;
    float pixelToTime(float pixel) const;
    ImVec2 getTimelineContentStart() const;
    ImVec2 getTimelineContentSize() const;
    bool isMouseInTimeline() const;
    
    // Animation management
    void createNewAnimation();
    void deleteAnimation(const std::string& name);
    
    // Playback
    void updatePlayback(float deltaTime);

    void toggleRecording();
    void keyAllBones();
};

} // namespace Riggle