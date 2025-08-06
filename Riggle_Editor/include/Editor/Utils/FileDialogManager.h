#pragma once
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <commdlg.h>
#endif

namespace Riggle {

enum class FileDialogType {
    Open,
    Save,
    Directory
};

struct FileFilter {
    std::string name;
    std::string extension;
    
    FileFilter(const std::string& n, const std::string& ext) : name(n), extension(ext) {}
};

class FileDialogManager {
public:
    static FileDialogManager& getInstance();
    
    // File dialogs
    bool openFileDialog(std::string& selectedPath, 
                       const std::vector<FileFilter>& filters = {},
                       const std::string& defaultPath = "");
    
    bool saveFileDialog(std::string& selectedPath, 
                       const std::vector<FileFilter>& filters = {},
                       const std::string& defaultPath = "",
                       const std::string& defaultName = "");
    
    bool directoryDialog(std::string& selectedPath, 
                        const std::string& title = "Select Directory",
                        const std::string& defaultPath = "");

private:
    FileDialogManager() = default;
    
#ifdef _WIN32
    std::string buildFilterString(const std::vector<FileFilter>& filters);
#endif
};

}