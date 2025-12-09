module;

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>


export module App.Types;

export namespace App
{

    struct Voxel
    {
        uint8_t r, g, b, a;
    };

    struct RenderSettings
    {
        // Текущий шаг (вычисляется каждый кадр)
        float stepSize = 0.005f;

        float densityThreshold = 0.20f;
        float opacityMultiplier = 1.0f;
        glm::vec3 lightDir = {-0.5f, 0.5f, -1.0f};
        bool vsync = false;

        // --- Новые поля для управления качеством ---
        bool dynamicQuality = true;     // Вкл/Выкл адаптивный шаг
        float highQualityStep = 0.004f; // Шаг в покое (или ручной шаг)
        float lowQualityStep = 0.02f;   // Шаг при движении
    };

    enum class TFPreset
    {
        Bone,
        Muscle,
        Rainbow
    };
}