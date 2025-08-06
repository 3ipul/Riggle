#include "Editor/Export/JSONExporter.h"
#include <fstream>
#include <iomanip>

namespace Riggle {

bool JSONProjectExporter::exportProject(const ExportProject& project, const std::string& outputPath) {
    try {
        std::string jsonContent = serializeProject(project);
        
        std::ofstream file(outputPath);
        if (!file.is_open()) {
            m_lastError = "Failed to open file for writing: " + outputPath;
            return false;
        }
        
        file << jsonContent;
        file.close();
        
        if (file.fail()) {
            m_lastError = "Failed to write to file: " + outputPath;
            return false;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        m_lastError = "Exception during export: " + std::string(e.what());
        return false;
    }
}

std::string JSONProjectExporter::serializeProject(const ExportProject& project) {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"name\": \"" << escapeJsonString(project.name) << "\",\n";
    json << "  \"version\": \"" << escapeJsonString(project.version) << "\",\n";
    json << "  \"bones\": " << serializeBones(project.bones) << ",\n";
    json << "  \"sprites\": " << serializeSprites(project.sprites) << ",\n";
    json << "  \"animations\": " << serializeAnimations(project.animations) << "\n";
    json << "}";
    
    return json.str();
}

std::string JSONProjectExporter::serializeBones(const std::vector<ExportBone>& bones) {
    std::ostringstream json;
    
    json << "[\n";
    for (size_t i = 0; i < bones.size(); ++i) {
        const auto& bone = bones[i];
        
        json << "    {\n";
        json << "      \"name\": \"" << escapeJsonString(bone.name) << "\",\n";
        json << "      \"parentName\": \"" << escapeJsonString(bone.parentName) << "\",\n";
        json << "      \"transform\": " << serializeTransform(bone.transform) << ",\n";
        json << "      \"length\": " << std::fixed << std::setprecision(6) << bone.length << ",\n";
        json << "      \"childNames\": [";
        
        for (size_t j = 0; j < bone.childNames.size(); ++j) {
            json << "\"" << escapeJsonString(bone.childNames[j]) << "\"";
            if (j < bone.childNames.size() - 1) json << ", ";
        }
        
        json << "]\n";
        json << "    }";
        if (i < bones.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ]";
    
    return json.str();
}

std::string JSONProjectExporter::serializeSprites(const std::vector<ExportSprite>& sprites) {
    std::ostringstream json;
    
    json << "[\n";
    for (size_t i = 0; i < sprites.size(); ++i) {
        const auto& sprite = sprites[i];
        
        json << "    {\n";
        json << "      \"name\": \"" << escapeJsonString(sprite.name) << "\",\n";
        json << "      \"texturePath\": \"" << escapeJsonString(sprite.texturePath) << "\",\n";
        json << "      \"transform\": " << serializeTransform(sprite.transform) << ",\n";
        json << "      \"isVisible\": " << (sprite.isVisible ? "true" : "false") << ",\n";
        json << "      \"boundBoneName\": \"" << escapeJsonString(sprite.boundBoneName) << "\",\n";
        json << "      \"bindOffset\": " << serializeVector2(sprite.bindOffset) << ",\n";
        json << "      \"bindRotation\": " << std::fixed << std::setprecision(6) << sprite.bindRotation << "\n";
        json << "    }";
        if (i < sprites.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ]";
    
    return json.str();
}

std::string JSONProjectExporter::serializeAnimations(const std::vector<ExportAnimation>& animations) {
    std::ostringstream json;
    
    json << "[\n";
    for (size_t i = 0; i < animations.size(); ++i) {
        const auto& anim = animations[i];
        
        json << "    {\n";
        json << "      \"name\": \"" << escapeJsonString(anim.name) << "\",\n";
        json << "      \"duration\": " << std::fixed << std::setprecision(6) << anim.duration << ",\n";
        json << "      \"tracks\": [\n";
        
        for (size_t j = 0; j < anim.tracks.size(); ++j) {
            const auto& track = anim.tracks[j];
            
            json << "        {\n";
            json << "          \"boneName\": \"" << escapeJsonString(track.boneName) << "\",\n";
            json << "          \"keyframes\": [\n";
            
            for (size_t k = 0; k < track.keyframes.size(); ++k) {
                const auto& keyframe = track.keyframes[k];
                
                json << "            {\n";
                json << "              \"time\": " << std::fixed << std::setprecision(6) << keyframe.time << ",\n";
                json << "              \"transform\": " << serializeTransform(keyframe.transform) << "\n";
                json << "            }";
                if (k < track.keyframes.size() - 1) json << ",";
                json << "\n";
            }
            
            json << "          ]\n";
            json << "        }";
            if (j < anim.tracks.size() - 1) json << ",";
            json << "\n";
        }
        
        json << "      ]\n";
        json << "    }";
        if (i < animations.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ]";
    
    return json.str();
}

std::string JSONProjectExporter::serializeTransform(const Transform& transform) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(6);
    json << "{ \"position\": " << serializeVector2(transform.position) 
         << ", \"rotation\": " << transform.rotation 
         << ", \"scale\": " << serializeVector2(transform.scale) << " }";
    return json.str();
}

std::string JSONProjectExporter::serializeVector2(const Vector2& vec) {
    std::ostringstream json;
    json << std::fixed << std::setprecision(6);
    json << "{ \"x\": " << vec.x << ", \"y\": " << vec.y << " }";
    return json.str();
}

std::string JSONProjectExporter::escapeJsonString(const std::string& str) {
    std::string escaped;
    escaped.reserve(str.length());
    
    for (char c : str) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    
    return escaped;
}

}