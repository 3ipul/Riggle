#include "Editor/EditorApplication.h"
#include "Editor/EditorController.h"
#include <imgui.h>
#include <imgui-SFML.h>
#include <iostream>

namespace Riggle {

EditorApplication::EditorApplication()
    : m_showStartup(true)
    , m_showAboutDialog(false)
    , m_window(sf::VideoMode({500, 400}), "Riggle - 2D Skeletal Animation Tool", sf::Style::Titlebar | sf::Style::Close)
    , m_editorController(nullptr)
{
    m_window.setFramerateLimit(60);
    
    // Center the startup window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::Vector2u windowSize = m_window.getSize();
    int x = (desktop.size.x - windowSize.x) / 2;
    int y = (desktop.size.y - windowSize.y) / 2;
    m_window.setPosition(sf::Vector2i(x, y));
}

EditorApplication::~EditorApplication() {
    if (m_window.isOpen()) {
        ImGui::SFML::Shutdown();
    }
}

int EditorApplication::run() {
    // Initialize ImGui
    if (!ImGui::SFML::Init(m_window)) {
        std::cerr << "Failed to initialize ImGui-SFML" << std::endl;
        return -1;
    }

    // Main loop
    while (m_window.isOpen()) {
        // Event handling 
        while (const std::optional<sf::Event> event = m_window.pollEvent()) {
            ImGui::SFML::ProcessEvent(m_window, *event);
            
            if (event->is<sf::Event::Closed>()) {
                if (m_editorController) {
                    m_editorController->requestExit();
                } else {
                    m_window.close();
                }
            }
            
            // Pass events to editor if it exists
            if (m_editorController) {
                m_editorController->handleEvent(*event);
            }
        }

        ImGui::SFML::Update(m_window, m_deltaClock.restart());
        
        // Update editor if it exists
        if (m_editorController) {
            m_editorController->update(m_window);
            
            // Check if editor wants to exit
            if (m_editorController->shouldExit()) {
                m_window.close();
                break;
            }
        }

        // Clear and render
        m_window.clear(sf::Color::Black);
        
        // Render startup or editor
        if (m_showStartup) {
            renderStartupWindow();
        } else if (m_editorController) {
            // Render editor content
            m_editorController->render(m_window);
        }
        
        // Render ImGui
        ImGui::SFML::Render(m_window);
        
        m_window.display();
    }

    std::cout << "Riggle Editor shutting down ..." << std::endl;
    ImGui::SFML::Shutdown();
    return 0;
}

void EditorApplication::renderStartupWindow() {
    ImVec2 windowSize(400, 300);
    ImVec2 windowPos((m_window.getSize().x - windowSize.x) * 0.5f, 
                     (m_window.getSize().y - windowSize.y) * 0.5f);
    
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | 
                            ImGuiWindowFlags_NoMove | 
                            ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar;
    
    if (ImGui::Begin("Startup", nullptr, flags)) {
        // Title
        const char* title = "Welcome to Riggle";
        float titleWidth = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((windowSize.x - titleWidth) * 0.5f);
        ImGui::Text("%s", title);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Subtitle
        const char* subtitle = "2D Skeletal Animation Tool";
        float subtitleWidth = ImGui::CalcTextSize(subtitle).x;
        ImGui::SetCursorPosX((windowSize.x - subtitleWidth) * 0.5f);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", subtitle);
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        // Buttons
        float buttonWidth = 250.0f;
        float buttonHeight = 40.0f;
        float buttonX = (windowSize.x - buttonWidth) * 0.5f;
        
        ImGui::SetCursorPosX(buttonX);
        if (ImGui::Button("Create New Project", ImVec2(buttonWidth, buttonHeight))) {
            startEditor();
        }
        
        ImGui::Spacing();
        
        ImGui::SetCursorPosX(buttonX);
        if (ImGui::Button("Open Existing Project", ImVec2(buttonWidth, buttonHeight))) {
            std::cout << "Project opening not implemented yet - coming soon!" << std::endl;
        }
        
        ImGui::Spacing();
        
        ImGui::SetCursorPosX(buttonX);
        if (ImGui::Button("About Riggle", ImVec2(buttonWidth, buttonHeight))) {
            m_showAboutDialog = true;
        }
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        float exitButtonWidth = 100.0f;
        float exitButtonX = (windowSize.x - exitButtonWidth) * 0.5f;
        ImGui::SetCursorPosX(exitButtonX);
        if (ImGui::Button("Exit", ImVec2(exitButtonWidth, 30.0f))) {
            m_window.close();
        }
    }
    ImGui::End();
    
    // About dialog
    if (m_showAboutDialog) {
        ImGui::OpenPopup("About Riggle");
    }
    
    if (ImGui::BeginPopupModal("About Riggle", &m_showAboutDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Riggle - 2D Skeletal Animation Tool");
        ImGui::Separator();
        ImGui::Text("Version: 1.0.0");
        ImGui::Text("A lightweight, open-source 2D skeletal animation tool");
        ImGui::Spacing();
        ImGui::Text("Features:");
        ImGui::BulletText("Forward Kinematics (FK)");
        ImGui::BulletText("Inverse Kinematics (IK)");
        ImGui::BulletText("Physics simulation");
        ImGui::Spacing();
        
        if (ImGui::Button("Close")) {
            m_showAboutDialog = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void EditorApplication::startEditor() {
    // Resize window to fullscreen
    m_window.create(sf::VideoMode::getDesktopMode(), "Riggle - 2D Skeletal Animation Tool");
    m_window.setFramerateLimit(60);
    
    // Create editor controller
    m_editorController = std::make_unique<EditorController>();
    
    // Hide startup window
    m_showStartup = false;
}

} // namespace Riggle