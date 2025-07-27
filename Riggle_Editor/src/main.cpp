#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <Editor/EditorController.h>
#include <iostream>

int main() {
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Riggle - 2D Skeletal Animation Tool", sf::State::Fullscreen);
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML" << std::endl;
        return -1;
    }

    // Create editor controller
    Riggle::EditorController editor;

    sf::Clock deltaClock;
    while (window.isOpen()) {
        // SFML 3.0 event handling
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            
            // Window closed: request exit through editor
            if (event->is<sf::Event::Closed>()) {
                editor.requestExit();
            }
            
            // Pass events to editor
            editor.handleEvent(*event);
        }

        ImGui::SFML::Update(window, deltaClock.restart());
        
        // Update editor
        editor.update(window);

        // Check if editor wants to exit
        if (editor.shouldExit()) {
            window.close();
            break;
        }

        // Clear and render
        window.clear(sf::Color::Black);
        
        // Render editor content
        editor.render(window);
        
        // Render ImGui
        ImGui::SFML::Render(window);
        
        window.display();
    }

    std::cout << "Riggle Editor shutting down ..." << std::endl;
    ImGui::SFML::Shutdown();
    return 0;
}