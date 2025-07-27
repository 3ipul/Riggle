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
    if (!validateChain(chain)) {
        std::cout << "IK Error: Invalid bone chain" << std::endl;
        return false;
    }
    
    return solveCCDChain(chain, targetPosition, settings);
}

bool IK_Solver::solveCCDChain(
    const std::vector<std::shared_ptr<Bone>>& chain,
    const Vector2& targetPosition,
    const IKSolverSettings& settings)
{
    if (chain.empty()) return false;
    
    auto endEffector = chain[0]; // First bone in chain is the end effector
    
    std::cout << "IK: Starting CCD solve for chain of " << chain.size() 
              << " bones, target: (" << targetPosition.x << ", " << targetPosition.y << ")" << std::endl;

    for (int iteration = 0; iteration < settings.maxIterations; ++iteration) {
        // Get current end effector position
        Vector2 currentEnd = getEndEffectorPosition(endEffector);
        float distance = (targetPosition - currentEnd).length();
        
        // Call iteration callback if provided
        if (settings.onIteration) {
            settings.onIteration(iteration, distance);
        }
        
        // Check if we're close enough to the target
        if (distance <= settings.tolerance) {
            std::cout << "IK: Converged in " << iteration << " iterations, distance: " << distance << std::endl;
            return true; // Success!
        }
        
        // Work backwards through the chain (excluding the end effector itself)
        for (int i = static_cast<int>(chain.size()) - 2; i >= 0; --i) {
            auto bone = chain[i];
            
            // Check if this bone is locked
            auto constraints = getBoneConstraints(bone);
            if (constraints.lockRotation) {
                continue; // Skip this bone
            }
            
            // Get bone's world start position (joint position)
            float jointX, jointY, endX, endY;
            bone->getWorldEndpoints(jointX, jointY, endX, endY);
            Vector2 jointPos(jointX, jointY);
            
            // Get current end effector position
            currentEnd = getEndEffectorPosition(endEffector);
            
            // Calculate vectors from joint to current end and to target
            Vector2 toEnd = currentEnd - jointPos;
            Vector2 toTarget = targetPosition - jointPos;
            
            // Skip if vectors are too small
            if (toEnd.length() < 0.001f || toTarget.length() < 0.001f) {
                continue;
            }
            
            // Calculate angle between vectors
            float angleToRotate = angleBetweenVectors(toEnd, toTarget);
            
            // Apply dampening
            angleToRotate *= settings.dampening;
            
            // Apply constraints if enabled
            if (settings.enableConstraints && constraints.enableAngleLimits) {
                float currentRotation = bone->getLocalTransform().rotation;
                float newRotation = normalizeAngle(currentRotation + angleToRotate);
                newRotation = clampAngle(newRotation, constraints.minAngle, constraints.maxAngle);
                angleToRotate = newRotation - currentRotation;
            }
            
            // Apply rotation to the bone
            if (std::abs(angleToRotate) > 0.001f) { // Only rotate if significant
                float currentRotation = bone->getLocalTransform().rotation;
                bone->setLocalRotation(currentRotation + angleToRotate);
            }
        }
    }
    
    // Final distance check
    Vector2 finalEnd = getEndEffectorPosition(endEffector);
    float finalDistance = (targetPosition - finalEnd).length();
    
    std::cout << "IK: Did not converge within " << settings.maxIterations 
              << " iterations, final distance: " << finalDistance << std::endl;
    
    return finalDistance <= settings.tolerance * 2.0f; // More lenient final check
}

void IK_Solver::setBoneConstraints(std::shared_ptr<Bone> bone, const IKConstraints& constraints) {
    if (bone) {
        s_boneConstraints[bone] = constraints;
        std::cout << "IK: Set constraints for bone '" << bone->getName() << "'" << std::endl;
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
    
    auto current = endEffector;
    while (current) {
        chain.push_back(current);
        current = current->getParent();
    }
    
    std::cout << "IK: Built chain of " << chain.size() << " bones to root" << std::endl;
    return chain;
}

std::vector<std::shared_ptr<Bone>> IK_Solver::buildChainBetween(
    std::shared_ptr<Bone> start, 
    std::shared_ptr<Bone> end) 
{
    std::vector<std::shared_ptr<Bone>> chain;
    
    // Build chain from start to root
    auto current = start;
    while (current && current != end) {
        chain.push_back(current);
        current = current->getParent();
    }
    
    if (current == end) {
        chain.push_back(end);
        std::cout << "IK: Built chain of " << chain.size() << " bones between specified bones" << std::endl;
    } else {
        std::cout << "IK Warning: End bone not found in parent chain of start bone" << std::endl;
        chain.clear();
    }
    
    return chain;
}

Vector2 IK_Solver::getEndEffectorPosition(std::shared_ptr<Bone> endEffector) {
    float startX, startY, endX, endY;
    endEffector->getWorldEndpoints(startX, startY, endX, endY);
    return Vector2(endX, endY);
}

bool IK_Solver::validateChain(const std::vector<std::shared_ptr<Bone>>& chain) {
    if (chain.empty()) {
        std::cout << "IK Error: Empty bone chain" << std::endl;
        return false;
    }
    
    // Check that all bones in chain are connected
    for (size_t i = 1; i < chain.size(); ++i) {
        if (chain[i-1]->getParent() != chain[i]) {
            std::cout << "IK Error: Broken bone chain at index " << i << std::endl;
            return false;
        }
    }
    
    return true;
}

float IK_Solver::angleBetweenVectors(const Vector2& a, const Vector2& b) {
    // Normalize vectors
    float lenA = a.length();
    float lenB = b.length();
    
    if (lenA < 0.001f || lenB < 0.001f) return 0.0f;
    
    Vector2 normA = a / lenA;
    Vector2 normB = b / lenB;
    
    // Calculate angle using atan2 for proper sign
    float cross = normA.cross(normB);
    float dot = normA.dot(normB);
    
    return std::atan2(cross, dot);
}

float IK_Solver::clampAngle(float angle, float minAngle, float maxAngle) {
    return std::max(minAngle, std::min(maxAngle, angle));
}

float IK_Solver::normalizeAngle(float angle) {
    // Normalize angle to [-π, π] range
    while (angle > 3.14159f) angle -= 2.0f * 3.14159f;
    while (angle < -3.14159f) angle += 2.0f * 3.14159f;
    return angle;
}

} // namespace Riggle