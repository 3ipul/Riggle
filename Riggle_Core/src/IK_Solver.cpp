#include "Riggle/IK_Solver.h"
#include "Riggle/Rig.h"
#include "Riggle/Bone.h"
#include <cmath>
#include <algorithm>

namespace Riggle {

bool IKSolver::solveCCD(Rig* rig,std::shared_ptr<Bone> endEffector, const Vector2& targetPos, int chainLength, int maxIterations, float tolerance) {
    if (!endEffector) return false;

    auto validation = validateChain(endEffector, chainLength);
    if (!validation.isValid) return false;

    std::vector<std::shared_ptr<Bone>> chain = validation.chain;
    if (chain.size() < 2) return false; // Need at least one bone to rotate and an end-effector.

    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        Vector2 endPos = getBoneWorldEndPosition(endEffector);
        if ((targetPos - endPos).length() < tolerance) {
            return true; // Success
        }

        // **Correct CCD: Iterate backwards from the end-effector's parent up the chain.**
        // The chain is ordered [Root-of-chain, ..., Parent, EndEffector].
        // So we loop from chain.size() - 2 down to 0.
        for (int i = chain.size() - 1; i >= 0; --i) {
            std::shared_ptr<Bone> currentBone = chain[i];
            Vector2 jointPos = getBoneWorldPosition(currentBone);
            Vector2 currentEndPos = getBoneWorldEndPosition(endEffector);

            Vector2 toEnd = currentEndPos - jointPos;
            Vector2 toTarget = targetPos - jointPos;

            // Safety check: skip if vectors are too small
            if (toEnd.lengthSquared() < 0.000001f || toTarget.lengthSquared() < 0.000001f) {
                continue;
            }

            toEnd = toEnd.normalized();
            toTarget = toTarget.normalized();
            float dot = std::max(-1.0f, std::min(1.0f, toEnd.dot(toTarget)));
            
            // Skip if already aligned
            if (dot > 0.9999f) {
                continue;
            }

            float angle = std::acos(dot);
            if (toEnd.cross(toTarget) < 0) {
                angle = -angle;
            }

            if (std::abs(angle) > 0.0001f) {
                // After rotating a bone, we MUST update the world transforms
                // of all bones so the next calculation is accurate.
                applyRotationToBone(currentBone, angle);
                if (i > 0) rig->forceUpdateWorldTransforms();
            }
        }
    }

    return (targetPos - getBoneWorldEndPosition(endEffector)).length() < tolerance;
}

std::vector<std::shared_ptr<Bone>> IKSolver::buildChain(std::shared_ptr<Bone> endEffector, int chainLength) {
    std::vector<std::shared_ptr<Bone>> chain;
    if (!endEffector || chainLength <= 0) return chain;

    std::shared_ptr<Bone> current = endEffector;
    for (int i = 0; i < chainLength && current != nullptr; ++i) {
        chain.push_back(current);
        current = current->getParent();
    }

    // The chain is currently [End, Parent, Grandparent, ...].
    // Reverse it to be [Root-of-chain, ..., Parent, End].
    std::reverse(chain.begin(), chain.end());
    return chain;
}

IKChainValidation IKSolver::validateChain(std::shared_ptr<Bone> endEffector, int chainLength) {
    if (!endEffector) {
        return {false, "No end effector selected", 0, {}};
    }

    if (chainLength <= 0) {
        return {false, "Chain length must be at least 1", 0, {}};
    }

    // Calculate the actual maximum possible chain length from this bone
    int maxPossibleLength = 1; // Start with the end-effector itself
    std::shared_ptr<Bone> current = endEffector;
    while(current->getParent() != nullptr) {
        maxPossibleLength++;
        current = current->getParent();
    }

    if (chainLength > maxPossibleLength) {
        return {false, "Only " + std::to_string(maxPossibleLength) + " bones available in chain", maxPossibleLength, {}};
    }

    auto chain = buildChain(endEffector, chainLength);
    return {true, "Valid chain", maxPossibleLength, chain};
}

Vector2 IKSolver::getBoneWorldPosition(std::shared_ptr<Bone> bone) {
    if (!bone) return Vector2(0, 0);
    
    // Get the world transform start position (joint position)
    Transform worldTransform = bone->getWorldTransform();
    return worldTransform.position;
}

Vector2 IKSolver::getBoneWorldEndPosition(std::shared_ptr<Bone> bone) {
    if (!bone) return Vector2(0, 0);
    
    // Get world endpoints using existing Bone method
    float startX, startY, endX, endY;
    bone->getWorldEndpoints(startX, startY, endX, endY);
    return Vector2(endX, endY);
}

float IKSolver::getAngleBetweenVectors(const Vector2& from, const Vector2& to) {
    Vector2 fromNorm = from.normalized();
    Vector2 toNorm = to.normalized();
    
    float dot = fromNorm.dot(toNorm);
    float cross = fromNorm.cross(toNorm);
    
    return std::atan2(cross, dot);
}

int IKSolver::getDistanceToRoot(std::shared_ptr<Bone> bone) {
    int distance = 0;
    std::shared_ptr<Bone> current = bone;
    
    while (current && current->getParent()) {
        current = current->getParent();
        distance++;
    }
    
    return distance;
}

void IKSolver::applyRotationToBone(std::shared_ptr<Bone> bone, float deltaAngle) {
    if (!bone) return;
    
    // Get current local transform
    Transform localTransform = bone->getLocalTransform();
    
    // Add delta angle to current rotation
    localTransform.rotation += deltaAngle;
    
    // Set the updated transform
    bone->setLocalTransform(localTransform);
}

}