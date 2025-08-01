#pragma once
#include <SFML/Graphics.hpp>
#include <string>

namespace Riggle {

class BasePanel {
public:
    BasePanel(const std::string& name) : m_name(name), m_isVisible(true) {}
    virtual ~BasePanel() = default;

    virtual void render() = 0;
    virtual void update(sf::RenderWindow& window) {}
    virtual void handleEvent(const sf::Event& event) {}

    // For content only rendering
    virtual void renderContent() {
        render();
    }

    // Panel state
    bool isVisible() const { return m_isVisible; }
    void setVisible(bool visible) { m_isVisible = visible; }
    const std::string& getName() const { return m_name; }

protected:
    std::string m_name;
    bool m_isVisible;
};

} // namespace Riggle