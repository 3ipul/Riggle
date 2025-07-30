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
    
    // SFML objects (not pointers, just like your working main)
    sf::RenderWindow m_window;
    sf::Clock m_deltaClock;
    
    // Editor controller (unique_ptr since it's created later)
    std::unique_ptr<EditorController> m_editorController;
    
    // Methods
    void renderStartupWindow();
    void startEditor();
};

} // namespace Riggle