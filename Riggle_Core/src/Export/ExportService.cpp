#include "Riggle/Export/ExportService.h"
#include <algorithm>

namespace Riggle {

ExportProject ExportService::extractProjectData(const Character& character, const std::string& projectName) {
    ExportProject project;
    project.name = projectName;
    project.version = "1.0";
    
    // Extract rig data
    if (character.getRig()) {
        project.bones = extractBoneData(*character.getRig());
    }
    
    // Extract sprite data
    project.sprites = extractSpriteData(character.getSprites());
    
    // Extract animation data
    const auto& animations = character.getAnimations();
    project.animations.reserve(animations.size());
    
    for (const auto& animation : animations) {
        if (animation) {
            project.animations.push_back(extractAnimationData(*animation));
        }
    }
    
    return project;
}

ExportAnimation ExportService::extractAnimationData(const Animation& animation) {
    ExportAnimation exportAnim;
    exportAnim.name = animation.getName();
    exportAnim.duration = animation.getDuration();
    
    // Convert all bone tracks
    const auto& tracks = animation.getTracks();
    exportAnim.tracks.reserve(tracks.size());
    
    for (const auto& [boneName, track] : tracks) {
        if (track && !track->isEmpty()) {
            exportAnim.tracks.push_back(convertBoneTrack(*track));
        }
    }
    
    return exportAnim;
}

std::vector<ExportBone> ExportService::extractBoneData(const Rig& rig) {
    std::vector<ExportBone> bones;
    
    // Get all root bones and traverse their hierarchies
    const auto& rootBones = rig.getRootBones();
    for (const auto& rootBone : rootBones) {
        if (rootBone) {
            collectBoneHierarchy(rootBone, bones);
        }
    }
    
    return bones;
}

std::vector<ExportSprite> ExportService::extractSpriteData(const std::vector<std::unique_ptr<Sprite>>& sprites) {
    std::vector<ExportSprite> exportSprites;
    exportSprites.reserve(sprites.size());
    
    for (const auto& sprite : sprites) {
        if (sprite) {
            exportSprites.push_back(convertSprite(*sprite));
        }
    }
    
    return exportSprites;
}

ExportBone ExportService::convertBone(std::shared_ptr<Bone> bone) {
    ExportBone exportBone;
    exportBone.name = bone->getName();
    exportBone.transform = bone->getLocalTransform();
    exportBone.length = bone->getLength();
    
    // Set parent name
    auto parent = bone->getParent();
    if (parent) {
        exportBone.parentName = parent->getName();
    }
    
    // Collect child names
    const auto& children = bone->getChildren();
    exportBone.childNames.reserve(children.size());
    for (const auto& child : children) {
        if (child) {
            exportBone.childNames.push_back(child->getName());
        }
    }
    
    return exportBone;
}

ExportSprite ExportService::convertSprite(const Sprite& sprite) {
    ExportSprite exportSprite;
    exportSprite.name = sprite.getName();
    exportSprite.texturePath = sprite.getTexturePath();
    exportSprite.isVisible = sprite.isVisible();

    exportSprite.transform = sprite.getLocalTransform();
    
    // Get binding information
    if (sprite.isBoundToBone()) {
        auto boundBone = sprite.getBoundBone();
        if (boundBone) {
            exportSprite.boundBoneName = boundBone->getName();
            const auto& binding = sprite.getBoneBinding();
            exportSprite.bindOffset = binding.bindOffset;
            exportSprite.bindRotation = binding.bindRotation;
            
            // Show current sprite world position for reference
            Transform spriteWorld = sprite.getWorldTransform();
            
            // Show bone world position for reference
            Transform boneWorld = boundBone->getWorldTransform();
            
        }
    } else {
        // For unbound sprites, store the local transform
        exportSprite.transform = sprite.getLocalTransform();
    }
    
    return exportSprite;
}

void ExportService::collectBoneHierarchy(std::shared_ptr<Bone> bone, std::vector<ExportBone>& bones) {
    if (!bone) return;
    
    bones.push_back(convertBone(bone));
    
    // Recursively collect children
    const auto& children = bone->getChildren();
    for (const auto& child : children) {
        collectBoneHierarchy(child, bones);
    }
}

ExportBoneTrack ExportService::convertBoneTrack(const BoneTrack& track) {
    ExportBoneTrack exportTrack;
    exportTrack.boneName = track.getBoneName();
    
    // Convert all keyframes
    const auto& keyframes = track.getKeyframes();
    exportTrack.keyframes.reserve(keyframes.size());
    
    for (const auto& keyframe : keyframes) {
        ExportKeyframe exportKeyframe;
        exportKeyframe.time = keyframe.time;
        exportKeyframe.transform = keyframe.transform;
        exportTrack.keyframes.push_back(exportKeyframe);
    }
    
    return exportTrack;
}

}