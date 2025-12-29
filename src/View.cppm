module;

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl3.h"
#include "imgui.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <emscripten.h>
#else
#include <glad/gl.h>
#endif

export module App.View;

import App.Types;
import App.Camera;

static const float cubeVerts[] = {
    -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f};

export namespace App
{

    class Renderer
    {
    private:
        SDL_Window *window = nullptr;
        SDL_GLContext context = nullptr;
        GLuint programID = 0;
        GLuint vao = 0, vbo = 0;
        GLuint volumeTexture = 0;
        GLuint tfTexture = 0;

        int currentPixelWidth = 1280;
        int currentPixelHeight = 720;
        float currentUiScale = 1.0f;

        SDL_FingerID leftJoyFinger = 0;
        SDL_FingerID rightJoyFinger = 0;

        GLint locMVP = -1, locModel = -1, locCamPos = -1, locLightDir = -1;
        GLint locStepSize = -1, locDensityThresh = -1, locOpacityMult = -1;
        GLint locVolTex = -1, locTF = -1;

        void drawJoystickVisuals(ImDrawList *draw_list, float radius, ImVec2 centerPos, glm::vec2 val)
        {
            draw_list->AddCircleFilled(centerPos, radius, IM_COL32(255, 255, 255, 50));
            draw_list->AddCircle(centerPos, radius, IM_COL32(255, 255, 255, 100), 0, 2.0f);
            ImVec2 knobPos = ImVec2(centerPos.x + val.x * radius, centerPos.y + val.y * radius);
            bool isActive = (glm::length(val) > 0.01f);
            draw_list->AddCircleFilled(knobPos, radius * 0.4f, isActive ? IM_COL32(100, 200, 255, 200) : IM_COL32(200, 200, 200, 200));
        }

        GLuint loadShader(const std::string &v, const std::string &f)
        {
            auto compile = [](GLenum type, const std::string &src) -> GLuint
            {
                GLuint shader = glCreateShader(type);
                const char *c_str = src.c_str();
                glShaderSource(shader, 1, &c_str, nullptr);
                glCompileShader(shader);
                GLint success;
                glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
                if (!success)
                {
                    char infoLog[512];
                    glGetShaderInfoLog(shader, 512, NULL, infoLog);
                    std::cerr << "SHADER ERR:\n"
                              << infoLog << std::endl;
                }
                return shader;
            };
            GLuint vs = compile(GL_VERTEX_SHADER, v);
            GLuint fs = compile(GL_FRAGMENT_SHADER, f);
            GLuint p = glCreateProgram();
            glAttachShader(p, vs);
            glAttachShader(p, fs);
            glLinkProgram(p);
            glDeleteShader(vs);
            glDeleteShader(fs);
            return p;
        }
        std::string readFile(const std::string &p)
        {
            std::ifstream f(p);
            if (!f.is_open())
                return "";
            std::stringstream b;
            b << f.rdbuf();
            return b.str();
        }

    public:
        Renderer() = default;

