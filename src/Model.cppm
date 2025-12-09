module;

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>

export module App.Model;
import App.Types;

export namespace App {

    class VolumeModel {
    private:
        std::vector<Voxel> volumeData;
        std::vector<glm::vec4> tfData;
        int width, height, depth;

    public:
        VolumeModel(int w, int h, int d) : width(w), height(h), depth(d) {
            tfData.resize(256);
        }

        bool loadRawData(const std::string& filepath) {
            std::ifstream file(filepath, std::ios::binary);
            if (!file.is_open()) return false;

            std::vector<uint8_t> densityData(std::istreambuf_iterator<char>(file), {});
            if (densityData.size() != width * height * depth) return false;

            volumeData.resize(densityData.size());

            // Pre-compute gradients (Optimization step)
            // Логика переносена из main.cpp
            for (int z = 0; z < depth; ++z) {
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
                        int index = x + y * width + z * width * height;
                        uint8_t density = densityData[index];

                        int x1 = (x > 0) ? densityData[index - 1] : density;
                        int x2 = (x < width - 1) ? densityData[index + 1] : density;
                        int y1 = (y > 0) ? densityData[index - width] : density;
                        int y2 = (y < height - 1) ? densityData[index + width] : density;
                        int z1 = (z > 0) ? densityData[index - width * height] : density;
                        int z2 = (z < depth - 1) ? densityData[index + width * height] : density;

                        glm::vec3 grad((float)(x1 - x2), (float)(y1 - y2), (float)(z1 - z2));
                        
                        if (glm::length(grad) > 0.0f) {
                            grad = glm::normalize(grad);
                            grad = grad * 0.5f + 0.5f;
                        } else {
                            grad = glm::vec3(0.5f);
                        }

                        volumeData[index] = {
                            (uint8_t)(grad.x * 255.0f),
                            (uint8_t)(grad.y * 255.0f),
                            (uint8_t)(grad.z * 255.0f),
                            density
                        };
                    }
                }
            }
            return true;
        }

        void updateTransferFunction(TFPreset preset) {
             for (int i = 0; i < 256; i++) {
                float t = i / 255.0f;
                glm::vec4 color(0.0f);

                if (preset == TFPreset::Bone) {
                    if (t > 0.15f && t < 0.25f) color = glm::vec4(0.6f, 0.4f, 0.3f, 0.05f);
                    else if (t > 0.35f) color = glm::vec4(0.9f, 0.85f, 0.8f, 0.9f);
                } else if (preset == TFPreset::Muscle) {
                    if (t > 0.1f) color = glm::vec4(0.6f, 0.1f, 0.1f, 0.05f);
                    if (t > 0.25f) color = glm::vec4(0.8f, 0.4f, 0.3f, 0.6f);
                    if (t > 0.5f) color = glm::vec4(1.0f, 1.0f, 1.0f, 0.9f);
                } else { // Rainbow
                    color = glm::vec4(t, 1.0f - std::abs(t - 0.5f) * 2.0f, 1.0f - t, t * 0.5f);
                }
                tfData[i] = color;
            }
        }

        const std::vector<Voxel>& getVolumeData() const { return volumeData; }
        const std::vector<glm::vec4>& getTFData() const { return tfData; }
        int getWidth() const { return width; }
        int getHeight() const { return height; }
        int getDepth() const { return depth; }
    };
}