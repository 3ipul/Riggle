#pragma once

#include "Sprite.h"
#include "Rig.h"
#include "IK_Solver.h"
#include "Animation.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <chrono>


namespace Riggle {

class Character {
public:
    Character(const std::string& name);
    ~Character() = default;

    // Event system for transform changes
    struct TransformEvent {
        std::string boneName;
        Transform oldTransform;
        Transform newTransform;
        float timestamp;
    };
    
    using TransformEventHandler = std::function<void(const TransformEvent&)>;
    
    void addTransformEventHandler(TransformEventHandler handler) {
        m_transformHandlers.push_back(handler);
    }
    
    void clearTransformEventHandlers() {
        m_transformHandlers.clear();
    }

    // Basic properties
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    // Sprite management
    void addSprite(std::unique_ptr<Sprite> sprite);
    void removeSprite(const std::string& name);
    Sprite* findSprite(const std::string& name);
    void removeSprite(Sprite* sprite); // Remove by pointer
    void removeSpriteAt(size_t index); // Remove by index
    const std::vector<std::unique_ptr<Sprite>>& getSprites() const { return m_sprites; }

    // Non-const access for reordering
    std::vector<std::unique_ptr<Sprite>>& getSprites() { return m_sprites; }

    // Rig management
    void setRig(std::unique_ptr<Rig> rig);
    Rig* getRig() const { return m_rig.get(); }

    // IK functionality
    IKSolver& getIKSolver() { return m_ikSolver; }
    const IKSolver& getIKSolver() const { return m_ikSolver; }
    bool solveIK(std::shared_ptr<Bone> endEffector, const Vector2& targetPos, int chainLength);

     // Animation management
    void addAnimation(std::unique_ptr<Animation> animation);
    void removeAnimation(const std::string& name);
    Animation* findAnimation(const std::string& name);
    Animation* findAnimation(const std::string& name) const;
    const std::vector<std::unique_ptr<Animation>>& getAnimations() const { return m_animations; }
    
    // Animation playback
    AnimationPlayer* getAnimationPlayer() { return &m_animationPlayer; }
    const AnimationPlayer* getAnimationPlayer() const { return &m_animationPlayer; }
    
    // Update system
    void update(float deltaTime);

    // Deformation - CRITICAL for FK/IK
    void updateDeformations();
    void forceUpdateDeformations(); // Force immediate update
    
    // Auto-update toggle
    void setAutoUpdate(bool autoUpdate) { m_autoUpdate = autoUpdate; }
    bool getAutoUpdate() const { return m_autoUpdate; }

    void setManualBoneEditMode(bool enabled) { m_manualBoneEditMode = enabled; }
    bool isInManualBoneEditMode() const { return m_manualBoneEditMode; }

private:
    std::string m_name;
    std::vector<std::unique_ptr<Sprite>> m_sprites;
    std::unique_ptr<Rig> m_rig;
    std::vector<std::unique_ptr<Animation>> m_animations;
    std::vector<TransformEventHandler> m_transformHandlers;
    IKSolver m_ikSolver;
    AnimationPlayer m_animationPlayer;
    bool m_autoUpdate = true; // Auto-update deformations
    bool m_manualBoneEditMode = false;

    void notifyTransformChanged(const std::string& boneName, 
                               const Transform& oldTransform, 
                               const Transform& newTransform);
    float getCurrentTime() const;
    
    friend class Bone; // Allow Bone to notify Character of changes
};

} // namespace Riggle