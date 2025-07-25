#include "Riggle/IK_Solver.h"
#include <cmath>
#include <algorithm>

namespace Riggle {

bool IK_Solver::solveCCD(
    std::shared_ptr<Bone> endEffector,
    const Vector2& targetPosition,
    int maxIterations,
    float tolerance)
{
    if (!endEffector) return false;

    // Build the bone chain from end effector to root
    auto chain = buildChain(endEffector);
    if (chain.empty()) return false;

    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        // Check if we're close enough to the target
        Vector2 currentEnd = getEndEffectorPosition(endEffector);
        float distance = (targetPosition - currentEnd).length();
        
        if (distance <= tolerance) {
            return true; // Success!
        }

        // Work backwards through the chain (excluding the end effector itself)
        for (int i = static_cast<int>(chain.size()) - 2; i >= 0; --i) {
            auto bone = chain[i];
            
            // Get bone's world position (joint position)
            Transform boneWorld = bone->getWorldTransform();
            Vector2 jointPos(boneWorld.x, boneWorld.y);
            
            // Get current end effector position
            currentEnd = getEndEffectorPosition(endEffector);
            
            // Calculate vectors from joint to current end and to target
            Vector2 toEnd = currentEnd - jointPos;
            Vector2 toTarget = targetPosition - jointPos;
            
            // Calculate angle between vectors
            float angle = angleBetweenVectors(toEnd, toTarget);
            
            // Apply rotation to the bone
            float currentRotation = bone->getLocalTransform().rotation;
            bone->setLocalRotation(currentRotation + angle);
        }
    }

    return false; // Didn't converge within max iterations
}

std::vector<std::shared_ptr<Bone>> IK_Solver::buildChain(std::shared_ptr<Bone> endEffector) {
    std::vector<std::shared_ptr<Bone>> chain;
    
    auto current = endEffector;
    while (current) {
        chain.push_back(current);
        current = current->getParent();
    }
    
    return chain;
}

Vector2 IK_Solver::getEndEffectorPosition(std::shared_ptr<Bone> endEffector) {
    float startX, startY, endX, endY;
    endEffector->getWorldEndpoints(startX, startY, endX, endY);
    return Vector2(endX, endY);
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

} // namespace Riggle