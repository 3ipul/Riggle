#pragma once
#include <Riggle/Export/IExporter.h>
#include <sstream>

namespace Riggle {

class JSONProjectExporter : public IProjectExporter {
public:
    bool exportProject(const ExportProject& project, const std::string& outputPath) override;
    std::string getFileExtension() const override { return ".json"; }
    std::string getFormatName() const override { return "JSON Project"; }
    std::string serializeProject(const ExportProject& project);
    
private:
    std::string serializeBones(const std::vector<ExportBone>& bones);
    std::string serializeSprites(const std::vector<ExportSprite>& sprites);
    std::string serializeAnimations(const std::vector<ExportAnimation>& animations);
    std::string serializeTransform(const Transform& transform);
    std::string serializeVector2(const Vector2& vec);
    std::string escapeJsonString(const std::string& str);
};

}