        bool init(int w, int h, const char *title)
        {
            SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

#ifdef __EMSCRIPTEN__
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
            EM_ASM({
                var canvas = Module.canvas;
                if (canvas)
                {
                    canvas.style.width = "100%";
                    canvas.style.height = "100%";
                    canvas.style.display = "block";
                    canvas.style.position = "absolute";
                    canvas.style.top = "0";
                    canvas.style.left = "0";
                }
            });
#else
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

            window = SDL_CreateWindow(title, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
            context = SDL_GL_CreateContext(window);
            SDL_GL_MakeCurrent(window, context);
            SDL_GL_SetSwapInterval(0);

            SDL_GetWindowSizeInPixels(window, &currentPixelWidth, &currentPixelHeight);
            glViewport(0, 0, currentPixelWidth, currentPixelHeight);

#ifndef __EMSCRIPTEN__
            gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
#endif

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui::StyleColorsDark();
            ImGui_ImplSDL3_InitForOpenGL(window, context);

#ifdef __EMSCRIPTEN__
            ImGui_ImplOpenGL3_Init("#version 300 es");
#else
            ImGui_ImplOpenGL3_Init("#version 460");
#endif

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

            currentUiScale = 1.0f;
#ifdef __EMSCRIPTEN__
            if (w < 800)
                currentUiScale = 1.3f;
#endif
            ImGui::GetIO().FontGlobalScale = currentUiScale;
            ImGui::GetStyle().ScaleAllSizes(currentUiScale);

            return true;
        }

        void cleanup()
        {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
            if (vao)
                glDeleteVertexArrays(1, &vao);
            if (vbo)
                glDeleteBuffers(1, &vbo);
            if (volumeTexture)
                glDeleteTextures(1, &volumeTexture);
            SDL_GL_DestroyContext(context);
            SDL_DestroyWindow(window);
            SDL_Quit();
        }

        void setVolumeData(int w, int h, int d, const std::vector<Voxel> &data)
        {
            if (volumeTexture)
                glDeleteTextures(1, &volumeTexture);
            glGenTextures(1, &volumeTexture);
            glBindTexture(GL_TEXTURE_3D, volumeTexture);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, w, h, d, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
        }

        void setTFData(const std::vector<glm::vec4> &data)
        {
            if (tfTexture)
                glDeleteTextures(1, &tfTexture);
            glGenTextures(1, &tfTexture);
            glBindTexture(GL_TEXTURE_2D, tfTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            std::vector<uint8_t> byteData;
            byteData.reserve(data.size() * 4);
            for (const auto &c : data)
            {
                byteData.push_back((uint8_t)(std::clamp(c.r, 0.0f, 1.0f) * 255.0f));
                byteData.push_back((uint8_t)(std::clamp(c.g, 0.0f, 1.0f) * 255.0f));
                byteData.push_back((uint8_t)(std::clamp(c.b, 0.0f, 1.0f) * 255.0f));
                byteData.push_back((uint8_t)(std::clamp(c.a, 0.0f, 1.0f) * 255.0f));
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)data.size(), 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, byteData.data());
        }

        // --- НОВОЕ: Отрисовка стартового экрана ---
        bool renderIntroScreen(RenderSettings &settings)
        {
            // Обработка ресайза
            int pixelW, pixelH;
            SDL_GetWindowSizeInPixels(window, &pixelW, &pixelH);
            if (pixelW != currentPixelWidth || pixelH != currentPixelHeight)
            {
                currentPixelWidth = pixelW;
                currentPixelHeight = pixelH;
                glViewport(0, 0, currentPixelWidth, currentPixelHeight);
            }

            // Очистка экрана (чтобы не было мусора)
            glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            bool startClicked = false;

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(300 * currentUiScale, 280 * currentUiScale));

            if (ImGui::Begin("Welcome", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
            {
                ImGui::TextWrapped("Volumetric Raymarching Demo");
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0, 10));

                ImGui::Text("Device Mode:");
                bool isMob = settings.isMobile;
                if (ImGui::RadioButton("Desktop (High Quality)", !isMob))
                {
                    settings.isMobile = false;
                    settings.highQualityStep = 0.004f;
                    settings.dynamicQuality = true;
                }
                if (ImGui::RadioButton("Mobile (Low Quality)", isMob))
                {
                    settings.isMobile = true;
                    settings.highQualityStep = 0.02f;
                    settings.stepSize = 0.02f;
                    settings.dynamicQuality = true;
                }

                ImGui::Dummy(ImVec2(0, 20));

                // Центрируем кнопку
                float btnWidth = 150.0f * currentUiScale;
                float avail = ImGui::GetContentRegionAvail().x;
                ImGui::SetCursorPosX((avail - btnWidth) * 0.5f);

                if (ImGui::Button("START RENDER", ImVec2(btnWidth, 50 * currentUiScale)))
                {
                    startClicked = true;
                }

                ImGui::End();
            }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);

            return startClicked;
        }

