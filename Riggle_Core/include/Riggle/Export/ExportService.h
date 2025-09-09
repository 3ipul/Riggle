#pragma once
#include "ExportData.h"
#include "../Character.h"
#include "../Animation.h"
#include "../Rig.h"
#include "../Sprite.h"

namespace Riggle {

class ExportService {
public:
    // Convert Character to exportable data
    static ExportProject extractProjectData(const Character& character, const std::string& projectName = "Untitled");
    static ExportAnimation extractAnimationData(const Animation& animation);
    static std::vector<ExportBone> extractBoneData(const Rig& rig);
    static std::vector<ExportSprite> extractSpriteData(const std::vector<std::shared_ptr<Sprite>>& sprites);

private:
    static ExportBone convertBone(std::shared_ptr<Bone> bone);
    static ExportSprite convertSprite(const Sprite& sprite);
    static void collectBoneHierarchy(std::shared_ptr<Bone> bone, std::vector<ExportBone>& bones);
    static ExportBoneTrack convertBoneTrack(const BoneTrack& track);
};

}