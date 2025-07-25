#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include "Editor/EditorController.h"

int main() {
    sf::RenderWindow window(sf::VideoMode({1200u, 800u}), "Riggle - 2D Skeletal Animation Tool");
    window.setFramerateLimit(60);
    
    if (!ImGui::SFML::Init(window)) {
        return -1;
    }

    // Create editor controller
    Riggle::EditorController editor;

    sf::Clock deltaClock;
    while (window.isOpen()) {
        // SFML 3.0 event handling
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            
            // Window closed or escape key pressed: exit
            if (event->is<sf::Event::Closed>() ||
                (event->is<sf::Event::KeyPressed>() &&
                 event->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Escape)) {
                window.close();
            }
            
            // Pass events to editor
            editor.handleEvent(*event);
        }

        ImGui::SFML::Update(window, deltaClock.restart());
        
        // Update editor
        editor.update(window);

        // Clear and render
        window.clear(sf::Color::Black);
        
        // Render editor content
        editor.render(window);
        
        // Render ImGui
        ImGui::SFML::Render(window);
        
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}