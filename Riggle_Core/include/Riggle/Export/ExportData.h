#pragma once
#include "../Math.h"
#include <string>
#include <vector>

namespace Riggle {

// Pure data structures for export
struct ExportBone {
    std::string name;
    std::string parentName;
    Transform transform;        // Local transform
    Transform worldTransform;   // World transform
    float length;
    std::vector<std::string> childNames;
    
    ExportBone() : length(0.0f) {}
};

struct ExportSprite {
    std::string name;
    std::string texturePath;
    Transform transform;
    bool isVisible;
    std::string boundBoneName; // Empty if unbound
    Vector2 bindOffset;
    float bindRotation;
    
    ExportSprite() : isVisible(true), bindRotation(0.0f) {}
};

struct ExportKeyframe {
    float time;
    Transform transform;
    
    ExportKeyframe() : time(0.0f) {}
    ExportKeyframe(float t, const Transform& tr) : time(t), transform(tr) {}
};

struct ExportBoneTrack {
    std::string boneName;
    std::vector<ExportKeyframe> keyframes;
};

struct ExportAnimation {
    std::string name;
    float duration;
    std::vector<ExportBoneTrack> tracks;
    
    ExportAnimation() : duration(0.0f) {}
};

struct ExportProject {
    std::string name;
    std::vector<ExportBone> bones;
    std::vector<ExportSprite> sprites;
    std::vector<ExportAnimation> animations;
    std::string version;
    
    ExportProject() : version("1.0") {}
};

}