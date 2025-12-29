module;

#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

export module App.Types;

export namespace App
{
    enum class AppState
    {
        Intro,
        Running
    };

    struct Voxel
    {
        uint8_t r, g, b, a;
    };

    enum class DataType
    {
        UInt8,
        UInt16
    };

    struct VolumeMetadata
    {
        std::string name;
        std::string filePath;
        int width, height, depth;
        DataType type;
    };

    struct RenderSettings
    {
        float stepSize = 0.005f;
        float densityThreshold = 0.10f;
        float opacityMultiplier = 1.0f;
        glm::vec3 lightDir = {-0.5f, 0.5f, -1.0f};
        bool vsync = false;

        bool dynamicQuality = true;
        float highQualityStep = 0.004f;
        float lowQualityStep = 0.02f;

        glm::vec3 volumeScale = {1.0f, 1.0f, 1.0f};

        bool isMobile = false;
        bool showOnScreenControls = true;
        float uiScale = 1.0f;
    };

    enum class TFPreset
    {
        Bone,
        Muscle,
        Rainbow
    };

    struct MobileInputData
    {
        glm::vec3 moveDir; // x, y, z
        glm::vec2 lookDir; // pitch, yaw
    };
}