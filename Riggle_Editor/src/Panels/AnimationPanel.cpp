#include "Editor/Panels/AnimationPanel.h"
#include "Riggle/Character.h"
#include "Riggle/Animation.h"
#include "Riggle/Rig.h"
#include "Riggle/Bone.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>

namespace Riggle {

AnimationPanel::AnimationPanel() 
    : BasePanel("Animation")
    , m_character(nullptr)
{
    m_state.pixelsPerSecond = 60.0f;
    m_state.zoom = 1.0f;
    m_state.currentTime = 0.0f;
}

void AnimationPanel::render() {
    if (!isVisible()) return;
    
    ImGui::Begin(getName().c_str(), &m_isVisible);
    
    if (!m_character) {
        ImGui::Text("No character selected");
        ImGui::End();
        return;
    }
    
    renderAnimationControls();
    ImGui::Separator();
    renderTimeline();
    
    ImGui::End();
    
    // Update playback
    if (m_state.isPlaying) {
        updatePlayback(ImGui::GetIO().DeltaTime);
    }
}

void AnimationPanel::update(sf::RenderWindow& window) {
    // Update animation state if needed
}

void AnimationPanel::renderAnimationControls() {
    auto* player = m_character->getAnimationPlayer();
    auto* currentAnim = player->getAnimation();
    
    // Animation selection
    ImGui::Text("Animation:");
    ImGui::SameLine();
    
    std::string currentAnimName = currentAnim ? currentAnim->getName() : "None";
    if (ImGui::BeginCombo("##AnimationSelect", currentAnimName.c_str())) {
        for (const auto& anim : m_character->getAnimations()) {
            bool isSelected = (currentAnim == anim.get());
            if (ImGui::Selectable(anim->getName().c_str(), isSelected)) {
                player->setAnimation(anim.get());
                player->setTime(m_state.currentTime);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("New")) {
        createNewAnimation();
        const auto& animations = m_character->getAnimations();
        if (!animations.empty()) {
            Animation* newAnim = animations.back().get();
            auto* player = m_character->getAnimationPlayer();
            player->setAnimation(newAnim); // Set as current animation
            strncpy(m_animNameBuffer, newAnim->getName().c_str(), sizeof(m_animNameBuffer));
            m_animToRename = newAnim;
            m_showAnimNameDialog = true;
            ImGui::OpenPopup("Set animation name");
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Delete") && currentAnim) {
        deleteAnimation(currentAnim->getName());
    }

    ImGui::SameLine();
    if (ImGui::Button("Rename") && currentAnim) {
        strncpy(m_animNameBuffer, currentAnim->getName().c_str(), sizeof(m_animNameBuffer));
        m_animToRename = currentAnim;
        m_showAnimNameDialog = true;
        ImGui::OpenPopup("Set animation name");
    }

    // Playback controls
    ImGui::Separator();
    
    // Recording section with better visual organization
    ImGui::BeginGroup();
    {
        // Record button with better visual feedback
        ImVec4 recordColor = m_state.isRecording ? ImVec4(0.9f, 0.1f, 0.1f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, recordColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_state.isRecording ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        
        if (ImGui::Button(m_state.isRecording ? "RECORDING" : "RECORD", ImVec2(120, 30))) {
            toggleRecording();
        }
        ImGui::PopStyleColor(2);
        
        // Recording status indicator
        if (m_state.isRecording) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "REC");
            
            // Blinking effect
            static float blinkTimer = 0.0f;
            blinkTimer += ImGui::GetIO().DeltaTime;
            if (fmod(blinkTimer, 1.0f) < 0.5f) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), " <o>");
            }
            else{
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.0f), " <o>");
            }
        }
        else {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 0.0f), "REC");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 0.0f), " <o>");
        }
        
        // Auto-key option
        ImGui::SameLine();
        ImGui::Checkbox("Auto-Key", &m_state.autoKeyframe);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Automatically create keyframes when bone transforms change");
        }
        
        // Key all bones button
        ImGui::SameLine();
        if (ImGui::Button("Key All")) {
            keyAllBones();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Create keyframes for all bones at current time");
        }
    }
    ImGui::EndGroup();
    
    // Show what's being recorded
    if (m_state.isRecording && currentAnim) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), 
                          "Recording to: %s (%.2fs)", 
                          currentAnim->getName().c_str(), 
                          m_state.currentTime);
        
        if (!m_state.selectedBone.empty()) {
            ImGui::Text("Active bone: %s", m_state.selectedBone.c_str());
        }
    }
    
    ImGui::Separator();
    
    // Standard playback controls in a row
    ImGui::BeginGroup();
    {
        if (ImGui::Button("To Start")) { // To start
            setCurrentTime(0.0f);
        }
        
        ImGui::SameLine();
        if (m_state.isPlaying) {
            if (ImGui::Button("Pause")) { // Pause
                m_state.isPlaying = false;
                player->pause();
            }
        } else {
            if (ImGui::Button("Play")) { // Play
                m_state.isPlaying = true;
                player->play();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Stop")) { // Stop
            m_state.isPlaying = false;
            setCurrentTime(0.0f);
            player->stop();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("To End") && currentAnim) { // To end
            setCurrentTime(currentAnim->getDuration());
        }
    }
    ImGui::EndGroup();
    
    // Time controls
    ImGui::Text("Time: %.3f", m_state.currentTime);
    ImGui::SameLine();
    
    float duration = currentAnim ? currentAnim->getDuration() : 10.0f;
    if (ImGui::SliderFloat("##TimeSlider", &m_state.currentTime, 0.0f, duration, "%.3f")) {
        setCurrentTime(m_state.currentTime);
    }
    
    // Playback speed and loop in same line
    ImGui::SliderFloat("Speed", &m_state.playbackSpeed, 0.1f, 3.0f, "%.1f");
    ImGui::SameLine();
    
    bool isLooping = player->isLooping();
    if (ImGui::Checkbox("Loop", &isLooping)) {
        player->setLooping(isLooping);
    }

    if (m_showAnimNameDialog) {
        m_showAnimNameDialog = false;
        ImGui::OpenPopup("Set animation name");
    }
    if (ImGui::BeginPopupModal("Set animation name", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Name", m_animNameBuffer, sizeof(m_animNameBuffer));
        if (ImGui::Button("OK")) {
            std::string name(m_animNameBuffer);
            if (m_animToRename && !name.empty()) {
                m_animToRename->setName(name);
            }
            m_animToRename = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_animToRename = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void AnimationPanel::renderTimeline() {
    auto* currentAnim = m_character->getAnimationPlayer()->getAnimation();
    if (!currentAnim) {
        ImGui::Text("No animation selected");
        return;
    }
    
    float animDuration = currentAnim->getDuration();
    float duration = std::max(currentAnim->getDuration(), 5.0f); // Minimum 5 seconds
    
    if (animDuration < 10.0f) {
        duration = 10.0f; // Show 10 seconds for easier editing
    }

    ImGui::BeginChild("Timeline", ImVec2(0, m_state.timelineHeight), true);
    
    // Handle zooming and scrolling with mouse wheel
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
        if (ImGui::GetIO().MouseWheel != 0.0f && !ImGui::IsAnyItemActive()) {
            if (ImGui::GetIO().KeyShift) {
                // Zoom
                float zoomFactor = 1.0f + ImGui::GetIO().MouseWheel * 0.1f;
                m_state.zoom = std::clamp(m_state.zoom * zoomFactor, 0.1f, 5.0f);
            } else if (ImGui::GetIO().KeyCtrl) {
                // Horizontal scroll
                m_state.scrollX -= ImGui::GetIO().MouseWheel * 50.0f;
                m_state.scrollX = std::max(0.0f, m_state.scrollX);
            }
        }
    }

    // Store the starting position for consistent layout
    ImVec2 timelineStart = ImGui::GetCursorScreenPos();
    
    renderTimelineHeader(timelineStart);
    renderTimelineRuler(duration, timelineStart);
    renderBoneTracks(currentAnim);
    renderPlayhead(duration, timelineStart);
    
    handleTimelineInteraction(duration);
    
    ImGui::EndChild();
    
    // Timeline controls
    ImGui::SliderFloat("Zoom", &m_state.zoom, 0.1f, 5.0f, "%.1f");
    ImGui::SameLine();
    if (ImGui::Button("Fit")) {
        // Auto-fit timeline to window
        float contentWidth = ImGui::GetContentRegionAvail().x - m_state.headerWidth;
        m_state.zoom = contentWidth / (duration * m_state.pixelsPerSecond);
        m_state.scrollX = 0.0f;
    }
}

void AnimationPanel::toggleRecording() {
    m_state.isRecording = !m_state.isRecording;
    
    if (m_state.isRecording) {
        // Enable manual bone edit mode during recording
        if (m_character) {
            m_character->setManualBoneEditMode(true);
        }

        std::cout << "Recording started" << std::endl;
        
        if (m_state.autoKeyframe && m_state.currentTime <= 0.0f) {
            keyAllBones();
        }

    } else {
        // Disable manual bone edit mode when stopping recording
        if (m_character) {
            m_character->setManualBoneEditMode(false);
        }
        std::cout << "Recording stopped" << std::endl;
    }
}

void AnimationPanel::keyAllBones() {
    if (!m_character) return;
    
    auto* rig = m_character->getRig();
    auto* currentAnim = m_character->getAnimationPlayer()->getAnimation();
    
    if (!rig || !currentAnim) return;
    
    const auto& allBones = rig->getAllBones();
    int keyframesAdded = 0;
    
    for (const auto& bone : allBones) {
        if (bone) {
            auto transform = bone->getLocalTransform();
            currentAnim->addKeyframe(bone->getName(), m_state.currentTime, transform);
            keyframesAdded++;
        }
    }
    
    std::cout << "Added keyframes for " << keyframesAdded << " bones at time " << m_state.currentTime << std::endl;
}

void AnimationPanel::renderTimelineHeader(ImVec2 timelineStart) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    float rulerHeight = 30.0f;
    
    // Header background - align with bone tracks
    ImVec2 headerStart = timelineStart;
    ImVec2 headerEnd = ImVec2(timelineStart.x + m_state.headerWidth, timelineStart.y + rulerHeight);
    drawList->AddRectFilled(headerStart, headerEnd, IM_COL32(60, 60, 60, 255));
    drawList->AddRect(headerStart, headerEnd, IM_COL32(100, 100, 100, 255));
    
    // Header text - center vertically in the header
    ImVec2 textPos = ImVec2(timelineStart.x + 5, timelineStart.y + 8);
    drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), "Bones");

    // Set cursor position for the ruler to be next to the header
    ImGui::SetCursorScreenPos(ImVec2(timelineStart.x, timelineStart.y + rulerHeight));
}

