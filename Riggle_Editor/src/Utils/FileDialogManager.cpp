#include "Editor/Utils/FileDialogManager.h"
#include <iostream>

namespace Riggle {

FileDialogManager& FileDialogManager::getInstance() {
    static FileDialogManager instance;
    return instance;
}

bool FileDialogManager::openFileDialog(std::string& selectedPath, 
                                      const std::vector<FileFilter>& filters,
                                      const std::string& defaultPath) {
#ifdef _WIN32
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = "";
    
    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    
    // Set default path
    if (!defaultPath.empty()) {
        ofn.lpstrInitialDir = defaultPath.c_str();
    }
    
    // Build filter string
    std::string filterStr = buildFilterString(filters);
    if (!filterStr.empty()) {
        ofn.lpstrFilter = filterStr.c_str();
        ofn.nFilterIndex = 1;
    }
    
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrTitle = "Open File";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameA(&ofn)) {
        selectedPath = std::string(szFile);
        return true;
    }
    
    return false;
#else
    std::cout << "File dialog not implemented for this platform" << std::endl;
    return false;
#endif
}

bool FileDialogManager::saveFileDialog(std::string& selectedPath, 
                                      const std::vector<FileFilter>& filters,
                                      const std::string& defaultPath,
                                      const std::string& defaultName) {
#ifdef _WIN32
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = "";
    
    // Set default filename
    if (!defaultName.empty()) {
        strncpy(szFile, defaultName.c_str(), sizeof(szFile) - 1);
    }
    
    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    
    // Set default path
    if (!defaultPath.empty()) {
        ofn.lpstrInitialDir = defaultPath.c_str();
    }
    
    // Build filter string
    std::string filterStr = buildFilterString(filters);
    if (!filterStr.empty()) {
        ofn.lpstrFilter = filterStr.c_str();
        ofn.nFilterIndex = 1;
    }
    
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrTitle = "Save File";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    
    if (GetSaveFileNameA(&ofn)) {
        selectedPath = std::string(szFile);
        
        // Add extension if not present
        if (!filters.empty() && selectedPath.find('.') == std::string::npos) {
            selectedPath += filters[0].extension;
        }
        
        return true;
    }
    
    return false;
#else
    std::cout << "File dialog not implemented for this platform" << std::endl;
    return false;
#endif
}

bool FileDialogManager::directoryDialog(std::string& selectedPath, 
                                       const std::string& title,
                                       const std::string& defaultPath) {
#ifdef _WIN32
    BROWSEINFOA bi = { 0 };
    bi.lpszTitle = title.c_str();
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl != nullptr) {
        char path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) {
            selectedPath = std::string(path);
            CoTaskMemFree(pidl);
            return true;
        }
        CoTaskMemFree(pidl);
    }
    return false;
#else
    std::cout << "Directory dialog not implemented for this platform" << std::endl;
    return false;
#endif
}

#ifdef _WIN32
std::string FileDialogManager::buildFilterString(const std::vector<FileFilter>& filters) {
    if (filters.empty()) return "";
    
    std::string result;
    for (const auto& filter : filters) {
        result += filter.name + '\0';
        result += "*" + filter.extension + '\0';
    }
    result += '\0'; // Double null terminator
    
    return result;
}
#endif

}