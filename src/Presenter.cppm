module;

#include "backends/imgui_impl_sdl3.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

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

        AppState state = AppState::Intro;

        bool isRunning = true;
        bool mouseCaptured = false;
        uint64_t lockRequestTime = 0;
        glm::vec3 modelRotation = glm::vec3(0.0f, 0.0f, 0.0f);
        TFPreset currentPreset = TFPreset::Bone;
        std::string fragShaderPath;
        std::vector<VolumeMetadata> presets;
        uint64_t lastTime = 0;

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
            int startW = 1024;
            int startH = 768;

#ifdef __EMSCRIPTEN__
            startW = EM_ASM_INT({ return window.innerWidth; });
            startH = EM_ASM_INT({ return window.innerHeight; });
            if (startW < 1)
                startW = 800;
            if (startH < 1)
                startH = 600;

            if (startW < 800 || startH < 600)
            {
                settings.isMobile = true;
                std::cout << "Mobile detected: " << startW << "x" << startH << std::endl;
            }
#endif

            if (!view.init(startW, startH, "Volumetric Raymarching"))
                return false;

            loadVolume(presets[0]);
            lastTime = SDL_GetTicks();
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

        void update()
        {
            SDL_Event event;

            while (SDL_PollEvent(&event))
            {
                ImGui_ImplSDL3_ProcessEvent(&event);
                if (event.type == SDL_EVENT_QUIT)
                    isRunning = false;
                if (event.type == SDL_EVENT_WINDOW_RESIZED)
                    view.handleResize(event.window.data1, event.window.data2);

                if (state == AppState::Running && !settings.isMobile)
                {
                    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
                    {
                        if (!mouseCaptured && !ImGui::GetIO().WantCaptureMouse)
                        {
                            SDL_SetWindowRelativeMouseMode(view.getWindow(), true);
                            lockRequestTime = SDL_GetTicks();
                        }
                    }
                    if (event.type == SDL_EVENT_KEY_DOWN && (event.key.key == SDLK_ESCAPE))
                    {
                        SDL_SetWindowRelativeMouseMode(view.getWindow(), false);
                        mouseCaptured = false;
                    }
                    if (mouseCaptured && event.type == SDL_EVENT_MOUSE_MOTION)
                    {
                        camera.rotateYaw(event.motion.xrel * 0.2f);
                        camera.rotatePitch(-event.motion.yrel * 0.2f);
                    }
                }
            }

            if (state == AppState::Intro)
            {
                if (view.renderIntroScreen(settings))
                {
                    state = AppState::Running;
                    lastTime = SDL_GetTicks();
                }
                return;
            }

            uint64_t now = SDL_GetTicks();
            float dt = (now - lastTime) / 1000.0f;
            lastTime = now;
            if (dt > 0.1f)
                dt = 0.1f;

#ifdef __EMSCRIPTEN__
            EmscriptenPointerlockChangeEvent plStatus;
            emscripten_get_pointerlock_status(&plStatus);
            bool isBrowserLocked = plStatus.isActive;
#else
            bool isBrowserLocked = SDL_GetWindowRelativeMouseMode(view.getWindow());
#endif

            bool isSDLLocked = SDL_GetWindowRelativeMouseMode(view.getWindow());

            if (isBrowserLocked)
                mouseCaptured = true;
            else
            {
                mouseCaptured = false;
                if (isSDLLocked && (now - lockRequestTime > 500))
                {
                    SDL_SetWindowRelativeMouseMode(view.getWindow(), false);
                }
            }

            MobileInputData mobileInput = {{0, 0, 0}, {0, 0}};
            VolumeMetadata loadReq;
            loadReq.width = 0;

            bool tfChanged = view.renderUI(settings, currentPreset, dt > 0 ? 1.0f / dt : 0, presets, &loadReq, &modelRotation, &mobileInput);

            bool isInteracting = false;
            if (settings.isMobile)
            {
                if (abs(mobileInput.lookDir.x) > 0 || abs(mobileInput.lookDir.y) > 0)
                {
                    float sensitivity = 100.0f * dt;
                    camera.rotateYaw(mobileInput.lookDir.x * sensitivity);
                    camera.rotatePitch(mobileInput.lookDir.y * sensitivity);
                    isInteracting = true;
                }
            }

            const bool *keys = SDL_GetKeyboardState(nullptr);
            float speed = 2.0f * dt;
            float rotSpeed = 100.0f * dt;
            float fwd = 0.0f, right = 0.0f, up = 0.0f;

            if (keys[SDL_SCANCODE_W])
                fwd += 1.0f;
            if (keys[SDL_SCANCODE_S])
                fwd -= 1.0f;
            if (keys[SDL_SCANCODE_A])
                right += 1.0f;
            if (keys[SDL_SCANCODE_D])
                right -= 1.0f;
            if (keys[SDL_SCANCODE_SPACE])
                up -= 1.0f;
            if (keys[SDL_SCANCODE_LSHIFT])
                up += 1.0f;

            fwd += mobileInput.moveDir.z;
            right += mobileInput.moveDir.x;
            up += mobileInput.moveDir.y;

            if (abs(fwd) > 0.01f || abs(right) > 0.01f || abs(up) > 0.01f)
            {
                camera.moveForward(fwd * speed);
                camera.moveRight(right * speed);
                camera.moveUp(up * speed);
                isInteracting = true;
            }

            if (!mouseCaptured && !settings.isMobile)
            {
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
            }

            if (settings.dynamicQuality)
                settings.stepSize = isInteracting ? settings.lowQualityStep : settings.highQualityStep;
            else
                settings.stepSize = settings.highQualityStep;

            if (tfChanged)
            {
                model.updateTransferFunction(currentPreset);
                view.setTFData(model.getTFData());
            }
            if (loadReq.width != 0)
                loadVolume(loadReq);

            view.renderFrame(camera, settings, modelRotation, fragShaderPath);

            if (!isRunning)
            {
#ifdef __EMSCRIPTEN__
                emscripten_cancel_main_loop();
#endif
                view.cleanup();
            }
        }

        void run()
        {
#ifdef __EMSCRIPTEN__
            emscripten_set_main_loop_arg([](void *arg)
                                         { static_cast<Presenter *>(arg)->update(); }, this, 0, 1);
#else
            while (isRunning)
                update();
            view.cleanup();
#endif
        }
    };
}