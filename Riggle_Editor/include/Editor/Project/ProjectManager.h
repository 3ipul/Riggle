#pragma once
#include <Riggle/Character.h>
#include "Editor/Utils/json.hpp"
#include <string>
#include <memory>
#include <vector>

using json = nlohmann::json;

namespace Riggle {

struct ProjectMetadata {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::string createdDate;
    std::string modifiedDate;
};

class ProjectManager {
public:
    ProjectManager();
    ~ProjectManager() = default;
    
    // Project operations
    bool saveProject(const Character& character, const std::string& filePath, 
                    const ProjectMetadata& metadata = ProjectMetadata());
    bool loadProject(std::unique_ptr<Character>& character, const std::string& filePath,
                    ProjectMetadata& metadata);
    
    // Helper functions
    bool isValidProjectFile(const std::string& filePath);
    std::string getLastError() const { return m_lastError; }
    
    // Project file extension
    static const char* getProjectExtension() { return ".riggle"; }

private:
    std::string m_lastError;
    
    // ZIP-based operations
    bool createZipProject(const Character& character, const std::string& filePath, 
                         const ProjectMetadata& metadata);
    bool extractZipProject(std::unique_ptr<Character>& character, const std::string& filePath,
                          ProjectMetadata& metadata);
    
    // Asset management
    std::vector<std::string> collectAssetPaths(const Character& character);
    
    // JSON creation and parsing
    std::string createProjectJson(const Character& character, const ProjectMetadata& metadata);
    std::string createMetadataJson(const ProjectMetadata& metadata);
    bool parseProjectJson(const std::string& jsonData, std::unique_ptr<Character>& character, 
                         const std::string& assetsDir);
    bool parseMetadataJson(const std::string& jsonData, ProjectMetadata& metadata);
    
    // Reconstruction methods (using actual class methods)
    bool reconstructRig(const json& bonesJson, Character* character);
    bool reconstructSprites(const json& spritesJson, Character* character, const std::string& assetsDir);
    bool reconstructAnimations(const json& animationsJson, Character* character);
    
    // Utility functions
    std::string getCurrentDateTime();
    std::string getFilename(const std::string& path);
    bool fileExists(const std::string& path);
    std::shared_ptr<Bone> findBoneByName(Rig* rig, const std::string& name);
    Transform jsonToTransform(const json& transformJson);
    Vector2 jsonToVector2(const json& vectorJson);
};

}