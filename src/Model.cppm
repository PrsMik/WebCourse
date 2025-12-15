module;

#include <algorithm>
#include <cmath>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

export module App.Model;
import App.Types;

export namespace App
{

    class VolumeModel
    {
    private:
        std::vector<Voxel> volumeData;
        std::vector<glm::vec4> tfData;
        int width, height, depth;

    public:
        VolumeModel() : width(0), height(0), depth(0)
        {
            tfData.resize(256);
        }

        bool loadVolume(const VolumeMetadata &meta)
        {
            std::ifstream file(meta.filePath, std::ios::binary);
            if (!file.is_open())
            {
                std::cerr << "Failed to open file: " << meta.filePath << std::endl;
                return false;
            }

            size_t numVoxels = meta.width * meta.height * meta.depth;
            std::vector<uint8_t> densityData;
            densityData.resize(numVoxels);

            std::cout << "Reading " << numVoxels << " voxels..." << std::endl;

            if (meta.type == DataType::UInt8)
            {
                file.read(reinterpret_cast<char *>(densityData.data()), numVoxels);
            }
            else if (meta.type == DataType::UInt16)
            {
                std::vector<uint16_t> tempBuffer(numVoxels);
                file.read(reinterpret_cast<char *>(tempBuffer.data()), numVoxels * 2);

                uint16_t minVal = std::numeric_limits<uint16_t>::max();
                uint16_t maxVal = 0;
                for (uint16_t val : tempBuffer)
                {
                    if (val < minVal)
                        minVal = val;
                    if (val > maxVal)
                        maxVal = val;
                }
                if (maxVal <= minVal)
                    maxVal = minVal + 1;
                float range = static_cast<float>(maxVal - minVal);

                for (size_t i = 0; i < numVoxels; ++i)
                {
                    float normalized = (float)(tempBuffer[i] - minVal) / range;
                    densityData[i] = static_cast<uint8_t>(normalized * 255.0f);
                }
            }

            this->width = meta.width;
            this->height = meta.height;
            this->depth = meta.depth;
            volumeData.resize(numVoxels);

// Расчет градиентов
#pragma omp parallel for
            for (int z = 0; z < depth; ++z)
            {
                for (int y = 0; y < height; ++y)
                {
                    for (int x = 0; x < width; ++x)
                    {
                        int index = x + y * width + z * width * height;
                        uint8_t density = densityData[index];

                        int x1_idx = (x > 0) ? index - 1 : index;
                        int x2_idx = (x < width - 1) ? index + 1 : index;
                        int y1_idx = (y > 0) ? index - width : index;
                        int y2_idx = (y < height - 1) ? index + width : index;
                        int z1_idx = (z > 0) ? index - width * height : index;
                        int z2_idx = (z < depth - 1) ? index + width * height : index;

                        glm::vec3 grad(
                            (float)densityData[x2_idx] - (float)densityData[x1_idx],
                            (float)densityData[y2_idx] - (float)densityData[y1_idx],
                            (float)densityData[z2_idx] - (float)densityData[z1_idx]);

                        if (glm::length(grad) > 0.0f)
                            grad = glm::normalize(grad) * 0.5f + 0.5f;
                        else
                            grad = glm::vec3(0.5f);

                        volumeData[index] = {
                            (uint8_t)(grad.x * 255.0f), (uint8_t)(grad.y * 255.0f), (uint8_t)(grad.z * 255.0f), density};
                    }
                }
            }
            return true;
        }

        void updateTransferFunction(TFPreset preset)
        {
            for (int i = 0; i < 256; i++)
            {
                float t = i / 255.0f;
                glm::vec4 color(0.0f);

                if (preset == TFPreset::Bone)
                {
                    if (t > 0.15f && t < 0.25f)
                        color = glm::vec4(0.6f, 0.4f, 0.3f, 0.05f);
                    else if (t > 0.35f)
                        color = glm::vec4(0.9f, 0.85f, 0.8f, 0.9f);
                }
                else if (preset == TFPreset::Muscle)
                {
                    if (t > 0.1f)
                        color = glm::vec4(0.6f, 0.1f, 0.1f, 0.05f);
                    if (t > 0.25f)
                        color = glm::vec4(0.8f, 0.4f, 0.3f, 0.6f);
                    if (t > 0.5f)
                        color = glm::vec4(1.0f, 1.0f, 1.0f, 0.9f);
                }
                else
                {
                    color = glm::vec4(t, 1.0f - std::abs(t - 0.5f) * 2.0f, 1.0f - t, t * 0.5f);
                }
                tfData[i] = color;
            }
        }

        const std::vector<Voxel> &getVolumeData() const { return volumeData; }
        const std::vector<glm::vec4> &getTFData() const { return tfData; }
        int getWidth() const { return width; }
        int getHeight() const { return height; }
        int getDepth() const { return depth; }
    };
}