void AnimationPanel::renderTimelineRuler(float duration, ImVec2 timelineStart) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    float rulerHeight = 30.0f;
    float scaledPixelsPerSecond = m_state.pixelsPerSecond * m_state.zoom;
    
    // Position the invisible button properly for the ruler area only
    ImGui::SetCursorScreenPos(ImVec2(timelineStart.x + m_state.headerWidth, timelineStart.y));
    ImGui::InvisibleButton("##TimelineRuler", 
                          ImVec2(canvasSize.x - m_state.headerWidth, rulerHeight));
    
    // Check if ruler is being interacted with
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        float relativeX = mousePos.x - (timelineStart.x + m_state.headerWidth) + m_state.scrollX;
        float newTime = relativeX / scaledPixelsPerSecond;
        
        if (newTime >= 0 && newTime <= duration) {
            setCurrentTime(newTime);
        }
    }
    
    // Visual feedback when hovering over ruler
    if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        
        ImVec2 mousePos = ImGui::GetMousePos();
        float relativeX = mousePos.x - (timelineStart.x + m_state.headerWidth) + m_state.scrollX;
        float hoverTime = relativeX / scaledPixelsPerSecond;
        
        if (hoverTime >= 0 && hoverTime <= duration) {
            ImGui::SetTooltip("%.3f s", hoverTime);
        }
    }
    
    // Draw ruler background - aligned with timeline content, no gap
    ImVec2 rulerStart = ImVec2(timelineStart.x + m_state.headerWidth, timelineStart.y);
    ImVec2 rulerEnd = ImVec2(timelineStart.x + canvasSize.x, timelineStart.y + rulerHeight);
    drawList->AddRectFilled(rulerStart, rulerEnd, IM_COL32(50, 50, 50, 255));
    
    // Calculate visible time range based on scroll position
    float startTime = m_state.scrollX / scaledPixelsPerSecond;
    float endTime = (m_state.scrollX + canvasSize.x - m_state.headerWidth) / scaledPixelsPerSecond;
    
    // Better step calculation
    float pixelsPerStep = 80.0f; // Minimum pixels between major ticks
    float timePerStep = pixelsPerStep / scaledPixelsPerSecond;
    
    // Round to nice numbers
    float step = 1.0f;
    if (timePerStep <= 0.1f) step = 0.1f;
    else if (timePerStep <= 0.25f) step = 0.25f;
    else if (timePerStep <= 0.5f) step = 0.5f;
    else if (timePerStep <= 1.0f) step = 1.0f;
    else if (timePerStep <= 2.0f) step = 2.0f;
    else if (timePerStep <= 5.0f) step = 5.0f;
    else if (timePerStep <= 10.0f) step = 10.0f;
    else step = std::ceil(timePerStep / 10.0f) * 10.0f;
    
    // Draw time markers - account for scroll position
    float startTick = std::floor(startTime / step) * step;
    
    for (float time = startTick; time <= endTime + step; time += step) {
        // Calculate x position relative to ruler start, accounting for scroll
        float x = (time * scaledPixelsPerSecond) - m_state.scrollX;
        float screenX = timelineStart.x + m_state.headerWidth + x;
        
        // Only draw if visible within ruler area
        if (screenX >= rulerStart.x && screenX <= rulerEnd.x) {
            ImVec2 lineStart = ImVec2(screenX, timelineStart.y);
            ImVec2 lineEnd = ImVec2(screenX, timelineStart.y + 15);
            
            drawList->AddLine(lineStart, lineEnd, IM_COL32(200, 200, 200, 255), 2.0f);
            
            // Draw time labels
            char timeText[32];
            snprintf(timeText, sizeof(timeText), "%.1f", time);
            ImVec2 textPos = ImVec2(screenX + 2, timelineStart.y + 17);
            drawList->AddText(textPos, IM_COL32(200, 200, 200, 255), timeText);
        }
    }
    
    // Draw minor ticks
    float minorStep = step / 5.0f;
    for (float time = startTick; time <= endTime + step; time += minorStep) {
        float x = (time * scaledPixelsPerSecond) - m_state.scrollX;
        float screenX = timelineStart.x + m_state.headerWidth + x;
        
        if (screenX >= rulerStart.x && screenX <= rulerEnd.x) {
            ImVec2 lineStart = ImVec2(screenX, timelineStart.y);
            ImVec2 lineEnd = ImVec2(screenX, timelineStart.y + 8);
            
            drawList->AddLine(lineStart, lineEnd, IM_COL32(100, 100, 100, 255), 1.0f);
        }
    }
    
    // Draw ruler border
    drawList->AddRect(rulerStart, rulerEnd, IM_COL32(100, 100, 100, 255));

    // Move cursor down for bone tracks
    ImGui::SetCursorScreenPos(ImVec2(timelineStart.x, timelineStart.y + rulerHeight));
}

