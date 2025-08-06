#include "Editor/Export/PNGExporter.h"
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace Riggle {

PNGSequenceExporter::PNGSequenceExporter() 
    : m_frameRate(30), m_width(1920), m_height(1080) {
}

bool PNGSequenceExporter::exportAnimation(const ExportAnimation& animation, 
                                         const std::vector<ExportSprite>& sprites,
                                         const std::vector<ExportBone>& bones,
                                         const std::string& outputPath) {
    try {
        // Create output directory
        if (!createDirectory(outputPath)) {
            m_lastError = "Failed to create output directory: " + outputPath;
            return false;
        }

        // Clear texture cache
        m_textureCache.clear();

        // Calculate total frames
        int totalFrames = static_cast<int>(animation.duration * m_frameRate);
        float frameTime = 1.0f / m_frameRate;

        std::cout << "Exporting " << totalFrames + 1 << " frames at " << m_frameRate << " FPS" << std::endl;
        std::cout << "Animation duration: " << animation.duration << " seconds" << std::endl;

        // Export each frame
        for (int frame = 0; frame <= totalFrames; ++frame) {
            float currentTime = frame * frameTime;
            
            // Generate frame filename
            std::ostringstream filename;
            filename << outputPath << "/frame_" << std::setfill('0') << std::setw(6) << frame << ".png";
            
            if (!renderFrame(currentTime, animation, sprites, bones, filename.str())) {
                m_lastError = "Failed to render frame " + std::to_string(frame);
                return false;
            }
            
            // Progress feedback
            if (frame % 10 == 0 || frame == totalFrames) {
                std::cout << "Rendered frame " << frame << "/" << totalFrames << std::endl;
            }
        }

        std::cout << "PNG sequence export completed successfully!" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        m_lastError = "Exception during PNG sequence export: " + std::string(e.what());
        return false;
    }
}

bool PNGSequenceExporter::renderFrame(float time, const ExportAnimation& animation,
                                     const std::vector<ExportSprite>& sprites, 
                                     const std::vector<ExportBone>& bones, 
                                     const std::string& framePath) {
    // Create render target
    sf::RenderTexture renderTexture;
    if (!renderTexture.resize({ static_cast<unsigned>(m_width), static_cast<unsigned>(m_height) })) {
        return false;
    }

    // Clear with transparent background
    renderTexture.clear(sf::Color::Transparent);

    // Step 1: Apply animation to bones (replicate Animation::applyAtTime)
    std::vector<ExportBone> animatedBones = bones;
    applyAnimationToBones(animatedBones, animation, time);

    // Step 2: Calculate world transforms for all bones (replicate Rig::forceUpdateWorldTransforms)
    calculateAllWorldTransforms(animatedBones);

    // Step 3: Render sprites with proper world transforms
    for (const auto& sprite : sprites) {
        if (!sprite.isVisible) continue;

        // Load or get cached texture
        sf::Texture* texture = nullptr;
        auto it = m_textureCache.find(sprite.texturePath);
        if (it != m_textureCache.end()) {
            texture = &it->second;
        } else {
            // Load and cache texture
            sf::Texture newTexture;
            if (newTexture.loadFromFile(sprite.texturePath)) {
                m_textureCache[sprite.texturePath] = std::move(newTexture);
                texture = &m_textureCache[sprite.texturePath];
            } else {
                std::cout << "Warning: Failed to load texture: " << sprite.texturePath << std::endl;
                continue;
            }
        }

        sf::Sprite sfSprite(*texture);
        
        // Calculate sprite's world transform
        Transform spriteWorldTransform = calculateSpriteWorldTransform(sprite, animatedBones);
        
        // Set sprite origin to center for proper rotation
        sf::Vector2u textureSize = texture->getSize();
        sfSprite.setOrigin({textureSize.x * 0.5f, textureSize.y * 0.5f});
        
        // Apply world transform (center the view in the render target)
        sfSprite.setPosition({
            spriteWorldTransform.position.x + m_width * 0.5f,
            spriteWorldTransform.position.y + m_height * 0.5f
        });
        // sfSprite.setRotation(spriteWorldTransform.rotation * 180.0f / 3.14159f); // Convert to degrees
        sfSprite.setRotation(sf::degrees(spriteWorldTransform.rotation * 180.f / 3.14159265f));
        sfSprite.setScale({spriteWorldTransform.scale.x, spriteWorldTransform.scale.y});

        renderTexture.draw(sfSprite);
    }

    // Finalize and save
    renderTexture.display();
    sf::Image image = renderTexture.getTexture().copyToImage();
    
    return image.saveToFile(framePath);
}

void PNGSequenceExporter::applyAnimationToBones(std::vector<ExportBone>& animatedBones, 
                                                const ExportAnimation& animation, float time) {
    // Apply animation transforms to each bone - exactly like Animation::applyAtTime
    for (auto& bone : animatedBones) {
        // Find the bone's track in the animation
        bool foundTrack = false;
        for (const auto& track : animation.tracks) {
            if (track.boneName == bone.name) {
                // Get interpolated transform at current time
                Transform animatedTransform = interpolateTransform(track.keyframes, time);
                bone.transform = animatedTransform; // This sets the LOCAL transform
                foundTrack = true;
                break;
            }
        }
        
        // If no track found, keep the original transform (rest pose)
        if (!foundTrack) {
            // bone.transform already contains the rest pose
        }
    }
}

void PNGSequenceExporter::calculateAllWorldTransforms(std::vector<ExportBone>& bones) {
    // First, mark all world transforms as needing calculation
    std::map<std::string, Transform> worldTransforms;
    
    // Calculate world transforms for all bones, respecting hierarchy
    for (auto& bone : bones) {
        if (worldTransforms.find(bone.name) == worldTransforms.end()) {
            worldTransforms[bone.name] = calculateBoneWorldTransform(bone, bones, worldTransforms);
        }
    }
    
    // Store calculated world transforms back to bones
    for (auto& bone : bones) {
        if (worldTransforms.find(bone.name) != worldTransforms.end()) {
            bone.worldTransform = worldTransforms[bone.name];
        }
    }
}

Transform PNGSequenceExporter::calculateBoneWorldTransform(const ExportBone& bone, 
                                                          const std::vector<ExportBone>& bones,
                                                          std::map<std::string, Transform>& worldTransforms) {
    // If already calculated, return it
    if (worldTransforms.find(bone.name) != worldTransforms.end()) {
        return worldTransforms[bone.name];
    }
    
    Transform worldTransform = bone.transform; // Start with local transform
    
    // If bone has a parent, apply parent's world transform
    if (!bone.parentName.empty()) {
        const ExportBone* parent = findBone(bones, bone.parentName);
        if (parent) {
            // Recursively calculate parent's world transform
            Transform parentWorldTransform = calculateBoneWorldTransform(*parent, bones, worldTransforms);
            
            // Apply parent transform to this bone's local transform
            worldTransform = combineTransforms(parentWorldTransform, bone.transform);
        }
    }
    
    // Cache the result
    worldTransforms[bone.name] = worldTransform;
    return worldTransform;
}

Transform PNGSequenceExporter::combineTransforms(const Transform& parent, const Transform& local) {
    Transform result;
    
    // Rotate local position by parent rotation
    float cosRot = std::cos(parent.rotation);
    float sinRot = std::sin(parent.rotation);
    
    Vector2 rotatedPos;
    rotatedPos.x = local.position.x * cosRot - local.position.y * sinRot;
    rotatedPos.y = local.position.x * sinRot + local.position.y * cosRot;
    
    // Apply parent scale and position
    result.position.x = parent.position.x + rotatedPos.x * parent.scale.x;
    result.position.y = parent.position.y + rotatedPos.y * parent.scale.y;
    result.rotation = parent.rotation + local.rotation;
    result.scale.x = parent.scale.x * local.scale.x;
    result.scale.y = parent.scale.y * local.scale.y;
    result.length = local.length; // Length doesn't inherit
    
    return result;
}

Transform PNGSequenceExporter::calculateSpriteWorldTransform(const ExportSprite& sprite, 
                                                            const std::vector<ExportBone>& animatedBones) {
    Transform spriteWorldTransform = sprite.transform; // Start with sprite's local transform
    
    // If sprite is bound to a bone, apply bone's world transform with binding
    if (!sprite.boundBoneName.empty()) {
        const ExportBone* boundBone = findBone(animatedBones, sprite.boundBoneName);
        if (boundBone) {
            // Get bone's world transform (should already be calculated)
            Transform boneWorldTransform = boundBone->worldTransform;
            
            // Apply binding offset and rotation
            float totalRotation = boneWorldTransform.rotation + sprite.bindRotation;
            float cosRot = std::cos(totalRotation);
            float sinRot = std::sin(totalRotation);
            
            // Rotate bind offset by bone's world rotation
            Vector2 rotatedOffset;
            rotatedOffset.x = sprite.bindOffset.x * std::cos(boneWorldTransform.rotation) - 
                             sprite.bindOffset.y * std::sin(boneWorldTransform.rotation);
            rotatedOffset.y = sprite.bindOffset.x * std::sin(boneWorldTransform.rotation) + 
                             sprite.bindOffset.y * std::cos(boneWorldTransform.rotation);
            
            // Apply bone world transform with binding
            spriteWorldTransform.position.x = boneWorldTransform.position.x + rotatedOffset.x * boneWorldTransform.scale.x;
            spriteWorldTransform.position.y = boneWorldTransform.position.y + rotatedOffset.y * boneWorldTransform.scale.y;
            spriteWorldTransform.rotation = totalRotation;
            spriteWorldTransform.scale.x = sprite.transform.scale.x * boneWorldTransform.scale.x;
            spriteWorldTransform.scale.y = sprite.transform.scale.y * boneWorldTransform.scale.y;
        }
    }
    
    return spriteWorldTransform;
}

Transform PNGSequenceExporter::interpolateTransform(const std::vector<ExportKeyframe>& keyframes, float time) {
    if (keyframes.empty()) {
        return Transform();
    }

    if (keyframes.size() == 1 || time <= keyframes.front().time) {
        return keyframes.front().transform;
    }

    if (time >= keyframes.back().time) {
        return keyframes.back().transform;
    }

    // Find surrounding keyframes
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            const auto& k1 = keyframes[i];
            const auto& k2 = keyframes[i + 1];
            
            float t = (time - k1.time) / (k2.time - k1.time);
            
            Transform result;
            result.position.x = k1.transform.position.x + t * (k2.transform.position.x - k1.transform.position.x);
            result.position.y = k1.transform.position.y + t * (k2.transform.position.y - k1.transform.position.y);
            
            // Shortest path rotation interpolation (same as your BoneTrack::interpolateTransforms)
            float angleDiff = k2.transform.rotation - k1.transform.rotation;
            const float PI = 3.14159f;
            
            // Normalize angle difference to [-π, π]
            while (angleDiff > PI) angleDiff -= 2.0f * PI;
            while (angleDiff < -PI) angleDiff += 2.0f * PI;
            
            result.rotation = k1.transform.rotation + angleDiff * t;
            
            result.scale.x = k1.transform.scale.x + t * (k2.transform.scale.x - k1.transform.scale.x);
            result.scale.y = k1.transform.scale.y + t * (k2.transform.scale.y - k1.transform.scale.y);
            result.length = k1.transform.length + t * (k2.transform.length - k1.transform.length);
            
            return result;
        }
    }

    return keyframes.back().transform;
}

const ExportBone* PNGSequenceExporter::findBone(const std::vector<ExportBone>& bones, const std::string& name) {
    auto it = std::find_if(bones.begin(), bones.end(),
        [&name](const ExportBone& bone) {
            return bone.name == name;
        });
    
    return (it != bones.end()) ? &(*it) : nullptr;
}

bool PNGSequenceExporter::createDirectory(const std::string& path) {
    try {
        std::filesystem::create_directories(path);
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Failed to create directory: " << e.what() << std::endl;
        return false;
    }
}

}