        void updateAndDrawJoysticks(ImVec2 leftCenter, float leftRadius,
                                    ImVec2 rightCenter, float rightRadius,
                                    MobileInputData *outInput, ImVec2 displaySize)
        {
            int numDevices = 0;
            SDL_TouchID *devices = SDL_GetTouchDevices(&numDevices);
            if (numDevices == 0)
            {
                drawJoystickVisuals(ImGui::GetWindowDrawList(), leftRadius, leftCenter, glm::vec2(0.0f));
                drawJoystickVisuals(ImGui::GetWindowDrawList(), rightRadius, rightCenter, glm::vec2(0.0f));
                return;
            }

            bool leftFound = false, rightFound = false;
            glm::vec2 leftVal(0.0f), rightVal(0.0f);

            for (int i = 0; i < numDevices; ++i)
            {
                int numFingers = 0;
                SDL_Finger **fingers = SDL_GetTouchFingers(devices[i], &numFingers);

                for (int f = 0; f < numFingers; ++f)
                {
                    SDL_Finger *finger = fingers[f];
                    float px = finger->x * displaySize.x;
                    float py = finger->y * displaySize.y;

                    if (leftJoyFinger != 0 && finger->id == leftJoyFinger)
                    {
                        leftFound = true;
                        glm::vec2 delta(px - leftCenter.x, py - leftCenter.y);
                        if (glm::length(delta) > leftRadius)
                            delta = glm::normalize(delta) * leftRadius;
                        leftVal = delta / leftRadius;
                    }
                    else if (leftJoyFinger == 0)
                    {
                        glm::vec2 delta(px - leftCenter.x, py - leftCenter.y);
                        if (glm::length(delta) < leftRadius * 1.5f)
                        {
                            leftJoyFinger = finger->id;
                            leftFound = true;
                            if (glm::length(delta) > leftRadius)
                                delta = glm::normalize(delta) * leftRadius;
                            leftVal = delta / leftRadius;
                        }
                    }

                    if (rightJoyFinger != 0 && finger->id == rightJoyFinger)
                    {
                        rightFound = true;
                        glm::vec2 delta(px - rightCenter.x, py - rightCenter.y);
                        if (glm::length(delta) > rightRadius)
                            delta = glm::normalize(delta) * rightRadius;
                        rightVal = delta / rightRadius;
                    }
                    else if (rightJoyFinger == 0)
                    {
                        glm::vec2 delta(px - rightCenter.x, py - rightCenter.y);
                        if (glm::length(delta) < rightRadius * 1.5f)
                        {
                            rightJoyFinger = finger->id;
                            rightFound = true;
                            if (glm::length(delta) > rightRadius)
                                delta = glm::normalize(delta) * rightRadius;
                            rightVal = delta / rightRadius;
                        }
                    }
                }
            }

            if (!leftFound)
                leftJoyFinger = 0;
            if (!rightFound)
                rightJoyFinger = 0;

            outInput->moveDir.z = -leftVal.y;
            outInput->moveDir.x = leftVal.x;
            outInput->lookDir.x = rightVal.x;
            outInput->lookDir.y = rightVal.y;

            drawJoystickVisuals(ImGui::GetWindowDrawList(), leftRadius, leftCenter, leftVal);
            drawJoystickVisuals(ImGui::GetWindowDrawList(), rightRadius, rightCenter, rightVal);
        }

