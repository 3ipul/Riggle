#include "Riggle/Animation.h"
#include "Riggle/Rig.h"
#include "Riggle/Bone.h"
#include <algorithm>
#include <cmath>

namespace Riggle {

// BoneTrack Implementation
BoneTrack::BoneTrack(const std::string& boneName) : m_boneName(boneName) {}

void BoneTrack::addKeyframe(float time, const Transform& transform) {
    // Remove existing keyframe at same time
    removeKeyframe(time);
    
    // Add new keyframe
    m_keyframes.emplace_back(time, transform);
    sortKeyframes();
}

void BoneTrack::removeKeyframe(float time) {
    const float tolerance = 0.001f;
    m_keyframes.erase(
        std::remove_if(m_keyframes.begin(), m_keyframes.end(),
            [time, tolerance](const BoneKeyframe& kf) {
                return std::abs(kf.time - time) < tolerance;
            }),
        m_keyframes.end()
    );
}

void BoneTrack::clearKeyframes() {
    m_keyframes.clear();
}

Transform BoneTrack::getTransformAtTime(float time) const {
    if (m_keyframes.empty()) {
        return Transform(); // Default transform
    }
    
    if (m_keyframes.size() == 1) {
        return m_keyframes[0].transform;
    }
    
    // Clamp time to animation bounds
    if (time <= m_keyframes.front().time) {
        return m_keyframes.front().transform;
    }
    if (time >= m_keyframes.back().time) {
        return m_keyframes.back().transform;
    }
    
    // Find surrounding keyframes
    size_t index = findKeyframeIndex(time);
    if (index == 0) {
        return m_keyframes[0].transform;
    }
    
    const BoneKeyframe& prev = m_keyframes[index - 1];
    const BoneKeyframe& next = m_keyframes[index];
    
    // Calculate interpolation factor
    float t = (time - prev.time) / (next.time - prev.time);
    
    return interpolateTransforms(prev.transform, next.transform, t);
}

float BoneTrack::getDuration() const {
    if (m_keyframes.empty()) return 0.0f;
    return m_keyframes.back().time;
}

void BoneTrack::sortKeyframes() {
    std::sort(m_keyframes.begin(), m_keyframes.end(),
        [](const BoneKeyframe& a, const BoneKeyframe& b) {
            return a.time < b.time;
        });
}

size_t BoneTrack::findKeyframeIndex(float time) const {
    for (size_t i = 0; i < m_keyframes.size(); ++i) {
        if (m_keyframes[i].time > time) {
            return i;
        }
    }
    return m_keyframes.size();
}

Transform BoneTrack::interpolateTransforms(const Transform& a, const Transform& b, float t) const {
    Transform result;
    
    // Linear interpolation for position and scale
    result.x = a.x + (b.x - a.x) * t;
    result.y = a.y + (b.y - a.y) * t;
    result.scaleX = a.scaleX + (b.scaleX - a.scaleX) * t;
    result.scaleY = a.scaleY + (b.scaleY - a.scaleY) * t;
    result.length = a.length + (b.length - a.length) * t;
    
    // Shortest path rotation interpolation
    float angleDiff = b.rotation - a.rotation;
    const float PI = 3.14159f;
    
    // Normalize angle difference to [-π, π]
    while (angleDiff > PI) angleDiff -= 2.0f * PI;
    while (angleDiff < -PI) angleDiff += 2.0f * PI;
    
    result.rotation = a.rotation + angleDiff * t;
    
    return result;
}

// Animation Implementation
Animation::Animation(const std::string& name) : m_name(name) {}

float Animation::getDuration() const {
    float maxDuration = 0.0f;
    for (const auto& pair : m_tracks) {
        maxDuration = std::max(maxDuration, pair.second->getDuration());
    }
    return maxDuration;
}

bool Animation::isEmpty() const {
    return m_tracks.empty();
}

BoneTrack* Animation::getBoneTrack(const std::string& boneName) {
    auto it = m_tracks.find(boneName);
    return (it != m_tracks.end()) ? it->second.get() : nullptr;
}

BoneTrack* Animation::createBoneTrack(const std::string& boneName) {
    auto track = std::make_unique<BoneTrack>(boneName);
    BoneTrack* result = track.get();
    m_tracks[boneName] = std::move(track);
    return result;
}

void Animation::removeBoneTrack(const std::string& boneName) {
    m_tracks.erase(boneName);
}

void Animation::addKeyframe(const std::string& boneName, float time, const Transform& transform) {
    BoneTrack* track = getBoneTrack(boneName);
    if (!track) {
        track = createBoneTrack(boneName);
    }
    track->addKeyframe(time, transform);
}

void Animation::removeKeyframe(const std::string& boneName, float time) {
    BoneTrack* track = getBoneTrack(boneName);
    if (track) {
        track->removeKeyframe(time);
        
        // Remove track if empty
        if (track->isEmpty()) {
            removeBoneTrack(boneName);
        }
    }
}

void Animation::applyAtTime(Rig* rig, float time) const {
    if (!rig) return;
    
    // Apply each track to its corresponding bone
    for (const auto& pair : m_tracks) {
        const std::string& boneName = pair.first;
        const BoneTrack* track = pair.second.get();
        
        auto bone = rig->findBone(boneName);
        if (bone) {
            Transform transform = track->getTransformAtTime(time);
            bone->setLocalTransform(transform);
        }
    }
    
    // Force update all world transforms
    rig->forceUpdateWorldTransforms();
}

// AnimationPlayer Implementation
AnimationPlayer::AnimationPlayer() 
    : m_animation(nullptr)
    , m_currentTime(0.0f)
    , m_isPlaying(false)
    , m_isLooping(true)
{}

void AnimationPlayer::setAnimation(Animation* animation) {
    m_animation = animation;
    m_currentTime = 0.0f;
}

void AnimationPlayer::play() {
    m_isPlaying = true;
}

void AnimationPlayer::pause() {
    m_isPlaying = false;
}

void AnimationPlayer::stop() {
    m_isPlaying = false;
    m_currentTime = 0.0f;
}

void AnimationPlayer::setTime(float time) {
    if (!m_animation) return;
    
    float duration = m_animation->getDuration();
    if (duration > 0.0f) {
        if (m_isLooping) {
            // Wrap time within animation duration
            m_currentTime = std::fmod(time, duration);
            if (m_currentTime < 0.0f) {
                m_currentTime += duration;
            }
        } else {
            // Clamp time to animation bounds
            m_currentTime = std::max(0.0f, std::min(time, duration));
            
            // Stop at end if not looping
            if (m_currentTime >= duration) {
                m_isPlaying = false;
            }
        }
    } else {
        m_currentTime = 0.0f;
    }
}

void AnimationPlayer::update(float deltaTime) {
    if (!m_isPlaying || !m_animation) return;
    
    setTime(m_currentTime + deltaTime);
}

void AnimationPlayer::applyToRig(Rig* rig) {
    if (m_animation && rig) {
        m_animation->applyAtTime(rig, m_currentTime);
    }
}

std::optional<Transform> Animation::getLastKeyframeTransform(const std::string& boneName) {
    auto* track = getBoneTrack(boneName);
    if (!track || track->getKeyframes().empty()) {
        return std::nullopt;
    }
    
    const auto& keyframes = track->getKeyframes();
    return keyframes.back().transform; // Return last keyframe
}

} // namespace Riggle