void AnimationPanel::renderBoneTracks(Animation* animation) {
    auto* rig = m_character->getRig();
    if (!rig) return;
    
    int trackIndex = 0;
    
    // Get all bones from the rig and render tracks for them
    const auto& allBones = rig->getAllBones();
    for (const auto& bone : allBones) {
        if (bone) {
            renderBoneTrack(bone->getName(), animation, trackIndex++);
        }
    }
}

void AnimationPanel::renderBoneTrack(const std::string& boneName, Animation* animation, int trackIndex) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    float trackY = canvasPos.y;
    bool isSelected = (m_state.selectedBone == boneName);
    
    // Track header
    ImVec2 headerStart = canvasPos;
    ImVec2 headerEnd = ImVec2(canvasPos.x + m_state.headerWidth, canvasPos.y + m_state.trackHeight);
    
    ImU32 headerColor = isSelected ? IM_COL32(80, 120, 160, 255) : IM_COL32(50, 50, 50, 255);
    drawList->AddRectFilled(headerStart, headerEnd, headerColor);
    drawList->AddRect(headerStart, headerEnd, IM_COL32(100, 100, 100, 255));
    
    // Bone name
    ImVec2 textPos = ImVec2(canvasPos.x + 5, canvasPos.y + 4);
    drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), boneName.c_str());
    
    // Track content area
    ImVec2 contentStart = ImVec2(canvasPos.x + m_state.headerWidth, canvasPos.y);
    ImVec2 contentEnd = ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + m_state.trackHeight);
    
    ImU32 contentColor = (trackIndex % 2 == 0) ? IM_COL32(40, 40, 40, 255) : IM_COL32(45, 45, 45, 255);
    drawList->AddRectFilled(contentStart, contentEnd, contentColor);
    drawList->AddRect(contentStart, contentEnd, IM_COL32(60, 60, 60, 255));
    
    // Render keyframes for this bone
    renderKeyframes(boneName, animation, trackY);
    
    // Handle bone selection
    ImGui::SetCursorScreenPos(canvasPos);
    ImGui::InvisibleButton(("track_" + boneName).c_str(), ImVec2(canvasSize.x, m_state.trackHeight));
    
    if (ImGui::IsItemClicked()) {
        m_state.selectedBone = boneName;
    }
    
    // Handle keyframe creation on double-click
    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0)) {
        if (m_state.isRecording) {
            addKeyframeAtCurrentTime(boneName, animation);
        }
    }
    
    ImGui::SetCursorScreenPos(ImVec2(canvasPos.x, canvasPos.y + m_state.trackHeight));
}

