#pragma once
#include <Riggle/Export/IExporter.h>
#include <Riggle/Character.h>
#include <memory>
#include <vector>

namespace Riggle {

class ExportManager {
public:
    ExportManager();
    ~ExportManager();

    // Register exporters
    void registerProjectExporter(std::unique_ptr<IProjectExporter> exporter);
    void registerAnimationExporter(std::unique_ptr<IAnimationExporter> exporter);

    // Get available exporters
    std::vector<IProjectExporter*> getProjectExporters() const;
    std::vector<IAnimationExporter*> getAnimationExporters() const;

    // Export operations
    bool exportProject(const Character& character, const std::string& projectName,
                      IProjectExporter* exporter, const std::string& outputPath);
    bool exportAnimation(const Character& character, const std::string& animationName,
                        IAnimationExporter* exporter, const std::string& outputPath);

    // Get last error from any operation
    std::string getLastError() const { return m_lastError; }

private:
    std::vector<std::unique_ptr<IProjectExporter>> m_projectExporters;
    std::vector<std::unique_ptr<IAnimationExporter>> m_animationExporters;
    std::string m_lastError;
};

}