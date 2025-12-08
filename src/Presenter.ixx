module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SDL3/SDL.h>
#include <string>
#include <iostream>
#include "backends/imgui_impl_sdl3.h"

export module App.Presenter;

import App.Types;
import App.Model;
import App.View;
import App.Camera;

export namespace App {

    class Presenter {
    private:
        VolumeModel model;
        Renderer view;
        Camera camera;
        RenderSettings settings;
        
        bool isRunning = true;
        bool mouseCaptured = false;
        float modelRotY = 0.0f;
        TFPreset currentPreset = TFPreset::Bone;

        // Константы
        const std::string fragShaderPath = DATA_DIR "/shaders/FragmentShader.glsl"; // Путь из define

    public:
        Presenter(int w, int h, int d) 
            : model(w, h, d), 
              camera(glm::vec3(0, 0, 3.5f), glm::vec3(0, 0, 0)) {}

        bool initialize(const std::string& volumePath) {
            if (!view.init(1280, 720, "MVP Raymarching")) return false;

            // 1. Загрузка данных в модель
            if (!model.loadRawData(volumePath)) {
                std::cerr << "Failed to load volume!" << std::endl;
                return false;
            }
            model.updateTransferFunction(currentPreset);

            // 2. Передача данных во View
            view.setVolumeData(model.getWidth(), model.getHeight(), model.getDepth(), model.getVolumeData());
            view.setTFData(model.getTFData());
            
            // 3. Компиляция шейдеров (во View) делается внутри init или отложенно
            return true;
        }

        void run() {
            SDL_Event event;
            uint64_t lastTime = SDL_GetTicks();

            while (isRunning) {
                // Time delta
                uint64_t now = SDL_GetTicks();
                float dt = (now - lastTime) / 1000.0f;
                lastTime = now;
                
                // --- 1. Input Handling (Presenter Logic) ---
                bool isInteracting = false;
                while (SDL_PollEvent(&event)) {
                    ImGui_ImplSDL3_ProcessEvent(&event);
                    if (event.type == SDL_EVENT_QUIT) isRunning = false;
                    if (event.type == SDL_EVENT_WINDOW_RESIZED) view.handleResize(event.window.data1, event.window.data2);
                    
                    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                        mouseCaptured = !mouseCaptured;
                        SDL_SetWindowRelativeMouseMode(view.getWindow(), mouseCaptured);
                    }
                    if (mouseCaptured && event.type == SDL_EVENT_MOUSE_MOTION) {
                         camera.rotateAroundTarget(event.motion.xrel * 0.1f, -event.motion.yrel * 0.1f);
                         isInteracting = true;
                    }
                }

                // Keyboard movement
                const bool* keys = SDL_GetKeyboardState(nullptr);
                float speed = 2.0f * dt;
                if (mouseCaptured) {
                    if (keys[SDL_SCANCODE_W]) camera.moveForward(speed);
                    if (keys[SDL_SCANCODE_S]) camera.moveForward(-speed);
                    // ... other keys ...
                    isInteracting = true;
                } else {
                    if (keys[SDL_SCANCODE_LEFT]) { modelRotY -= 50.0f * dt; isInteracting = true; }
                    if (keys[SDL_SCANCODE_RIGHT]) { modelRotY += 50.0f * dt; isInteracting = true; }
                }

                // Dynamic Quality Adjustment logic
                settings.stepSize = isInteracting ? settings.lowQualityStep : settings.highQualityStep;

                // --- 2. Update Model if UI changed ---
                TFPreset oldPreset = currentPreset;
                if (view.renderUI(settings, currentPreset, 1.0f / dt)) {
                     // Если UI вернул true, значит настройки обновились
                     if (oldPreset != currentPreset) {
                         model.updateTransferFunction(currentPreset);
                         view.setTFData(model.getTFData());
                     }
                }

                // --- 3. Render View ---
                // View рисует сцену + уже отрисованный внутри renderUI интерфейс
                view.renderFrame(camera, settings, modelRotY, fragShaderPath); 
            }
            
            view.cleanup();
        }
    };
}