void AnimationPanel::renderKeyframes(const std::string& boneName, Animation* animation, float trackY) {
    auto* track = animation->getBoneTrack(boneName);
    if (!track) return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float scaledPixelsPerSecond = m_state.pixelsPerSecond * m_state.zoom;
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    for (const auto& keyframe : track->getKeyframes()) {
        // Calculate x position accounting for scroll
        float x = (keyframe.time * scaledPixelsPerSecond) - m_state.scrollX;
        float screenX = canvasPos.x + m_state.headerWidth + x;

        // Only draw if visible
        if (screenX >= canvasPos.x + m_state.headerWidth && screenX <= canvasPos.x + canvasSize.x) {
            ImVec2 center = ImVec2(screenX, trackY + m_state.trackHeight * 0.5f);

            bool isSelected = (m_state.selectedBone == boneName &&
                               std::abs(m_state.selectedKeyframeTime - keyframe.time) < 0.001f);

            ImU32 keyframeColor = isSelected ? IM_COL32(255, 255, 100, 255) : IM_COL32(100, 200, 100, 255);

            // Diamond shape keyframe
            ImVec2 points[4] = {
                ImVec2(center.x, center.y - 6),  // Top
                ImVec2(center.x + 6, center.y),  // Right
                ImVec2(center.x, center.y + 6),  // Bottom
                ImVec2(center.x - 6, center.y)   // Left
            };

            drawList->AddConvexPolyFilled(points, 4, keyframeColor);
            drawList->AddPolyline(points, 4, IM_COL32(255, 255, 255, 255), true, 1.0f);

            // --- Add hit detection and double-click for deletion ---
            ImVec2 min = ImVec2(center.x - 7, center.y - 7);
            ImVec2 max = ImVec2(center.x + 7, center.y + 7);
            ImGui::SetCursorScreenPos(min);
            ImGui::InvisibleButton(("keyframe_" + boneName + "_" + std::to_string(keyframe.time)).c_str(), ImVec2(14, 14));

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Time: %.3f\nDouble-click to delete", keyframe.time);
            }

            if (ImGui::IsItemClicked()) {
                m_state.selectedBone = boneName;
                m_state.selectedKeyframeTime = keyframe.time;
            }

            if (ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0)) {
                // Double-click: delete this keyframe
                animation->removeKeyframe(boneName, keyframe.time);
                // Optionally clear selection if deleted
                if (m_state.selectedBone == boneName && std::abs(m_state.selectedKeyframeTime - keyframe.time) < 0.001f) {
                    m_state.selectedKeyframeTime = -1.0f;
                }
                break; // Keyframes list may be invalid after deletion
            }
        }
    }
}

