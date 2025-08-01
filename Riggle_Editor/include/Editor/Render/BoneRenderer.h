#pragma once
#include <Riggle/Character.h>
#include <Riggle/Bone.h>
#include <SFML/Graphics.hpp>
#include <memory>

namespace Riggle {

class BoneRenderer {
public:
    BoneRenderer();
    ~BoneRenderer() = default;

    void setCharacter(Character* character) { m_character = character; }
    
    void render(sf::RenderTarget& target, float zoomLevel = 1.0f);
    void renderBone(sf::RenderTarget& target, std::shared_ptr<Bone> bone, float zoomLevel);
    void renderBoneHighlight(sf::RenderTarget& target, std::shared_ptr<Bone> bone, float zoomLevel = 1.0f);
    
    // Display options
    void setShowBoneNames(bool show) { m_showBoneNames = show; }
    void setShowJoints(bool show) { m_showJoints = show; }
    void setBoneThickness(float thickness) { m_boneThickness = thickness; }

private:
    Character* m_character;
    
    // Rendering options
    bool m_showBoneNames;
    bool m_showJoints;
    float m_boneThickness;
    
    // Colors
    sf::Color m_boneColor;
    sf::Color m_jointColor;
    sf::Color m_selectedBoneColor;
    sf::Color m_rootBoneColor;
    
    // Font for bone names (optional)
    sf::Font m_font;
    bool m_fontLoaded;
    
    void renderBoneLine(sf::RenderTarget& target, const sf::Vector2f& start, const sf::Vector2f& end, 
                       const sf::Color& color, float thickness = 2.0f);
    void renderBoneTriangle(sf::RenderTarget& target, const sf::Vector2f& start, const sf::Vector2f& end, 
                           const sf::Color& color, float thickness, float zoomLevel = 1.0f);
    void renderJoint(sf::RenderTarget& target, const sf::Vector2f& position, 
                    const sf::Color& color, float radius = 5.0f);
    void renderBoneName(sf::RenderTarget& target, std::shared_ptr<Bone> bone, const sf::Vector2f& position);
};

} // namespace Riggle