#pragma once

#include <SFML/Graphics.hpp>
#include <memory>

namespace Riggle {

class EditorController;

class EditorApplication {
public:
    EditorApplication();
    ~EditorApplication();

    int run();

private:
    // Simple state
    bool m_showStartup;
    bool m_showAboutDialog;
    
    // SFML objects
    sf::RenderWindow m_window;
    sf::Clock m_deltaClock;

    sf::Texture m_logoTexture;
    bool m_logoLoaded = false;

    sf::Vector2u m_savedWindowSize;
    sf::Vector2i m_savedWindowPosition;
    bool m_windowStateLoaded = false;
    
    // Editor controller
    std::unique_ptr<EditorController> m_editorController;
    
    // Methods
    void renderDockSpace();
    void renderStartupWindow();
    void startEditor();

    void loadWindowState();
    void saveWindowState();
};

} // namespace Riggle