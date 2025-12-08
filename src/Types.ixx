module;

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

export module App.Types;

export namespace App {

    // Структура вокселя (как в оригинале)
    struct Voxel {
        uint8_t r, g, b, a;
    };

    // Настройки, которые View может менять через UI, а Presenter передает в Model/Shader
    struct RenderSettings {
        float stepSize = 0.01f;
        float densityThreshold = 0.20f;
        float opacityMultiplier = 1.0f;
        glm::vec3 lightDir = {-0.5f, 0.5f, -1.0f};
        bool vsync = false;
        
        // Для динамического изменения качества
        float highQualityStep = 0.005f;
        float lowQualityStep = 0.015f;
    };

    enum class TFPreset {
        Bone,
        Muscle,
        Rainbow
    };
}