#pragma once
#include <SFML/Graphics.hpp>
#include <Riggle/Rig.h>

namespace Riggle {

class BoneRenderer {
public:
    BoneRenderer();
    ~BoneRenderer() = default;

    // Change parameter from sf::RenderWindow& to sf::RenderTarget&
    void render(sf::RenderTarget& target, const Rig& rig);

private:
    // Helper methods
    void renderBone(sf::RenderTarget& target, const Bone& bone);
    void drawBoneLine(sf::RenderTarget& target, float startX, float startY, float endX, float endY);
    void drawJoint(sf::RenderTarget& target, float x, float y, float radius = 5.0f);
};

} // namespace Riggle