void AnimationPanel::renderPlayhead(float duration, ImVec2 timelineStart) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    
    float scaledPixelsPerSecond = m_state.pixelsPerSecond * m_state.zoom;
    float x = (m_state.currentTime * scaledPixelsPerSecond) - m_state.scrollX;
    float screenX = timelineStart.x + m_state.headerWidth + x;
    
    if (screenX >= timelineStart.x + m_state.headerWidth && screenX <= timelineStart.x + canvasSize.x) {
        // Calculate the ruler position
        float rulerHeight = 30.0f;
        float rulerY = timelineStart.y;
        
        // Draw playhead line from ruler through all tracks
        ImVec2 lineStart = ImVec2(screenX, rulerY);
        ImVec2 lineEnd = ImVec2(screenX, timelineStart.y + m_state.timelineHeight - 60); // End at bottom of timeline
        
        drawList->AddLine(lineStart, lineEnd, IM_COL32(255, 100, 100, 255), 2.0f);
        
        // Playhead handle on ruler
        ImVec2 handleCenter = ImVec2(screenX, rulerY + 15);
        drawList->AddTriangleFilled(
            ImVec2(handleCenter.x, handleCenter.y + 8),
            ImVec2(handleCenter.x - 6, handleCenter.y - 4),
            ImVec2(handleCenter.x + 6, handleCenter.y - 4),
            IM_COL32(255, 100, 100, 255)
        );
    }
}

