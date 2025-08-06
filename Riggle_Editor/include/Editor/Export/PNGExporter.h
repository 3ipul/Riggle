#pragma once
#include <Riggle/Export/IExporter.h>
#include <SFML/Graphics.hpp>
#include <map>

namespace Riggle {

class PNGSequenceExporter : public IAnimationExporter {
public:
    PNGSequenceExporter();
    
    bool exportAnimation(const ExportAnimation& animation, 
                        const std::vector<ExportSprite>& sprites,
                        const std::vector<ExportBone>& bones,
                        const std::string& outputPath) override;
    std::string getFileExtension() const override { return ""; } // Directory
    std::string getFormatName() const override { return "PNG Sequence"; }

    // Configuration
    void setFrameRate(int fps) { m_frameRate = fps; }
    void setResolution(int width, int height) { m_width = width; m_height = height; }

private:
    int m_frameRate;
    int m_width;
    int m_height;
    
    // Texture cache for performance
    mutable std::map<std::string, sf::Texture> m_textureCache;
    
    bool renderFrame(float time, const ExportAnimation& animation,
                    const std::vector<ExportSprite>& sprites, 
                    const std::vector<ExportBone>& bones, 
                    const std::string& framePath);
    
    Transform interpolateTransform(const std::vector<ExportKeyframe>& keyframes, float time);
    void applyAnimationToBones(std::vector<ExportBone>& animatedBones, 
                              const ExportAnimation& animation, float time);
    void calculateAllWorldTransforms(std::vector<ExportBone>& bones);
    Transform calculateBoneWorldTransform(const ExportBone& bone, 
                                         const std::vector<ExportBone>& bones,
                                         std::map<std::string, Transform>& worldTransforms);
    Transform combineTransforms(const Transform& parent, const Transform& local);
    Transform calculateSpriteWorldTransform(const ExportSprite& sprite, 
                                           const std::vector<ExportBone>& animatedBones);
    const ExportBone* findBone(const std::vector<ExportBone>& bones, const std::string& name);
    bool createDirectory(const std::string& path);
};

}