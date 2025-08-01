#pragma once
#include <Riggle/Character.h>
#include <Riggle/Sprite.h>
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <memory>

namespace Riggle {

class SpriteRenderer {
public:
    SpriteRenderer();
    ~SpriteRenderer() = default;

    void setCharacter(Character* character) { m_character = character; }
    
    void render(sf::RenderTarget& target);
    void renderSprite(sf::RenderTarget& target, Sprite* sprite);
    void renderSpriteHighlight(sf::RenderTarget& target, Sprite* sprite);
    
    // Texture management
    sf::Texture* getTexture(const std::string& path);
    void clearTextureCache();

private:
    Character* m_character;
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> m_textureCache;
    
    // Default texture for missing files
    sf::Texture m_defaultTexture;
    
    void createDefaultTexture();
    sf::Texture* loadTexture(const std::string& path);
};

} // namespace Riggle