void AnimationPanel::handleTimelineInteraction(float duration) {
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    
    // Define ruler area (the time bar above the bone tracks)
    float rulerHeight = 30.0f;
    ImVec2 rulerStart = ImVec2(canvasPos.x + m_state.headerWidth, canvasPos.y - rulerHeight);
    ImVec2 rulerEnd = ImVec2(canvasPos.x + ImGui::GetContentRegionAvail().x, canvasPos.y);
    
    // Define timeline content area (bone tracks area)
    ImVec2 contentStart = ImVec2(canvasPos.x + m_state.headerWidth, canvasPos.y);
    ImVec2 contentEnd = ImVec2(canvasPos.x + ImGui::GetContentRegionAvail().x, canvasPos.y + m_state.timelineHeight);
    
    // Check if mouse is in ruler area (priority for time scrubbing)
    bool isInRuler = (mousePos.x >= rulerStart.x && mousePos.x <= rulerEnd.x &&
                     mousePos.y >= rulerStart.y && mousePos.y <= rulerEnd.y);
    
    // Check if mouse is in content area (for keyframe operations)
    bool isInContent = (mousePos.x >= contentStart.x && mousePos.x <= contentEnd.x &&
                       mousePos.y >= contentStart.y && mousePos.y <= contentEnd.y);
    
    // Must be hovering the animation panel window
    bool canInteract = (isInRuler || isInContent) && ImGui::IsWindowHovered();
    
    if (!canInteract) {
        return;
    }

    // Handle time scrubbing - PRIORITY for ruler area, also works in content area
    if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemActive()) {
        // Calculate relative X position from the start of the timeline (after header)
        float relativeX = mousePos.x - (canvasPos.x + m_state.headerWidth) + m_state.scrollX;
        
        if (relativeX >= 0) {
            float scaledPixelsPerSecond = m_state.pixelsPerSecond * m_state.zoom;
            float newTime = relativeX / scaledPixelsPerSecond;
            
            if (newTime >= 0 && newTime <= duration) {
                setCurrentTime(newTime);
                std::cout << "Timeline scrubbed to time: " << newTime << std::endl;
                
                // If in ruler area, prioritize time scrubbing over other interactions
                if (isInRuler) {
                    return; // Don't process other interactions
                }
            }
        }
    }
}

