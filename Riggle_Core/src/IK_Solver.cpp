#include "Riggle/IK_Solver.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Riggle {

// Static constraint storage
std::map<std::shared_ptr<Bone>, IKConstraints> IK_Solver::s_boneConstraints;

bool IK_Solver::solveCCD(
    std::shared_ptr<Bone> endEffector,
    const Vector2& targetPosition,
    const IKSolverSettings& settings)
{
    if (!endEffector) {
        std::cout << "IK Error: No end effector provided" << std::endl;
        return false;
    }

    // Build the bone chain from end effector to root
    auto chain = buildChainToRoot(endEffector);
    if (chain.empty()) {
        std::cout << "IK Error: Empty bone chain" << std::endl;
        return false;
    }
    
    std::cout << "IK: Solving for chain of " << chain.size() << " bones" << std::endl;
    return solveCCDChain(chain, targetPosition, settings);
}

bool IK_Solver::solveCCDChain(
    const std::vector<std::shared_ptr<Bone>>& chain,
    const Vector2& targetPosition,
    const IKSolverSettings& settings)
{
    if (chain.empty()) {
        std::cout << "IK Error: Empty chain" << std::endl;
        return false;
    }
    
    if (chain.size() == 1) {
        std::cout << "IK: Single bone - cannot solve IK" << std::endl;
        return false;
    }
    
    std::cout << "IK: Starting CCD solve with " << chain.size() << " bones" << std::endl;
    
    for (int iteration = 0; iteration < 5; ++iteration) {
        
        // Get current end effector position
        if (!chain[0]) {
            std::cout << "IK Error: Null end effector" << std::endl;
            return false;
        }
        
        float endX, endY, startX, startY;
        chain[0]->getWorldEndpoints(startX, startY, endX, endY);
        Vector2 currentEnd(endX, endY);
        
        // Check distance
        float dx = currentEnd.x - targetPosition.x;
        float dy = currentEnd.y - targetPosition.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        
        std::cout << "IK Iteration " << iteration << ": distance = " << distance << std::endl;
        
        if (distance < 15.0f) {
            std::cout << "IK: Converged!" << std::endl;
            return true;
        }
        
        // FIXED: Rotate ALL bones including end effector (index 0)
        for (int i = 0; i < chain.size(); ++i) {
            auto bone = chain[i];
            if (!bone) {
                std::cout << "IK Error: Null bone at index " << i << std::endl;
                continue;
            }
            
            std::cout << "IK: Rotating bone '" << bone->getName() << "'" << std::endl;
            
            // Get joint position (start of this bone)
            float jointStartX, jointStartY, jointEndX, jointEndY;
            bone->getWorldEndpoints(jointStartX, jointStartY, jointEndX, jointEndY);
            Vector2 jointPos(jointStartX, jointStartY);
            
            // Update current end position
            chain[0]->getWorldEndpoints(startX, startY, endX, endY);
            currentEnd = Vector2(endX, endY);
            
            // Calculate rotation needed
            Vector2 toEnd(currentEnd.x - jointPos.x, currentEnd.y - jointPos.y);
            Vector2 toTarget(targetPosition.x - jointPos.x, targetPosition.y - jointPos.y);
            
            float toEndLen = std::sqrt(toEnd.x * toEnd.x + toEnd.y * toEnd.y);
            float toTargetLen = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);
            
            if (toEndLen < 0.1f || toTargetLen < 0.1f) continue;
            
            // Simple angle calculation
            float currentAngle = std::atan2(toEnd.y, toEnd.x);
            float targetAngle = std::atan2(toTarget.y, toTarget.x);
            float rotationDelta = targetAngle - currentAngle;
            
            // Normalize angle
            while (rotationDelta > 3.14159f) rotationDelta -= 2.0f * 3.14159f;
            while (rotationDelta < -3.14159f) rotationDelta += 2.0f * 3.14159f;
            
            // Apply small rotation
            rotationDelta *= 0.3f;  // Heavy dampening for stability
            
            Transform boneTransform = bone->getLocalTransform();
            boneTransform.rotation += rotationDelta;
            bone->setLocalTransform(boneTransform);
            
            // CRITICAL: Force update all children transforms
            forceUpdateChildrenTransforms(bone);
            
            std::cout << "IK: Applied rotation " << (rotationDelta * 180.0f / 3.14159f) << " degrees" << std::endl;
        }
    }
    
    std::cout << "IK: Did not converge" << std::endl;
    return false;
}

