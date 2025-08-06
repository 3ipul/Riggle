#include "Editor/Project/ProjectManager.h"
#include "Editor/Utils/miniz.h"
#include <Riggle/Export/ExportService.h>
#include "Editor/Export/JSONExporter.h"
#include <Riggle/Rig.h>
#include <Riggle/Bone.h>
#include <Riggle/Sprite.h>
#include <Riggle/Animation.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <iostream>

namespace Riggle {

ProjectManager::ProjectManager() = default;

bool ProjectManager::saveProject(const Character& character, const std::string& filePath, 
                                const ProjectMetadata& metadata) {
    try {
        m_lastError.clear();
        
        std::string actualPath = filePath;
        if (actualPath.find(getProjectExtension()) == std::string::npos) {
            actualPath += getProjectExtension();
        }
        
        std::cout << "Saving project to: " << actualPath << std::endl;
        
        if (!createZipProject(character, actualPath, metadata)) {
            return false;
        }
        
        std::cout << "Project saved successfully!" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        m_lastError = "Exception during project save: " + std::string(e.what());
        return false;
    }
}

bool ProjectManager::loadProject(std::unique_ptr<Character>& character, const std::string& filePath,
                                ProjectMetadata& metadata) {
    try {
        m_lastError.clear();
        
        if (!isValidProjectFile(filePath)) {
            m_lastError = "Invalid project file: " + filePath;
            return false;
        }
        
        std::cout << "Loading project from: " << filePath << std::endl;
        
        if (!extractZipProject(character, filePath, metadata)) {
            return false;
        }
        
        std::cout << "Project loaded and fully reconstructed!" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        m_lastError = "Exception during project load: " + std::string(e.what());
        return false;
    }
}

bool ProjectManager::isValidProjectFile(const std::string& filePath) {
    if (!fileExists(filePath)) {
        return false;
    }
    
    if (filePath.find(getProjectExtension()) == std::string::npos) {
        return false;
    }
    
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    
    bool isValid = mz_zip_reader_init_file(&zip, filePath.c_str(), 0);
    if (isValid) {
        bool hasProject = mz_zip_reader_locate_file(&zip, "project.json", nullptr, 0) >= 0;
        bool hasMetadata = mz_zip_reader_locate_file(&zip, "metadata.json", nullptr, 0) >= 0;
        isValid = hasProject && hasMetadata;
        mz_zip_reader_end(&zip);
    }
    
    return isValid;
}

bool ProjectManager::createZipProject(const Character& character, const std::string& filePath, 
                                     const ProjectMetadata& metadata) {
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    
    if (!mz_zip_writer_init_file(&zip, filePath.c_str(), 0)) {
        m_lastError = "Failed to create ZIP archive: " + filePath;
        return false;
    }
    
    try {
        // Add project.json
        std::string projectJson = createProjectJson(character, metadata);
        if (!mz_zip_writer_add_mem(&zip, "project.json", 
                                   projectJson.c_str(), projectJson.size(), 
                                   MZ_BEST_COMPRESSION)) {
            m_lastError = "Failed to add project.json to archive";
            mz_zip_writer_end(&zip);
            return false;
        }
        std::cout << "Added project.json (" << projectJson.size() << " bytes)" << std::endl;
        
        // Add metadata.json
        std::string metadataJson = createMetadataJson(metadata);
        if (!mz_zip_writer_add_mem(&zip, "metadata.json", 
                                   metadataJson.c_str(), metadataJson.size(), 
                                   MZ_BEST_COMPRESSION)) {
            m_lastError = "Failed to add metadata.json to archive";
            mz_zip_writer_end(&zip);
            return false;
        }
        std::cout << "Added metadata.json (" << metadataJson.size() << " bytes)" << std::endl;
        
        // Add assets
        std::vector<std::string> assetPaths = collectAssetPaths(character);
        std::cout << "Found " << assetPaths.size() << " assets to pack" << std::endl;
        
        for (const auto& assetPath : assetPaths) {
            if (!fileExists(assetPath)) {
                std::cout << "Warning: Asset file not found: " << assetPath << std::endl;
                continue;
            }
            
            std::string archivePath = "assets/" + getFilename(assetPath);
            
            if (!mz_zip_writer_add_file(&zip, archivePath.c_str(), assetPath.c_str(), 
                                        nullptr, 0, MZ_BEST_COMPRESSION)) {
                std::cout << "Warning: Failed to add asset: " << assetPath << std::endl;
                continue;
            }
            
            std::cout << "Added asset: " << archivePath << std::endl;
        }
        
        if (!mz_zip_writer_finalize_archive(&zip)) {
            m_lastError = "Failed to finalize ZIP archive";
            mz_zip_writer_end(&zip);
            return false;
        }
        
        if (!mz_zip_writer_end(&zip)) {
            m_lastError = "Failed to close ZIP archive";
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = "Exception while creating ZIP: " + std::string(e.what());
        mz_zip_writer_end(&zip);
        return false;
    }
}

bool ProjectManager::extractZipProject(std::unique_ptr<Character>& character, const std::string& filePath,
                                      ProjectMetadata& metadata) {
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    
    if (!mz_zip_reader_init_file(&zip, filePath.c_str(), 0)) {
        m_lastError = "Failed to open ZIP archive: " + filePath;
        return false;
    }
    
    try {
        // 1. Extract and parse metadata.json
        size_t metadataSize = 0;
        void* metadataData = mz_zip_reader_extract_file_to_heap(&zip, "metadata.json", &metadataSize, 0);
        if (!metadataData) {
            m_lastError = "Failed to extract metadata.json from archive";
            mz_zip_reader_end(&zip);
            return false;
        }
        
        std::string metadataJson(static_cast<char*>(metadataData), metadataSize);
        mz_free(metadataData);
        
        if (!parseMetadataJson(metadataJson, metadata)) {
            mz_zip_reader_end(&zip);
            return false;
        }
        std::cout << "Loaded metadata: " << metadata.name << std::endl;
        
        // 2. Extract assets
        std::filesystem::path riggleFilePath(filePath);
        std::string assetsDir = riggleFilePath.parent_path().string() + "/" + 
                               riggleFilePath.stem().string() + "_assets";
        
        std::filesystem::create_directories(assetsDir);
        
        mz_uint numFiles = mz_zip_reader_get_num_files(&zip);
        std::cout << "Extracting assets..." << std::endl;
        
        int extractedCount = 0;
        for (mz_uint i = 0; i < numFiles; i++) {
            mz_zip_archive_file_stat fileStat;
            if (!mz_zip_reader_file_stat(&zip, i, &fileStat)) {
                continue;
            }
            
            std::string filename = fileStat.m_filename;
            
            if (filename.find("assets/") == 0 && filename.length() > 7) {
                std::string assetFilename = filename.substr(7);
                std::string extractPath = assetsDir + "/" + assetFilename;
                
                if (!mz_zip_reader_extract_to_file(&zip, i, extractPath.c_str(), 0)) {
                    std::cout << "Warning: Failed to extract asset: " << filename << std::endl;
                    continue;
                }
                
                std::cout << "Extracted: " << assetFilename << std::endl;
                extractedCount++;
            }
        }
        
        // 3. Extract and parse project.json
        size_t projectSize = 0;
        void* projectData = mz_zip_reader_extract_file_to_heap(&zip, "project.json", &projectSize, 0);
        if (!projectData) {
            m_lastError = "Failed to extract project.json from archive";
            mz_zip_reader_end(&zip);
            return false;
        }
        
        std::string projectJson(static_cast<char*>(projectData), projectSize);
        mz_free(projectData);
        
        // 4. Fully reconstruct the character
        if (!parseProjectJson(projectJson, character, assetsDir)) {
            mz_zip_reader_end(&zip);
            return false;
        }
        
        mz_zip_reader_end(&zip);
        
        std::cout << "Extracted " << extractedCount << " assets to: " << assetsDir << std::endl;
        std::cout << "Character fully reconstructed!" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = "Exception while extracting ZIP: " + std::string(e.what());
        mz_zip_reader_end(&zip);
        return false;
    }
}

bool ProjectManager::parseProjectJson(const std::string& jsonData, std::unique_ptr<Character>& character, 
                                     const std::string& assetsDir) {
    try {
        json projectJson = json::parse(jsonData);
        
        std::string projectName = projectJson.value("name", "Loaded Project");
        std::cout << "Reconstructing project: " << projectName << std::endl;
        
        // Create character using actual constructor
        character = std::make_unique<Character>(projectName);
        
        // Reconstruct rig first (bones are the foundation)
        if (projectJson.contains("bones") && projectJson["bones"].is_array()) {
            if (!reconstructRig(projectJson["bones"], character.get())) {
                std::cout << "Warning: Failed to reconstruct rig" << std::endl;
            }
        }
        
        // Reconstruct sprites
        if (projectJson.contains("sprites") && projectJson["sprites"].is_array()) {
            if (!reconstructSprites(projectJson["sprites"], character.get(), assetsDir)) {
                std::cout << "Warning: Failed to reconstruct sprites" << std::endl;
            }
        }
        
        // Reconstruct animations
        if (projectJson.contains("animations") && projectJson["animations"].is_array()) {
            if (!reconstructAnimations(projectJson["animations"], character.get())) {
                std::cout << "Warning: Failed to reconstruct animations" << std::endl;
            }
        }
        
        std::cout << "Project reconstruction completed!" << std::endl;
        return true;
        
    } catch (const json::exception& e) {
        m_lastError = "JSON parsing error: " + std::string(e.what());
        return false;
    }
}

bool ProjectManager::reconstructRig(const json& bonesJson, Character* character) {
    try {
        std::cout << "Reconstructing " << bonesJson.size() << " bones..." << std::endl;
        
        auto rig = std::make_unique<Rig>("Reconstructed Rig");
        rig->setCharacter(character);
        
        std::unordered_map<std::string, std::shared_ptr<Bone>> boneMap;

        // First pass: Create all bones and store them in a map.
        for (const auto& boneJson : bonesJson) {
            std::string name = boneJson.value("name", "UnnamedBone");
            float length = boneJson.value("length", 100.0f);
            
            // Create bone directly, don't add to rig yet.
            auto bone = std::make_shared<Bone>(name, length);
            bone->setCharacter(character);
            
            if (boneJson.contains("transform")) {
                Transform transform = jsonToTransform(boneJson["transform"]);
                bone->setLocalTransform(transform);
            }
            
            boneMap[name] = bone;
            std::cout << "Created bone instance: " << name << std::endl;
        }
        
        // Second pass: Set up parent-child relationships and add ONLY root bones to the rig.
        for (const auto& boneJson : bonesJson) {
            std::string boneName = boneJson.value("name", "");
            std::string parentName = boneJson.value("parentName", "");
            
            auto bone = boneMap[boneName];
            if (parentName.empty()) {
                // This is a root bone, add it to the rig.
                rig->addRootBone(bone);
                std::cout << "Added root bone to rig: " << boneName << std::endl;
            } else {
                auto parent = boneMap[parentName];
                if (parent) {
                    // This is a child bone, add it to its parent.
                    parent->addChild(bone);
                    bone->setParent(parent);
                    std::cout << "Set parent: " << boneName << " -> " << parentName << std::endl;
                }
            }
        }
        
        character->setRig(std::move(rig));
        
        std::cout << "Rig reconstruction completed!" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "Error reconstructing rig: " << e.what() << std::endl;
        return false;
    }
}

// bool ProjectManager::reconstructSprites(const json& spritesJson, Character* character, const std::string& assetsDir) {
//     try {
//         std::cout << "Reconstructing " << spritesJson.size() << " sprites..." << std::endl;

//         Rig* rig = character->getRig();
//         if (!rig) {
//             std::cout << "ERROR: No rig available!" << std::endl;
//             return false;
//         }
        
//         // Force update all bone world transforms before binding sprites
//         rig->forceUpdateWorldTransforms();
        
//         int successCount = 0;
        
//         for (const auto& spriteJson : spritesJson) {
//             try {
//                 std::string name = spriteJson.value("name", "UnnamedSprite");
//                 std::string boundBoneName = spriteJson.value("boundBoneName", "");
                
//                 std::cout << "\n--- Processing sprite: " << name << " ---" << std::endl;
                
//                 // Create sprite with texture path
//                 std::string texturePath = spriteJson.value("texturePath", "");
//                 std::string finalTexturePath = texturePath;
//                 if (!texturePath.empty()) {
//                     std::string filename = getFilename(texturePath);
//                     finalTexturePath = assetsDir + "/" + filename;
//                     if (!fileExists(finalTexturePath) && fileExists(texturePath)) {
//                         finalTexturePath = texturePath;
//                     }
//                 }
                
//                 // Create sprite as shared_ptr (since Sprite inherits from enable_shared_from_this)
//                 auto sprite = std::make_shared<Sprite>(name, finalTexturePath);
                
//                 // Set visibility
//                 if (spriteJson.contains("isVisible")) {
//                     sprite->setVisible(spriteJson.value("isVisible", true));
//                 }
                
//                 // Check if this sprite should be bound
//                 bool shouldBind = !boundBoneName.empty();
//                 std::cout << "Should bind to bone: " << (shouldBind ? boundBoneName : "NO") << std::endl;
                
//                 if (shouldBind) {
//                     // Get binding data from JSON
//                     Vector2 bindOffset = {0, 0};
//                     float bindRotation = 0.0f;
                    
//                     if (spriteJson.contains("bindOffset")) {
//                         bindOffset = jsonToVector2(spriteJson["bindOffset"]);
//                         std::cout << "  bindOffset: (" << bindOffset.x << ", " << bindOffset.y << ")" << std::endl;
//                     }
                    
//                     if (spriteJson.contains("bindRotation")) {
//                         bindRotation = spriteJson.value("bindRotation", 0.0f);
//                         std::cout << "  bindRotation: " << bindRotation << " rad (" << (bindRotation * 180.0f / 3.14159f) << " deg)" << std::endl;
//                     }
                    
//                     // Find the bone
//                     auto bone = rig->findBone(boundBoneName);
//                     if (!bone) {
//                         std::cout << "ERROR: Bone not found: " << boundBoneName << std::endl;
                        
//                         // Set local transform as fallback
//                         if (spriteJson.contains("transform")) {
//                             Transform transform = jsonToTransform(spriteJson["transform"]);
//                             sprite->setLocalTransform(transform);
//                         }
                        
//                         // Add unbound sprite to character
//                         character->addSprite(std::move(std::make_unique<Sprite>(*sprite)));
//                         continue;
//                     }
                    
//                     try {
//                         std::cout << "Attempting to bind sprite (shared_ptr context)..." << std::endl;
                        
//                         // Bind the sprite to bone - now sprite is a shared_ptr
//                         sprite->restoreBinding(bone, bindOffset, bindRotation);
                        
//                         // Check the resulting world position
//                         Transform spriteWorld = sprite->getWorldTransform();
                        
//                         std::cout << "Binding successful!" << std::endl;
                        
//                     } catch (const std::exception& bindError) {
//                         std::cout << "ERROR during binding: " << bindError.what() << std::endl;
                        
//                         // Fallback: set local transform if available
//                         if (spriteJson.contains("transform")) {
//                             Transform transform = jsonToTransform(spriteJson["transform"]);
//                             sprite->setLocalTransform(transform);
//                             std::cout << "Applied local transform as fallback" << std::endl;
//                         }
//                     }
//                 } else {
//                     // Unbound sprite: set local transform
//                     if (spriteJson.contains("transform")) {
//                         Transform transform = jsonToTransform(spriteJson["transform"]);
//                         sprite->setLocalTransform(transform);
//                         std::cout << "Set local transform for unbound sprite" << std::endl;
//                     }
//                 }
                
//                 // Convert shared_ptr to unique_ptr for adding to character
//                 // This is needed if Character::addSprite expects unique_ptr
//                 auto uniqueSprite = std::make_unique<Sprite>(*sprite); // Copy construct
                
//                 // If the sprite was bound, we need to re-bind after the copy
//                 if (shouldBind && !boundBoneName.empty()) {
//                     auto bone = rig->findBone(boundBoneName);
//                     if (bone) {
//                         Vector2 bindOffset = {0, 0};
//                         float bindRotation = 0.0f;
                        
//                         if (spriteJson.contains("bindOffset")) {
//                             bindOffset = jsonToVector2(spriteJson["bindOffset"]);
//                         }
//                         if (spriteJson.contains("bindRotation")) {
//                             bindRotation = spriteJson.value("bindRotation", 0.0f);
//                         }
                        
//                         // We can't bind the unique_ptr version easily, so let's try a different approach
//                     }
//                 }
                
//                 // Add sprite to character
//                 character->addSprite(std::move(uniqueSprite));
                
//                 successCount++;
//                 std::cout << "Successfully processed sprite: " << name << std::endl;
                
//             } catch (const std::exception& e) {
//                 std::cout << "ERROR processing sprite: " << e.what() << std::endl;
//                 continue;
//             }
//         }
        
//         std::cout << "Sprite reconstruction completed! Successfully loaded " << successCount << " out of " << spritesJson.size() << " sprites." << std::endl;
//         return successCount > 0;
        
//     } catch (const std::exception& e) {
//         std::cout << "FATAL ERROR in sprite reconstruction: " << e.what() << std::endl;
//         return false;
//     }
// }

bool ProjectManager::reconstructSprites(const json& spritesJson, Character* character, const std::string& assetsDir) {
    try {
        std::cout << "Reconstructing " << spritesJson.size() << " sprites..." << std::endl;

        Rig* rig = character->getRig();
        if (!rig) {
            std::cout << "ERROR: No rig available!" << std::endl;
            return false;
        }

        // Force update all bone world transforms before binding sprites
        rig->forceUpdateWorldTransforms();

        int successCount = 0;

        for (const auto& spriteJson : spritesJson) {
            try {
                std::string name = spriteJson.value("name", "UnnamedSprite");
                std::string boundBoneName = spriteJson.value("boundBoneName", "");

                std::cout << "\n--- Processing sprite: " << name << " ---" << std::endl;

                // Create sprite with texture path
                std::string texturePath = spriteJson.value("texturePath", "");
                std::string finalTexturePath = texturePath;
                if (!texturePath.empty()) {
                    std::string filename = getFilename(texturePath);
                    finalTexturePath = assetsDir + "/" + filename;
                    if (!fileExists(finalTexturePath) && fileExists(texturePath)) {
                        finalTexturePath = texturePath;
                    }
                }

                // Create sprite as shared_ptr (since Sprite inherits from enable_shared_from_this)
                auto sprite = std::make_shared<Sprite>(name, finalTexturePath);

                // Set visibility
                if (spriteJson.contains("isVisible")) {
                    sprite->setVisible(spriteJson.value("isVisible", true));
                }

                // Unbound sprite: set local transform
                if (spriteJson.contains("transform")) {
                    Transform transform = jsonToTransform(spriteJson["transform"]);
                    sprite->setLocalTransform(transform);
                    if (boundBoneName.empty()) {
                        std::cout << "Set local transform for unbound sprite" << std::endl;
                    }
                }

                // Convert shared_ptr to unique_ptr for adding to character
                auto uniqueSprite = std::make_unique<Sprite>(*sprite); // Copy construct

                // Add sprite to character
                character->addSprite(std::move(uniqueSprite));

                // After adding, re-bind the actual stored sprite if needed
                if (!boundBoneName.empty()) {
                    Sprite* storedSprite = character->findSprite(name);
                    auto bone = rig->findBone(boundBoneName);
                    if (storedSprite && bone) {
                        Vector2 bindOffset = {0, 0};
                        float bindRotation = 0.0f;
                        if (spriteJson.contains("bindOffset")) {
                            bindOffset = jsonToVector2(spriteJson["bindOffset"]);
                            std::cout << "  bindOffset: (" << bindOffset.x << ", " << bindOffset.y << ")" << std::endl;
                        }
                        if (spriteJson.contains("bindRotation")) {
                            bindRotation = spriteJson.value("bindRotation", 0.0f);
                            std::cout << "  bindRotation: " << bindRotation << " rad (" << (bindRotation * 180.0f / 3.14159f) << " deg)" << std::endl;
                        }
                        storedSprite->restoreBinding(bone, bindOffset, bindRotation);
                        std::cout << "Successfully bound sprite '" << name << "' to bone '" << boundBoneName << "'" << std::endl;
                    } else if (!bone) {
                        std::cout << "ERROR: Bone not found: " << boundBoneName << std::endl;
                    }
                }

                successCount++;
                std::cout << "Successfully processed sprite: " << name << std::endl;

            } catch (const std::exception& e) {
                std::cout << "ERROR processing sprite: " << e.what() << std::endl;
                continue;
            }
        }

        std::cout << "Sprite reconstruction completed! Successfully loaded " << successCount << " out of " << spritesJson.size() << " sprites." << std::endl;
        return successCount > 0;

    } catch (const std::exception& e) {
        std::cout << "FATAL ERROR in sprite reconstruction: " << e.what() << std::endl;
        return false;
    }
}

bool ProjectManager::reconstructAnimations(const json& animationsJson, Character* character) {
    try {
        std::cout << "Reconstructing " << animationsJson.size() << " animations..." << std::endl;
        
        for (const auto& animJson : animationsJson) {
            std::string name = animJson.value("name", "UnnamedAnimation");
            float duration = animJson.value("duration", 1.0f);  // We still read duration from JSON
            
            // Create animation using actual constructor (only takes name)
            auto animation = std::make_unique<Animation>(name);  // Remove duration parameter
            
            // Add keyframes if available
            if (animJson.contains("tracks") && animJson["tracks"].is_array()) {
                for (const auto& trackJson : animJson["tracks"]) {
                    std::string boneName = trackJson.value("boneName", "");
                    
                    if (trackJson.contains("keyframes") && trackJson["keyframes"].is_array()) {
                        for (const auto& keyframeJson : trackJson["keyframes"]) {
                            float time = keyframeJson.value("time", 0.0f);
                            
                            if (keyframeJson.contains("transform")) {
                                Transform transform = jsonToTransform(keyframeJson["transform"]);
                                // Use actual Animation method to add keyframe
                                animation->addKeyframe(boneName, time, transform);
                            }
                        }
                    }
                }
            }
            
            // Add animation to character using actual method
            character->addAnimation(std::move(animation));
            std::cout << "Created animation: " << name << std::endl;  // Remove duration from log
        }
        
        std::cout << "Animation reconstruction completed!" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "Error reconstructing animations: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<Bone> ProjectManager::findBoneByName(Rig* rig, const std::string& name) {
    return rig ? rig->findBone(name) : nullptr;
}

Transform ProjectManager::jsonToTransform(const json& transformJson) {
    Transform transform;
    
    if (transformJson.contains("position")) {
        transform.position = jsonToVector2(transformJson["position"]);
    }
    
    if (transformJson.contains("rotation")) {
        transform.rotation = transformJson.value("rotation", 0.0f);
    }
    
    if (transformJson.contains("scale")) {
        transform.scale = jsonToVector2(transformJson["scale"]);
    }
    
    if (transformJson.contains("length")) {
        transform.length = transformJson.value("length", 50.0f);
    }
    
    return transform;
}

Vector2 ProjectManager::jsonToVector2(const json& vectorJson) {
    Vector2 vec;
    vec.x = vectorJson.value("x", 0.0f);
    vec.y = vectorJson.value("y", 0.0f);
    return vec;
}

std::vector<std::string> ProjectManager::collectAssetPaths(const Character& character) {
    std::vector<std::string> assetPaths;
    
    // Use actual Character method to get sprites
    const auto& sprites = character.getSprites();
    for (const auto& sprite : sprites) {
        if (sprite) {
            std::string texturePath = sprite->getTexturePath();
            if (!texturePath.empty() && fileExists(texturePath)) {
                if (std::find(assetPaths.begin(), assetPaths.end(), texturePath) == assetPaths.end()) {
                    assetPaths.push_back(texturePath);
                }
            }
        }
    }
    
    return assetPaths;
}

std::string ProjectManager::createProjectJson(const Character& character, const ProjectMetadata& metadata) {
    // Use existing ExportService which should be compatible
    ExportProject projectData = ExportService::extractProjectData(character, metadata.name);
    JSONProjectExporter jsonExporter;
    return jsonExporter.serializeProject(projectData);
}

std::string ProjectManager::createMetadataJson(const ProjectMetadata& metadata) {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"name\": \"" << metadata.name << "\",\n";
    json << "  \"version\": \"" << metadata.version << "\",\n";
    json << "  \"author\": \"" << metadata.author << "\",\n";
    json << "  \"description\": \"" << metadata.description << "\",\n";
    json << "  \"createdDate\": \"" << metadata.createdDate << "\",\n";
    json << "  \"modifiedDate\": \"" << getCurrentDateTime() << "\",\n";
    json << "  \"riggleVersion\": \"1.0\"\n";
    json << "}";
    
    return json.str();
}

bool ProjectManager::parseMetadataJson(const std::string& jsonData, ProjectMetadata& metadata) {
    try {
        json metadataJson = json::parse(jsonData);
        
        metadata.name = metadataJson.value("name", "Loaded Project");
        metadata.version = metadataJson.value("version", "1.0");
        metadata.author = metadataJson.value("author", "Unknown");
        metadata.description = metadataJson.value("description", "Loaded from .riggle file");
        metadata.createdDate = metadataJson.value("createdDate", getCurrentDateTime());
        metadata.modifiedDate = metadataJson.value("modifiedDate", getCurrentDateTime());
        
        return true;
        
    } catch (const json::exception& e) {
        std::cout << "Warning: Could not parse metadata JSON: " << e.what() << std::endl;
        
        // Set defaults
        metadata.name = "Loaded Project";
        metadata.version = "1.0";
        metadata.author = "Unknown";
        metadata.description = "Loaded from .riggle file";
        metadata.createdDate = getCurrentDateTime();
        metadata.modifiedDate = getCurrentDateTime();
        
        return true;
    }
}

std::string ProjectManager::getCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string ProjectManager::getFilename(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

bool ProjectManager::fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

}