void AnimationPanel::addKeyframeAtCurrentTime(const std::string& boneName, Animation* animation) {
    if (!m_character || !animation) return;
    
    auto* rig = m_character->getRig();
    if (!rig) return;
    
    auto bone = rig->findBone(boneName);  // This returns shared_ptr
    if (!bone) return;
    
    // Get current bone transform
    auto transform = bone->getLocalTransform();
    
    // Add keyframe
    animation->addKeyframe(boneName, m_state.currentTime, transform);
    
    std::cout << "Added keyframe for bone '" << boneName << "' at time " << m_state.currentTime << std::endl;
}

void AnimationPanel::createKeyframeForBone(const std::string& boneName) {
    if (!m_character) {
        std::cout << "No character available for keyframing" << std::endl;
        return;
    }
    auto* currentAnim = m_character->getAnimationPlayer()->getAnimation();
    if (currentAnim && m_character && m_character->getRig()) {
        auto bone = m_character->getRig()->findBone(boneName);
        if (bone) {
            addKeyframeAtCurrentTime(boneName, currentAnim);
        }
    }
}

void AnimationPanel::setCurrentTime(float time) {
    // Playhead snapping logic: Snap to time step
    float scaledPixelsPerSecond = m_state.pixelsPerSecond * m_state.zoom;
    float pixelsPerStep = 80.0f;
    float timePerStep = pixelsPerStep / scaledPixelsPerSecond;
    float step = 1.0f;
    if (timePerStep <= 0.1f) step = 0.1f;
    else if (timePerStep <= 0.25f) step = 0.25f;
    else if (timePerStep <= 0.5f) step = 0.5f;
    else if (timePerStep <= 1.0f) step = 1.0f;
    else if (timePerStep <= 2.0f) step = 2.0f;
    else if (timePerStep <= 5.0f) step = 5.0f;
    else if (timePerStep <= 10.0f) step = 10.0f;
    else step = std::ceil(timePerStep / 10.0f) * 10.0f;

    float minorStep = step / 5.0f;

    // Snap time to nearest minor step
    float snappedTime = std::round(time / minorStep) * minorStep;
    if (snappedTime < 0.0f) snappedTime = 0.0f;
    m_state.currentTime = snappedTime;

    // DISABLE manual edit mode during playhead updates
    bool wasInManualMode = false;
    if (m_character && m_character->isInManualBoneEditMode()) {
        wasInManualMode = true;
        m_character->setManualBoneEditMode(false);
    }

    // Apply animation at current time
    if (m_character && m_character->getAnimationPlayer()) {
        auto* currentAnim = m_character->getAnimationPlayer()->getAnimation();
        if (currentAnim) {
            m_character->getAnimationPlayer()->setTime(m_state.currentTime);
            m_character->getAnimationPlayer()->update(0.0f); // Force update
        }
    }

    // RESTORE manual edit mode if it was enabled
    if (wasInManualMode && m_character) {
        m_character->setManualBoneEditMode(true);
    }
}

void AnimationPanel::applyLastKeyframeToAllBones(Animation* animation) {
    if (!m_character || !animation) return;
    
    auto* rig = m_character->getRig();
    const auto& allBones = rig->getAllBones();
    
    for (const auto& bone : allBones) {
        if (bone) {
            // Get the last keyframe for this bone
            auto lastTransform = animation->getLastKeyframeTransform(bone->getName());
            if (lastTransform.has_value()) {
                bone->setLocalTransform(lastTransform.value());
            }
        }
    }
    
    // Force update transforms
    rig->forceUpdateWorldTransforms();
    m_character->forceUpdateDeformations();
}

