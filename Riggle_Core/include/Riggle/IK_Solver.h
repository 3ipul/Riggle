#pragma once

#include "Bone.h"
#include "Math.h"
#include <memory>
#include <vector>
#include <map>
#include <functional>

namespace Riggle {

struct IKConstraints {
    bool enableAngleLimits = false;
    float minAngle = -3.14159f;  // -180 degrees
    float maxAngle = 3.14159f;   // +180 degrees
    bool lockRotation = false;   // Lock this joint from IK rotation
};

struct IKSolverSettings {
    int maxIterations = 15;
    float tolerance = 2.0f;      // Distance tolerance in pixels
    float dampening = 0.8f;      // Dampening factor (0.0 - 1.0)
    bool enableConstraints = true;
    
    // Callback for iteration updates (for debugging/visualization)
    std::function<void(int iteration, float distance)> onIteration = nullptr;
};

class IK_Solver {
public:
    IK_Solver() = default;
    ~IK_Solver() = default;

    // Main CCD IK Solver
    static bool solveCCD(
        std::shared_ptr<Bone> endEffector,
        const Vector2& targetPosition,
        const IKSolverSettings& settings = IKSolverSettings()
    );
    
    // Solve IK for a specific bone chain
    static bool solveCCDChain(
        const std::vector<std::shared_ptr<Bone>>& chain,
        const Vector2& targetPosition,
        const IKSolverSettings& settings = IKSolverSettings()
    );
    
    // Set constraints for a bone
    static void setBoneConstraints(std::shared_ptr<Bone> bone, const IKConstraints& constraints);
    static IKConstraints getBoneConstraints(std::shared_ptr<Bone> bone);
    
    // Utility functions
    static std::vector<std::shared_ptr<Bone>> buildChainToRoot(std::shared_ptr<Bone> endEffector);
    static std::vector<std::shared_ptr<Bone>> buildChainBetween(
        std::shared_ptr<Bone> start, 
        std::shared_ptr<Bone> end
    );
    
    // Get end effector position
    static Vector2 getEndEffectorPosition(std::shared_ptr<Bone> endEffector);
    
    // Validation
    static bool validateChain(const std::vector<std::shared_ptr<Bone>>& chain);

private:
    // Helper functions
    static float angleBetweenVectors(const Vector2& a, const Vector2& b);
    static float clampAngle(float angle, float minAngle, float maxAngle);
    static float normalizeAngle(float angle);
    
    // Constraint system
    static std::map<std::shared_ptr<Bone>, IKConstraints> s_boneConstraints;
};

} // namespace Riggle