        bool renderUI(RenderSettings &settings, TFPreset &currentPreset, float fps,
                      const std::vector<VolumeMetadata> &presets, VolumeMetadata *outLoadRequest,
                      glm::vec3 *modelRotation, MobileInputData *outMobileInput)
        {
            bool changed = false;
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            ImVec2 displaySize = ImGui::GetIO().DisplaySize;

            if (settings.isMobile)
            {
                ImGui::SetNextWindowSizeConstraints(ImVec2(displaySize.x * 0.9f, 100), ImVec2(displaySize.x, displaySize.y * 0.65f));
                ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
            }

            ImGui::Begin("Settings");
            ImGui::Text("FPS: %.1f", fps);

            if (ImGui::BeginTabBar("Tabs"))
            {
                if (ImGui::BeginTabItem("UI & Control"))
                {
                    bool forceMobile = settings.isMobile;
                    if (ImGui::Checkbox("Mobile Mode", &forceMobile))
                    {
                        settings.isMobile = forceMobile;
                        if (settings.isMobile)
                        {
                            settings.highQualityStep = 0.02f;
                            settings.stepSize = 0.02f;
                            settings.dynamicQuality = true;
                        }
                        else
                        {
                            settings.highQualityStep = 0.004f;
                        }
                    }

                    if (settings.isMobile)
                    {
                        ImGui::Checkbox("Show Joysticks", &settings.showOnScreenControls);
                    }

                    ImGui::Separator();
                    if (ImGui::SliderFloat("UI Scale", &settings.uiScale, 0.5f, 3.0f, "%.1f"))
                    {
                        float ratio = settings.uiScale / currentUiScale;
                        ImGui::GetStyle().ScaleAllSizes(ratio);
                        ImGui::GetIO().FontGlobalScale = settings.uiScale;
                        currentUiScale = settings.uiScale;
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Rendering"))
                {
                    ImGui::Text("Quality");
                    ImGui::SliderFloat("Step Size", &settings.highQualityStep, 0.001f, 0.02f, "%.4f");
                    ImGui::Checkbox("Dynamic Quality", &settings.dynamicQuality);
                    ImGui::Separator();

                    ImGui::Text("Appearance");
                    ImGui::SliderFloat("Threshold", &settings.densityThreshold, 0.0f, 1.0f);
                    ImGui::SliderFloat("Opacity", &settings.opacityMultiplier, 0.1f, 10.0f);
                    ImGui::Separator();

                    ImGui::Text("Light");
                    ImGui::SliderFloat3("Direction", glm::value_ptr(settings.lightDir), -1.0f, 1.0f);
                    ImGui::Separator();

                    int p = (int)currentPreset;
                    ImGui::Text("Transfer Function:");
                    if (ImGui::RadioButton("Bone", &p, 0))
                    {
                        currentPreset = TFPreset::Bone;
                        changed = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton("Muscle", &p, 1))
                    {
                        currentPreset = TFPreset::Muscle;
                        changed = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton("Rainbow", &p, 2))
                    {
                        currentPreset = TFPreset::Rainbow;
                        changed = true;
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Data"))
                {
                    ImGui::Text("Presets:");
                    for (const auto &m : presets)
                    {
                        if (ImGui::Button((m.name + "##Btn").c_str()))
                            *outLoadRequest = m;
                    }
                    ImGui::Separator();
                    ImGui::SliderFloat3("Scale", glm::value_ptr(settings.volumeScale), 0.1f, 3.0f);
                    if (ImGui::Button("Reset Scale"))
                        settings.volumeScale = glm::vec3(1.0f);
                    ImGui::Separator();
                    ImGui::SliderFloat("Rot X", &modelRotation->x, 0.0f, 360.0f);
                    ImGui::SliderFloat("Rot Y", &modelRotation->y, 0.0f, 360.0f);
                    ImGui::SliderFloat("Rot Z", &modelRotation->z, 0.0f, 360.0f);
                    if (ImGui::Button("Reset Rot"))
                        *modelRotation = glm::vec3(0.0f);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::End();

            if (settings.isMobile && settings.showOnScreenControls)
            {
                float joySize = (displaySize.x < 500) ? 140.0f : 200.0f;
                joySize *= settings.uiScale;
                float joyRadius = joySize * 0.35f;
                float padding = 20.0f * settings.uiScale;

                ImVec2 lCenter = ImVec2(padding + joySize / 2, displaySize.y - padding - joySize / 2);
                ImVec2 rCenter = ImVec2(displaySize.x - padding - joySize / 2, displaySize.y - padding - joySize / 2);

                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize(displaySize);
                ImGui::SetNextWindowBgAlpha(0.0f);
                ImGui::Begin("JoyOverlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);

                updateAndDrawJoysticks(lCenter, joyRadius, rCenter, joyRadius, outMobileInput, displaySize);

                ImGui::End();

                float btnW = joySize * 0.8f;
                ImGui::SetNextWindowPos(ImVec2((displaySize.x - btnW) / 2, displaySize.y - (80 * settings.uiScale)));
                ImGui::SetNextWindowSize(ImVec2(btnW, 60 * settings.uiScale));
                ImGui::SetNextWindowBgAlpha(0.0f);
                ImGui::Begin("Elev", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

                ImGui::Button("Up", ImVec2(btnW / 2 - 5, 40 * settings.uiScale));
                if (ImGui::IsItemActive())
                    outMobileInput->moveDir.y = 1.0f;

                ImGui::SameLine();

                ImGui::Button("Dn", ImVec2(btnW / 2 - 5, 40 * settings.uiScale));
                if (ImGui::IsItemActive())
                    outMobileInput->moveDir.y = -1.0f;

                ImGui::End();
            }

            ImGui::Render();
            return changed;
        }

        void renderFrame(const Camera &cam, const RenderSettings &settings, glm::vec3 modelRot, const std::string &fragPath)
        {
            if (programID == 0)
            {
                std::string vertSrc =
#ifdef __EMSCRIPTEN__
                    "#version 300 es\n"
#else
                    "#version 460 core\n"
#endif
                    R"(
                    layout (location = 0) in vec3 aPos;
                    out vec3 vLocalPos;
                    uniform mat4 MVP;
                    void main() { vLocalPos = aPos; gl_Position = MVP * vec4(aPos, 1.0); }
                )";

                std::string fragSrc = readFile(fragPath);
                if (fragSrc.empty())
                {
                    glClear(GL_COLOR_BUFFER_BIT);
                    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                    SDL_GL_SwapWindow(window);
                    return;
                }
                programID = loadShader(vertSrc, fragSrc);
                if (programID == 0)
                    return;

                locMVP = glGetUniformLocation(programID, "MVP");
                locModel = glGetUniformLocation(programID, "u_ModelMatrix");
                locCamPos = glGetUniformLocation(programID, "u_CameraPos");
                locLightDir = glGetUniformLocation(programID, "u_LightDir");
                locStepSize = glGetUniformLocation(programID, "u_StepSize");
                locDensityThresh = glGetUniformLocation(programID, "u_DensityThreshold");
                locOpacityMult = glGetUniformLocation(programID, "u_OpacityMultiplier");
                locVolTex = glGetUniformLocation(programID, "u_VolumeTexture");
                locTF = glGetUniformLocation(programID, "u_TransferFunction");

                glUseProgram(programID);
                glUniform1i(locVolTex, 0);
                glUniform1i(locTF, 1);
            }

            int pixelW, pixelH;
            SDL_GetWindowSizeInPixels(window, &pixelW, &pixelH);
            if (pixelW != currentPixelWidth || pixelH != currentPixelHeight)
            {
                currentPixelWidth = pixelW;
                currentPixelHeight = pixelH;
                glViewport(0, 0, currentPixelWidth, currentPixelHeight);
            }

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(programID);
            glDisable(GL_CULL_FACE);
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, volumeTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, tfTexture);

            float aspect = (float)currentPixelWidth / (float)currentPixelHeight;
            if (currentPixelHeight == 0)
                aspect = 1.0f;

            glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 100.0f);
            glm::mat4 view = cam.getViewMatrix();
            glm::mat4 model = glm::mat4(1.0f);

            model = glm::rotate(model, glm::radians(modelRot.x), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(modelRot.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(modelRot.z), glm::vec3(0, 0, 1));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            model = glm::scale(model, settings.volumeScale);

            glm::mat4 mvp = projection * view * model;

            glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(locCamPos, 1, glm::value_ptr(cam.getPosition()));
            glUniform3fv(locLightDir, 1, glm::value_ptr(glm::normalize(settings.lightDir)));
            glUniform1f(locStepSize, settings.stepSize);
            glUniform1f(locDensityThresh, settings.densityThreshold);
            glUniform1f(locOpacityMult, settings.opacityMultiplier);

            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glEnable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);
        }

        void handleResize(int w, int h) {}
        SDL_Window *getWindow() { return window; }
    };
}