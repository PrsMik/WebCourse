module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "backends/imgui_impl_sdl3.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <string>


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
        float modelRotY = 0.0f;
        TFPreset currentPreset = TFPreset::Bone;

        std::string fragShaderPath;

    public:
        Presenter(int w, int h, int d)
            : model(w, h, d),
              camera(glm::vec3(0, 0, 3.5f), glm::vec3(0, 0, 0))
        {
            fragShaderPath = DATA_DIR "/shaders/FragmentShader.glsl";
        }

        bool initialize(const std::string &volumePath)
        {
            if (!view.init(1280, 720, "MVP Raymarching"))
                return false;

            std::cout << "Loading volume from: " << volumePath << std::endl;
            if (!model.loadRawData(volumePath))
            {
                std::cerr << "CRITICAL ERROR: Failed to load volume file!" << std::endl;
                return false;
            }
            model.updateTransferFunction(currentPreset);

            view.setVolumeData(model.getWidth(), model.getHeight(), model.getDepth(), model.getVolumeData());
            view.setTFData(model.getTFData());

            return true;
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

                // Сбрасываем флаг взаимодействия каждый кадр
                bool isInteracting = false;

                while (SDL_PollEvent(&event))
                {
                    ImGui_ImplSDL3_ProcessEvent(&event);
                    if (event.type == SDL_EVENT_QUIT)
                        isRunning = false;

                    if (event.type == SDL_EVENT_WINDOW_RESIZED)
                    {
                        view.handleResize(event.window.data1, event.window.data2);
                    }

                    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
                    {
                        mouseCaptured = !mouseCaptured;
                        SDL_SetWindowRelativeMouseMode(view.getWindow(), mouseCaptured);
                    }

                    if (mouseCaptured && event.type == SDL_EVENT_MOUSE_MOTION)
                    {
                        camera.rotateYaw(event.motion.xrel * 0.1f);
                        camera.rotatePitch(-event.motion.yrel * 0.1f);
                        // Мышь двигается — значит взаимодействуем
                        isInteracting = true;
                    }
                }

                const bool *keys = SDL_GetKeyboardState(nullptr);
                float speed = 2.0f * dt;

                if (mouseCaptured)
                {
                    bool moved = false; // Локальный флаг движения

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

                    // ИСПРАВЛЕНИЕ: isInteracting ставится true ТОЛЬКО если было реальное движение
                    if (moved)
                        isInteracting = true;
                }
                else
                {
                    // Вращение модели стрелками
                    if (keys[SDL_SCANCODE_LEFT])
                    {
                        modelRotY -= 50.0f * dt;
                        isInteracting = true;
                    }
                    if (keys[SDL_SCANCODE_RIGHT])
                    {
                        modelRotY += 50.0f * dt;
                        isInteracting = true;
                    }
                }

                // ИСПРАВЛЕНИЕ: Логика выбора шага
                if (settings.dynamicQuality)
                {
                    // Если включена динамика: при движении LQ, в покое HQ
                    settings.stepSize = isInteracting ? settings.lowQualityStep : settings.highQualityStep;
                }
                else
                {
                    // Если динамика выключена: всегда используем значение из слайдера HQ (основного)
                    settings.stepSize = settings.highQualityStep;
                }

                TFPreset oldPreset = currentPreset;
                if (view.renderUI(settings, currentPreset, 1.0f / dt))
                {
                    if (oldPreset != currentPreset)
                    {
                        model.updateTransferFunction(currentPreset);
                        view.setTFData(model.getTFData());
                    }
                }

                view.renderFrame(camera, settings, modelRotY, fragShaderPath);
            }

            view.cleanup();
        }
    };
}