void IK_Solver::setBoneConstraints(std::shared_ptr<Bone> bone, const IKConstraints& constraints) {
    if (bone) {
        s_boneConstraints[bone] = constraints;
    }
}

IKConstraints IK_Solver::getBoneConstraints(std::shared_ptr<Bone> bone) {
    auto it = s_boneConstraints.find(bone);
    if (it != s_boneConstraints.end()) {
        return it->second;
    }
    return IKConstraints(); // Default constraints
}

std::vector<std::shared_ptr<Bone>> IK_Solver::buildChainToRoot(std::shared_ptr<Bone> endEffector) {
    std::vector<std::shared_ptr<Bone>> chain;
    
    if (!endEffector) {
        std::cout << "IK Error: Null end effector" << std::endl;
        return chain;
    }
    
    // Start with the end effector
    auto current = endEffector;
    
    // Build a SHORT chain (max 3 bones for testing)
    int maxChainLength = 3;
    
    for (int i = 0; i < maxChainLength && current; ++i) {
        chain.push_back(current);
        std::cout << "IK: Added bone '" << current->getName() << "' to chain (index " << i << ")" << std::endl;
        
        current = current->getParent();
        
        // If we reached a bone with no parent, stop
        if (!current) {
            std::cout << "IK: Reached root bone, stopping chain build" << std::endl;
            break;
        }
    }
    
    std::cout << "IK: Built chain of " << chain.size() << " bones" << std::endl;
    return chain;
}

std::vector<std::shared_ptr<Bone>> IK_Solver::buildChainBetween(
    std::shared_ptr<Bone> start, 
    std::shared_ptr<Bone> end) 
{
    std::vector<std::shared_ptr<Bone>> chain;
    
    if (!start || !end) {
        return chain;
    }
    
    auto current = start;
    while (current && current != end) {
        chain.push_back(current);
        current = current->getParent();
        
        // Safety check
        if (chain.size() > 100) {
            std::cout << "IK Warning: Chain search too long, breaking" << std::endl;
            chain.clear();
            return chain;
        }
    }
    
    if (current == end) {
        chain.push_back(end);
    } else {
        chain.clear();
    }
    
    return chain;
}

Vector2 IK_Solver::getEndEffectorPosition(std::shared_ptr<Bone> endEffector) {
    if (!endEffector) {
        std::cout << "IK Error: Null bone in getEndEffectorPosition" << std::endl;
        return Vector2(0.0f, 0.0f);
    }
    
    float startX, startY, endX, endY;
    endEffector->getWorldEndpoints(startX, startY, endX, endY);
    return Vector2(endX, endY);
}

bool IK_Solver::validateChain(const std::vector<std::shared_ptr<Bone>>& chain) {
    if (chain.empty()) {
        return false;
    }
    
    // Check that all bones are valid
    for (const auto& bone : chain) {
        if (!bone) {
            std::cout << "IK Error: Null bone in chain" << std::endl;
            return false;
        }
    }
    
    // Check connectivity (optional for now)
    return true;
}

float IK_Solver::angleBetweenVectors(const Vector2& a, const Vector2& b) {
    float dot = a.x * b.x + a.y * b.y;
    float cross = a.x * b.y - a.y * b.x;
    return std::atan2(cross, dot);
}

float IK_Solver::clampAngle(float angle, float minAngle, float maxAngle) {
    return std::max(minAngle, std::min(maxAngle, angle));
}

float IK_Solver::normalizeAngle(float angle) {
    const float PI = 3.14159f;
    while (angle > PI) angle -= 2.0f * PI;
    while (angle < -PI) angle += 2.0f * PI;
    return angle;
}

void IK_Solver::forceUpdateChildrenTransforms(std::shared_ptr<Bone> bone) {
    if (!bone) return;
    
    // Force update this bone's world transform
    bone->getWorldTransform();
    
    // Recursively update all children
    const auto& children = bone->getChildren();
    for (const auto& child : children) {
        if (child) {
            // Mark child as dirty and update
            child->markWorldTransformDirty();
            forceUpdateChildrenTransforms(child);
        }
    }
}

} // namespace Riggle