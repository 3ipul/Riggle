#include "Editor/Export/ExportManager.h"
#include <Riggle/Export/ExportService.h>

namespace Riggle {

ExportManager::ExportManager() = default;
ExportManager::~ExportManager() = default;

void ExportManager::registerProjectExporter(std::unique_ptr<IProjectExporter> exporter) {
    if (exporter) {
        m_projectExporters.push_back(std::move(exporter));
    }
}

void ExportManager::registerAnimationExporter(std::unique_ptr<IAnimationExporter> exporter) {
    if (exporter) {
        m_animationExporters.push_back(std::move(exporter));
    }
}

std::vector<IProjectExporter*> ExportManager::getProjectExporters() const {
    std::vector<IProjectExporter*> exporters;
    exporters.reserve(m_projectExporters.size());
    
    for (const auto& exporter : m_projectExporters) {
        exporters.push_back(exporter.get());
    }
    
    return exporters;
}

std::vector<IAnimationExporter*> ExportManager::getAnimationExporters() const {
    std::vector<IAnimationExporter*> exporters;
    exporters.reserve(m_animationExporters.size());
    
    for (const auto& exporter : m_animationExporters) {
        exporters.push_back(exporter.get());
    }
    
    return exporters;
}

bool ExportManager::exportProject(const Character& character, const std::string& projectName,
                                 IProjectExporter* exporter, const std::string& outputPath) {
    if (!exporter) {
        m_lastError = "No exporter provided";
        return false;
    }

    try {
        // Extract project data using core service
        ExportProject projectData = ExportService::extractProjectData(character, projectName);
        
        // Export using the provided exporter
        bool success = exporter->exportProject(projectData, outputPath);
        if (!success) {
            m_lastError = "Export failed: " + exporter->getLastError();
        }
        
        return success;
    }
    catch (const std::exception& e) {
        m_lastError = "Exception during project export: " + std::string(e.what());
        return false;
    }
}

bool ExportManager::exportAnimation(const Character& character, const std::string& animationName,
                                   IAnimationExporter* exporter, const std::string& outputPath) {
    if (!exporter) {
        m_lastError = "No exporter provided";
        return false;
    }

    try {
        // Find the specific animation by name
        Animation* targetAnimation = character.findAnimation(animationName);
        if (!targetAnimation) {
            m_lastError = "Animation '" + animationName + "' not found";
            return false;
        }
        
        // Extract data using core service
        ExportAnimation animationData = ExportService::extractAnimationData(*targetAnimation);
        std::vector<ExportSprite> sprites = ExportService::extractSpriteData(character.getSprites());
        std::vector<ExportBone> bones;
        
        if (character.getRig()) {
            bones = ExportService::extractBoneData(*character.getRig());
        }
        
        // Export using the provided exporter
        bool success = exporter->exportAnimation(animationData, sprites, bones, outputPath);
        if (!success) {
            m_lastError = "Export failed: " + exporter->getLastError();
        }
        
        return success;
    }
    catch (const std::exception& e) {
        m_lastError = "Exception during animation export: " + std::string(e.what());
        return false;
    }
}

}