#pragma once
#include "Bone.h"
#include "Math.h"
#include <memory>
#include <vector>

namespace Riggle {

class IK_Solver {
public:
    IK_Solver() = default;
    ~IK_Solver() = default;

    // CCD IK Solver
    static bool solveCCD(
        std::shared_ptr<Bone> endEffector,
        const Vector2& targetPosition,
        int maxIterations = 10,
        float tolerance = 1.0f
    );

private:
    // Helper functions
    static std::vector<std::shared_ptr<Bone>> buildChain(std::shared_ptr<Bone> endEffector);
    static Vector2 getEndEffectorPosition(std::shared_ptr<Bone> endEffector);
    static float angleBetweenVectors(const Vector2& a, const Vector2& b);
};

} // namespace Riggle