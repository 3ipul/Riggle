#pragma once
#include "ExportData.h"

namespace Riggle {

class IProjectExporter {
public:
    virtual ~IProjectExporter() = default;
    virtual bool exportProject(const ExportProject& project, const std::string& outputPath) = 0;
    virtual std::string getFileExtension() const = 0;
    virtual std::string getFormatName() const = 0;
    virtual std::string getLastError() const { return m_lastError; }

protected:
    mutable std::string m_lastError;
};

class IAnimationExporter {
public:
    virtual ~IAnimationExporter() = default;
    virtual bool exportAnimation(const ExportAnimation& animation, 
                                const std::vector<ExportSprite>& sprites,
                                const std::vector<ExportBone>& bones,
                                const std::string& outputPath) = 0;
    virtual std::string getFileExtension() const = 0;
    virtual std::string getFormatName() const = 0;
    virtual std::string getLastError() const { return m_lastError; }

protected:
    mutable std::string m_lastError;
};

}