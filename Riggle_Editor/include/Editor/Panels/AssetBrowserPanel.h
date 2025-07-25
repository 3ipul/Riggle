#pragma once
#include "BasePanel.h"
#include "../AssetManager.h"
#include <memory>
#include <functional>

namespace Riggle {

class AssetBrowserPanel : public BasePanel {
public:
    AssetBrowserPanel();
    ~AssetBrowserPanel() = default;

    void render() override;
    void update(sf::RenderWindow& window) override;

    // Callbacks
    void setOnAssetSelected(std::function<void(const AssetInfo&)> callback) {
        m_onAssetSelected = callback;
    }

    // Asset management
    void refreshAssets();
    void scanDirectory(const std::string& directory);

private:
    std::unique_ptr<AssetManager> m_assetManager;
    std::function<void(const AssetInfo&)> m_onAssetSelected;
    
    std::string m_currentDirectory;
    bool m_showDirectoryInput;
};

} // namespace Riggle