#pragma once
#include "BasePanel.h"
#include "Editor/AssetManager.h"
#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <commdlg.h>
#endif

namespace Riggle {

class AssetBrowserPanel : public BasePanel {
public:
    AssetBrowserPanel();
    ~AssetBrowserPanel() = default;

    void render() override;
    void update(sf::RenderWindow& window) override;

    // Asset selection callback
    void setOnAssetSelected(std::function<void(const AssetInfo&)> callback) {
        m_onAssetSelected = callback;
    }
    
    // Multiple assets selected callback
    void setOnMultipleAssetsSelected(std::function<void(const std::vector<AssetInfo>&)> callback) {
        m_onMultipleAssetsSelected = callback;
    }

    // Directory management
    void setCurrentDirectory(const std::string& directory);
    const std::string& getCurrentDirectory() const { return m_currentDirectory; }

private:
    std::unique_ptr<AssetManager> m_assetManager;
    std::string m_currentDirectory;
    bool m_showDirectoryInput;
    
    // Selection state
    std::unordered_set<std::string> m_selectedAssets; // Store selected asset paths
    
    // Callbacks
    std::function<void(const AssetInfo&)> m_onAssetSelected;
    std::function<void(const std::vector<AssetInfo>&)> m_onMultipleAssetsSelected; // NEW
    
    // Helper functions
    void refreshAssets();
    void scanDirectory(const std::string& directory);
    void openDirectoryDialog();
    std::string selectDirectory();
    
    // Selection management
    void selectAll();
    void selectNone();
    void insertSelected();
    bool isAssetSelected(const std::string& assetPath) const;
    void toggleAssetSelection(const std::string& assetPath);
};

} // namespace Riggle