#pragma once

#include "Math.h"
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <optional>

namespace Riggle {

class Bone;
class Rig;

// Single keyframe for a bone's transform
struct BoneKeyframe {
    float time;           // Time in seconds
    Transform transform;  // Bone's local transform at this time
    
    BoneKeyframe(float t, const Transform& trans) : time(t), transform(trans) {}
};

// Animation track for a single bone
class BoneTrack {
public:
    BoneTrack(const std::string& boneName);
    
    // Keyframe management
    void addKeyframe(float time, const Transform& transform);
    void removeKeyframe(float time);
    void clearKeyframes();
    
    // Get interpolated transform at given time
    Transform getTransformAtTime(float time) const;
    
    // Getters
    const std::string& getBoneName() const { return m_boneName; }
    const std::vector<BoneKeyframe>& getKeyframes() const { return m_keyframes; }
    
    // Check if track has keyframes
    bool isEmpty() const { return m_keyframes.empty(); }
    float getDuration() const;

private:
    std::string m_boneName;
    std::vector<BoneKeyframe> m_keyframes; // Sorted by time
    
    // Helper methods
    void sortKeyframes();
    size_t findKeyframeIndex(float time) const;
    Transform interpolateTransforms(const Transform& a, const Transform& b, float t) const;
};

// Complete animation containing multiple bone tracks
class Animation {
public:
    Animation(const std::string& name);
    
    // Basic properties
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    float getDuration() const;
    bool isEmpty() const;
    
    // Track management
    BoneTrack* getBoneTrack(const std::string& boneName);
    BoneTrack* createBoneTrack(const std::string& boneName);
    void removeBoneTrack(const std::string& boneName);
    
    // Keyframe operations
    void addKeyframe(const std::string& boneName, float time, const Transform& transform);
    void removeKeyframe(const std::string& boneName, float time);
    
    // Animation playback
    void applyAtTime(class Rig* rig, float time) const;
    
    // Get all tracks
    const std::map<std::string, std::unique_ptr<BoneTrack>>& getTracks() const { return m_tracks; }

    std::optional<Transform> getLastKeyframeTransform(const std::string& boneName);

private:
    std::string m_name;
    std::map<std::string, std::unique_ptr<BoneTrack>> m_tracks;
};

// Animation player for controlling playback
class AnimationPlayer {
public:
    AnimationPlayer();
    
    // Animation control
    void setAnimation(Animation* animation);
    Animation* getAnimation() const { return m_animation; }
    
    // Playback control
    void play();
    void pause();
    void stop();
    void setTime(float time);
    
    // Update (call this every frame)
    void update(float deltaTime);
    
    // Apply current state to rig
    void applyToRig(class Rig* rig);
    
    // State queries
    bool isPlaying() const { return m_isPlaying; }
    float getCurrentTime() const { return m_currentTime; }
    bool isLooping() const { return m_isLooping; }
    void setLooping(bool loop) { m_isLooping = loop; }

private:
    Animation* m_animation;
    float m_currentTime;
    bool m_isPlaying;
    bool m_isLooping;
};

} // namespace Riggle