#include "Editor/EditorApplication.h"
#include "Editor/EditorController.h"
#include <imgui.h>
#include <imgui-SFML.h>
#include <iostream>
#include <fstream>

namespace Riggle {

EditorApplication::EditorApplication()
    : m_showStartup(true)
    , m_showAboutDialog(false)
    , m_window(sf::VideoMode({500, 400}), "Riggle - 2D Skeletal Animation Tool", sf::Style::Titlebar | sf::Style::Close)
    , m_editorController(nullptr)
    , m_logoLoaded(false)
{
    m_window.setFramerateLimit(60);
    
    loadWindowState();

    // Center the startup window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::Vector2u windowSize = m_window.getSize();
    int x = (desktop.size.x - windowSize.x) / 2;
    int y = (desktop.size.y - windowSize.y) / 2;
    m_window.setPosition(sf::Vector2i(x, y));

    m_logoLoaded = m_logoTexture.loadFromFile("Riggle_Editor/Assets/Riggle_logo.png");
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

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.122f, 0.337f, 0.235f, 1.0f);

    style.Colors[ImGuiCol_Button] = ImVec4(0.122f, 0.337f, 0.235f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.137f, 0.604f, 0.412f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.129f, 0.702f, 0.475f, 1.0f);

    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.086f, 0.176f, 0.129f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.129f, 0.424f, 0.294f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.137f, 0.514f, 0.353f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.129f, 0.702f, 0.475f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 1.0f, 0.6667f, 1.0f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);

    style.Colors[ImGuiCol_Header] = ImVec4(0.122f, 0.337f, 0.235f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.137f, 0.604f, 0.412f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.137f, 0.514f, 0.353f, 1.0f);

    style.Colors[ImGuiCol_Tab] = ImVec4(0.106f, 0.255f, 0.180f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.137f, 0.604f, 0.412f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.137f, 0.514f, 0.353f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.106f, 0.255f, 0.180f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.129f, 0.424f, 0.294f, 1.0f);
    style.Colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.137f, 0.514f, 0.353f, 1.0f);

    style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    style.Colors[ImGuiCol_Border] = ImVec4(0.122f, 0.337f, 0.235f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered]  = ImVec4(0.129f, 0.702f, 0.475f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive]   = ImVec4(0.0f, 1.0f, 0.667f, 1.0f);

    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.122f, 0.337f, 0.235f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.129f, 0.702f, 0.475f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.0f, 1.0f, 0.6667f, 1.0f);

    style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.0f, 1.0f, 0.667f, 0.3f);
    style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.086f, 0.176f, 0.129f, 1.0f);

    sf::Vector2i prevPosition = m_window.getPosition();

    // Main loop
    while (m_window.isOpen()) {
        // Event handling 
        while (const std::optional<sf::Event> event = m_window.pollEvent()) {
            ImGui::SFML::ProcessEvent(m_window, *event);
            
            if (event->is<sf::Event::Closed>()) {
                if (m_editorController) {
                    m_editorController->requestExit();
                    saveWindowState();
                } else {
                    m_window.close();
                }
            }
            
            // Pass events to editor if it exists
            if (m_editorController) {
                m_editorController->handleEvent(*event);
            }
        }

        if (!m_showStartup) {
            sf::Vector2u size = m_window.getSize();
            sf::Vector2i currPosition = m_window.getPosition();
            unsigned int minWidth = 640, minHeight = 480;

            bool positionChangedX = (currPosition.x != prevPosition.x);
            bool positionChangedY = (currPosition.y != prevPosition.y);

            if (size.x < minWidth || size.y < minHeight) {
                sf::Vector2i newPosition = currPosition;

                if (size.x < minWidth && positionChangedX) {
                    int dx = (int)minWidth - (int)size.x;
                    newPosition.x -= dx;
                }
                if (size.y < minHeight && positionChangedY) {
                    int dy = (int)minHeight - (int)size.y;
                    newPosition.y -= dy;
                }

                m_window.setSize({std::max(size.x, minWidth), std::max(size.y, minHeight)});
                m_window.setPosition(newPosition);
            }
            prevPosition = m_window.getPosition();
        }

        ImGui::SFML::Update(m_window, m_deltaClock.restart());
        
        // Update editor if it exists
        if (m_editorController) {
            m_editorController->update(m_window);
            
            // Check if editor wants to exit
            if (m_editorController->shouldExit()) {
                saveWindowState();
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
            renderDockSpace();
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

void EditorApplication::renderDockSpace() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags dockspace_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
                                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("DockSpace", nullptr, dockspace_flags);

    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

    // Check if layout reset is requested
    if (m_editorController && m_editorController->isLayoutResetRequested()) {
        m_editorController->setupInitialDockLayout(dockspace_id);
        m_editorController->clearLayoutResetRequest();
    }

    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    ImGui::End();
}

void EditorApplication::renderStartupWindow() {
    ImVec2 windowSize(420, 340);
    ImVec2 windowPos((m_window.getSize().x - windowSize.x) * 0.5f, 
                     (m_window.getSize().y - windowSize.y) * 0.5f);
    
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | 
                            ImGuiWindowFlags_NoMove | 
                            ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar;
    
    if (ImGui::Begin("Startup", nullptr, flags)) {
        if (m_logoLoaded) {
        float logoWidth = 64.0f;
        float logoHeight = 64.0f;
        float centerX = (windowSize.x - logoWidth) * 0.5f;
        ImGui::SetCursorPosX(centerX);
        ImTextureID logoTexId = (ImTextureID)(intptr_t)(m_logoTexture.getNativeHandle());
        ImGui::Image(logoTexId, ImVec2(logoWidth, logoHeight));
        ImGui::Spacing();
    }

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        const char* title = "Riggle";
        float titleWidth = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((windowSize.x - titleWidth) * 0.5f);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.6667f, 1.0f), "%s", title);
        ImGui::PopFont();

        // Subtitle
        const char* subtitle = "2D Skeletal Animation Tool";
        float subtitleWidth = ImGui::CalcTextSize(subtitle).x;
        ImGui::SetCursorPosX((windowSize.x - subtitleWidth) * 0.5f);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", subtitle);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        // Start Editor Button
        float buttonWidth = 250.0f;
        float buttonHeight = 44.0f;
        float buttonX = (windowSize.x - buttonWidth) * 0.5f;
        ImGui::SetCursorPosX(buttonX);
        if (ImGui::Button("Start Editor", ImVec2(buttonWidth, buttonHeight))) {
            startEditor();
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        // About Button
        ImGui::SetCursorPosX(buttonX);
        if (ImGui::Button("About Riggle", ImVec2(buttonWidth, 36.0f))) {
            m_showAboutDialog = true;
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        // Exit Button
        float exitButtonWidth = 100.0f;
        float exitButtonX = (windowSize.x - exitButtonWidth) * 0.5f;
        ImGui::SetCursorPosX(exitButtonX);
        if (ImGui::Button("Exit", ImVec2(exitButtonWidth, 30.0f))) {
            m_window.close();
        }

        // Version and credits at the bottom
        ImGui::SetCursorPosY(windowSize.y - 40);
        ImGui::Separator();
        ImGui::SetCursorPosX((windowSize.x - 200) * 0.5f);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Version 1.0.0 | © 2025 Riggle Team");
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
        ImGui::Text("Developed by:");
        ImGui::BulletText("Bipul Gautam");
        ImGui::BulletText("Bishal Khatiwada");
        ImGui::BulletText("Bishal Rimal");
        ImGui::Spacing();
        
        if (ImGui::Button("Close")) {
            m_showAboutDialog = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void EditorApplication::startEditor() {
    // Check if the ImGui settings file exists BEFORE creating the main editor.
    std::ifstream imguiIniFile("imgui.ini");
    bool applyDefaultLayout = !imguiIniFile.good();

    // Create the main editor window
    m_window.create(sf::VideoMode(m_savedWindowSize), "Riggle - 2D Skeletal Animation Tool", sf::Style::Resize | sf::Style::Titlebar | sf::Style::Close);
    m_window.setPosition(m_savedWindowPosition);
    m_window.setFramerateLimit(60);

    m_editorController = std::make_unique<EditorController>();

    if (applyDefaultLayout) {
        m_editorController->requestLayoutReset();
    }

    m_showStartup = false;
}

void EditorApplication::loadWindowState() {
    std::ifstream in("window_state.ini");
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    unsigned int w = 1280, h = 720;
    int x = -1, y = -1;
    bool valid = false;

    if (in) {
        in >> w >> h >> x >> y;
        // Validate size and position
        if (!in.fail() &&
            w >= 640 && h >= 480 &&
            w <= desktop.size.x && h <= desktop.size.y &&
            x >= -1000 && y >= -1000 &&
            x + (int)w <= (int)desktop.size.x &&
            y + (int)h <= (int)desktop.size.y) {
            valid = true;
        }
    }

    if (!valid) {
        w = 1280; h = 720;
        x = (desktop.size.x - w) / 2;
        y = (desktop.size.y - h) / 2;
    }

    m_savedWindowSize = sf::Vector2u(w, h);
    m_savedWindowPosition = sf::Vector2i(x, y);
}

void EditorApplication::saveWindowState() {
    std::ofstream out("window_state.ini");
    if (out) {
        sf::Vector2u size = m_window.getSize();
        sf::Vector2i position = m_window.getPosition();
        out << size.x << " " << size.y << " " << position.x << " " << position.y << std::endl;
    }
}

} // namespace Riggle