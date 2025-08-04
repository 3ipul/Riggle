#pragma once
#include "Math.h"
#include <vector>
#include <string>
#include <memory>

namespace Riggle {
    class Rig;
    class Bone;
    
    struct IKChainValidation {
        bool isValid;
        std::string message;
        int maxPossibleLength;
        std::vector<std::shared_ptr<Bone>> chain;
    };

    class IKSolver {
    public:
        // Main solving function
        bool solveCCD(Rig* rig, std::shared_ptr<Bone> endEffector, const Vector2& targetPos, int chainLength, int maxIterations = 50, float tolerance = 1.0f);
        
        // Chain management
        std::vector<std::shared_ptr<Bone>> buildChain(std::shared_ptr<Bone> endEffector, int chainLength);
        IKChainValidation validateChain(std::shared_ptr<Bone> endEffector, int chainLength);
        
        // Utility functions
        Vector2 getBoneWorldPosition(std::shared_ptr<Bone> bone);
        Vector2 getBoneWorldEndPosition(std::shared_ptr<Bone> bone);
        float getAngleBetweenVectors(const Vector2& from, const Vector2& to);
        
    private:
        int getDistanceToRoot(std::shared_ptr<Bone> bone);
        void applyRotationToBone(std::shared_ptr<Bone> bone, float deltaAngle);
    };
}