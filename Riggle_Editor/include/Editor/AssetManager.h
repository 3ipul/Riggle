#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace Riggle {

struct AssetInfo {
    std::string name;
    std::string path;
    std::string type; // "image", "json", etc.
    
    AssetInfo(const std::string& n, const std::string& p, const std::string& t) 
        : name(n), path(p), type(t) {}
};

class AssetManager {
public:
    AssetManager();
    ~AssetManager() = default;
    
    // Asset discovery
    void scanDirectory(const std::string& directory);
    void addAsset(const std::string& path);
    
    // Asset access
    const std::vector<AssetInfo>& getAssets() const { return m_assets; }
    const std::vector<AssetInfo>& getImageAssets() const { return m_imageAssets; }
    
    // Asset utilities
    bool isImageFile(const std::string& path) const;
    std::string getAssetName(const std::string& path) const;
    
private:
    std::vector<AssetInfo> m_assets;
    std::vector<AssetInfo> m_imageAssets;
    
    void updateImageAssets();
};

} // namespace Riggle