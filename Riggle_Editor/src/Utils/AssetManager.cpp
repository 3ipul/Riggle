#include "Editor/Utils/AssetManager.h"
#include <iostream>
#include <algorithm>

namespace Riggle {

AssetManager::AssetManager() {
}

void AssetManager::scanDirectory(const std::string& directory) {
    m_assets.clear();

    try {
        if (std::filesystem::exists(directory) && std::filesystem::is_directory(directory)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    addAsset(entry.path().string());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Error scanning directory " << directory << ": " << e.what() << std::endl;
    }
}

void AssetManager::addAsset(const std::string& path) {
    std::string name = getAssetName(path);
    std::string type = "unknown";
    
    if (isImageFile(path)) {
        type = "image";
    }
    
    m_assets.emplace_back(name, path, type);
    updateImageAssets();
}

bool AssetManager::isImageFile(const std::string& path) const {
    std::string extension = std::filesystem::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return extension == ".png" || extension == ".jpg" || extension == ".jpeg" || 
           extension == ".bmp" || extension == ".tga" || extension == ".gif";
}

std::string AssetManager::getAssetName(const std::string& path) const {
    return std::filesystem::path(path).filename().string();
}

void AssetManager::updateImageAssets() {
    m_imageAssets.clear();
    for (const auto& asset : m_assets) {
        if (asset.type == "image") {
            m_imageAssets.push_back(asset);
        }
    }
}

} // namespace Riggle