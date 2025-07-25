#pragma once
#include <SFML/Graphics.hpp>
#include <Riggle/Character.h>
#include <map>
#include <string>

namespace Riggle {

class SpriteRenderer {
public:
    SpriteRenderer();
    ~SpriteRenderer() = default;

    // Change parameter from sf::RenderWindow& to sf::RenderTarget&
    void render(sf::RenderTarget& target, const Character& character);
    void renderSprite(sf::RenderTarget& target, const Sprite& sprite);
    
    // Texture management
    bool loadTexture(const std::string& path);
    void clearTextureCache();

private:
    std::map<std::string, sf::Texture> m_textureCache;
    
    // Helper functions
    sf::Texture* getTexture(const std::string& path);
    sf::Vector2f toSFMLVector(const Riggle::Vertex& vertex) const;
};

} // namespace Riggle