void AnimationPanel::createNewAnimation() {
    if (!m_character) return;

    // Find next available name
    int animCounter = 1;
    std::string name;
    bool nameExists = true;
    do {
        name = "Animation_" + std::to_string(animCounter++);
        nameExists = false;
        for (const auto& anim : m_character->getAnimations()) {
            if (anim && anim->getName() == name) {
                nameExists = true;
                break;
            }
        }
    } while (nameExists);

    auto animation = std::make_unique<Animation>(name);
    m_character->addAnimation(std::move(animation));

    std::cout << "Created new animation: " << name << std::endl;
}

void AnimationPanel::deleteAnimation(const std::string& name) {
    if (!m_character) return;

    // CRITICAL: Check if we're deleting the currently active animation
    auto* player = m_character->getAnimationPlayer();
    auto* currentAnim = player->getAnimation();
    
    if (currentAnim && currentAnim->getName() == name) {
        // Stop playback and clear current animation BEFORE deleting
        player->stop();
        player->setAnimation(nullptr);
        m_state.isPlaying = false;
        m_state.isRecording = false;
        std::cout << "Cleared current animation before deletion" << std::endl;
    }
    
    // Clear any UI selections related to this animation
    if (m_state.selectedBone.empty() == false) {
        // Reset any selected keyframes that might reference the deleted animation
        m_state.selectedKeyframeTime = -1.0f;
    }
    
    m_character->removeAnimation(name);
    std::cout << "Deleted animation: " << name << std::endl;
}

void AnimationPanel::updatePlayback(float deltaTime) {
    if (!m_character) return;
    
    auto* player = m_character->getAnimationPlayer();
    auto* currentAnim = player->getAnimation();
    
    if (currentAnim) {
        float scaledDelta = deltaTime * m_state.playbackSpeed;
        m_state.currentTime += scaledDelta;
        
        float duration = currentAnim->getDuration();
        if (m_state.currentTime > duration) {
            if (player->isLooping()) {
                m_state.currentTime = 0.0f;
            } else {
                m_state.currentTime = duration;
                m_state.isPlaying = false;
            }
        }
        
        player->setTime(m_state.currentTime);
    }
}

float AnimationPanel::timeToPixel(float time) const {
    return time * m_state.pixelsPerSecond * m_state.zoom;
}

float AnimationPanel::pixelToTime(float pixel) const {
    return pixel / (m_state.pixelsPerSecond * m_state.zoom);
}

void AnimationPanel::handleKeyframeInteraction(const std::string& boneName, Animation* animation) {
    // TODO: Implement keyframe selection and manipulation
}

void AnimationPanel::deleteSelectedKeyframe(Animation* animation) {
    if (m_state.selectedBone.empty() || m_state.selectedKeyframeTime < 0) return;
    
    animation->removeKeyframe(m_state.selectedBone, m_state.selectedKeyframeTime);
    m_state.selectedKeyframeTime = -1.0f;
}

void AnimationPanel::moveKeyframe(const std::string& boneName, Animation* animation, float oldTime, float newTime) {
    // TODO: Implement keyframe moving
}

ImVec2 AnimationPanel::getTimelineContentStart() const {
    return ImVec2(m_state.headerWidth, 60); // After header and ruler
}

ImVec2 AnimationPanel::getTimelineContentSize() const {
    ImVec2 available = ImGui::GetContentRegionAvail();
    return ImVec2(available.x - m_state.headerWidth, m_state.timelineHeight - 60);
}

bool AnimationPanel::isMouseInTimeline() const {
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 contentStart = getTimelineContentStart();
    ImVec2 contentSize = getTimelineContentSize();
    
    return mousePos.x >= contentStart.x && 
           mousePos.x <= contentStart.x + contentSize.x &&
           mousePos.y >= contentStart.y && 
           mousePos.y <= contentStart.y + contentSize.y;
}

} // namespace Riggle