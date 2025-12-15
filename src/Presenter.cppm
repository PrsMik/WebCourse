module;

#include "backends/imgui_impl_sdl3.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <vector>

export module App.Presenter;

import App.Types;
import App.Model;
import App.View;
import App.Camera;

export namespace App
{

    class Presenter
    {
    private:
        VolumeModel model;
        Renderer view;
        Camera camera;
        RenderSettings settings;

        bool isRunning = true;
        bool mouseCaptured = false;

        glm::vec3 modelRotation = glm::vec3(0.0f, 0.0f, 0.0f);

        TFPreset currentPreset = TFPreset::Bone;

        std::string fragShaderPath;
        std::vector<VolumeMetadata> presets;

    public:
        Presenter()
            : model(), camera(glm::vec3(0, 0, 3.5f), glm::vec3(0, 0, 0))
        {
            fragShaderPath = DATA_DIR "/shaders/FragmentShader.glsl";

            presets.push_back({"Male Head (CT)", DATA_DIR "/vis_male_128x256x256_uint8.raw", 128, 256, 256, DataType::UInt8});
            presets.push_back({"Engine Block", DATA_DIR "/engine_256x256x128_uint8.raw", 256, 256, 128, DataType::UInt8});
            presets.push_back({"Christmas Carp", DATA_DIR "/carp_256x256x512_uint16.raw", 256, 256, 512, DataType::UInt16});
        }

        bool initialize()
        {
            if (!view.init(1280, 720, "Volumetric Raymarching"))
                return false;
            loadVolume(presets[0]);
            return true;
        }

        void loadVolume(const VolumeMetadata &meta)
        {
            std::cout << "Loading: " << meta.name << "..." << std::endl;
            if (model.loadVolume(meta))
            {
                model.updateTransferFunction(currentPreset);
                view.setVolumeData(model.getWidth(), model.getHeight(), model.getDepth(), model.getVolumeData());
                view.setTFData(model.getTFData());

                modelRotation = glm::vec3(0.0f);

                if (meta.name.find("Male Head") != std::string::npos)
                    settings.volumeScale = glm::vec3(1.0f);
                else if (meta.name.find("Carp") != std::string::npos)
                    settings.volumeScale = glm::vec3(1.0f, 1.0f, 2.0f);
                else if (meta.name.find("Engine") != std::string::npos)
                    settings.volumeScale = glm::vec3(1.0f, 1.0f, 0.5f);
                else
                    settings.volumeScale = glm::vec3(1.0f);
            }
        }

        void run()
        {
            SDL_Event event;
            uint64_t lastTime = SDL_GetTicks();

            while (isRunning)
            {
                uint64_t now = SDL_GetTicks();
                float dt = (now - lastTime) / 1000.0f;
                lastTime = now;
                if (dt > 0.1f)
                    dt = 0.1f;

                bool isInteracting = false;

                while (SDL_PollEvent(&event))
                {
                    ImGui_ImplSDL3_ProcessEvent(&event);
                    if (event.type == SDL_EVENT_QUIT)
                        isRunning = false;
                    if (event.type == SDL_EVENT_WINDOW_RESIZED)
                        view.handleResize(event.window.data1, event.window.data2);
                    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
                    {
                        mouseCaptured = !mouseCaptured;
                        SDL_SetWindowRelativeMouseMode(view.getWindow(), mouseCaptured);
                    }
                    if (mouseCaptured && event.type == SDL_EVENT_MOUSE_MOTION)
                    {
                        camera.rotateYaw(event.motion.xrel * 0.1f);
                        camera.rotatePitch(-event.motion.yrel * 0.1f);
                        isInteracting = true;
                    }
                }

                const bool *keys = SDL_GetKeyboardState(nullptr);
                float speed = 2.0f * dt;
                float rotSpeed = 100.0f * dt; // Скорость вращения модели

                if (mouseCaptured)
                {
                    bool moved = false;
                    if (keys[SDL_SCANCODE_W])
                    {
                        camera.moveForward(speed);
                        moved = true;
                    }
                    if (keys[SDL_SCANCODE_S])
                    {
                        camera.moveForward(-speed);
                        moved = true;
                    }
                    if (keys[SDL_SCANCODE_A])
                    {
                        camera.moveRight(speed);
                        moved = true;
                    }
                    if (keys[SDL_SCANCODE_D])
                    {
                        camera.moveRight(-speed);
                        moved = true;
                    }
                    if (keys[SDL_SCANCODE_SPACE])
                    {
                        camera.moveUp(-speed);
                        moved = true;
                    }
                    if (keys[SDL_SCANCODE_LSHIFT])
                    {
                        camera.moveUp(speed);
                        moved = true;
                    }
                    if (moved)
                        isInteracting = true;
                }
                else
                {
                    // Управление вращением модели
                    // Y - Влево/Вправо
                    if (keys[SDL_SCANCODE_LEFT])
                    {
                        modelRotation.y -= rotSpeed;
                        isInteracting = true;
                    }
                    if (keys[SDL_SCANCODE_RIGHT])
                    {
                        modelRotation.y += rotSpeed;
                        isInteracting = true;
                    }

                    // X - Вверх/Вниз
                    if (keys[SDL_SCANCODE_UP])
                    {
                        modelRotation.x -= rotSpeed;
                        isInteracting = true;
                    }
                    if (keys[SDL_SCANCODE_DOWN])
                    {
                        modelRotation.x += rotSpeed;
                        isInteracting = true;
                    }

                    // Z - PageUp/PageDown
                    if (keys[SDL_SCANCODE_PAGEUP])
                    {
                        modelRotation.z -= rotSpeed;
                        isInteracting = true;
                    }
                    if (keys[SDL_SCANCODE_PAGEDOWN])
                    {
                        modelRotation.z += rotSpeed;
                        isInteracting = true;
                    }
                }

                if (settings.dynamicQuality)
                    settings.stepSize = isInteracting ? settings.lowQualityStep : settings.highQualityStep;
                else
                    settings.stepSize = settings.highQualityStep;

                TFPreset oldPreset = currentPreset;
                VolumeMetadata loadReq;
                loadReq.width = 0;

                bool tfChanged = view.renderUI(settings, currentPreset, 1.0f / dt, presets, &loadReq, &modelRotation);

                if (tfChanged)
                {
                    model.updateTransferFunction(currentPreset);
                    view.setTFData(model.getTFData());
                }

                if (loadReq.width != 0)
                    loadVolume(loadReq);

                view.renderFrame(camera, settings, modelRotation, fragShaderPath);
            }
            view.cleanup();
        }
    };
}