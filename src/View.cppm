module;

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl3.h"
#include "imgui.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <fstream>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

export module App.View;

import App.Types;
import App.Camera;

// Вершины куба остаются те же...
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

        int currentWidth = 1280;
        int currentHeight = 720;

        GLint locMVP = -1, locModel = -1, locCamPos = -1, locLightDir = -1;
        GLint locStepSize = -1, locDensityThresh = -1, locOpacityMult = -1;

        char customPathBuf[256] = "";
        int customDims[3] = {256, 256, 256};
        bool customIs16Bit = false;

        GLuint loadShader(const std::string &v, const std::string &f)
        {
            auto compile = [](GLenum type, const std::string &src) -> GLuint
            {
                GLuint shader = glCreateShader(type);
                const char *c_str = src.c_str();
                glShaderSource(shader, 1, &c_str, nullptr);
                glCompileShader(shader);
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
            std::stringstream b;
            b << f.rdbuf();
            return b.str();
        }

    public:
        Renderer() = default;

        bool init(int w, int h, const char *title)
        {
            currentWidth = w;
            currentHeight = h;
            SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            window = SDL_CreateWindow(title, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
            context = SDL_GL_CreateContext(window);
            SDL_GL_MakeCurrent(window, context);
            SDL_GL_SetSwapInterval(0);
            gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui::StyleColorsDark();
            ImGui_ImplSDL3_InitForOpenGL(window, context);
            ImGui_ImplOpenGL3_Init("#version 460");
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
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
            glBindTexture(GL_TEXTURE_1D, tfTexture);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, (GLsizei)data.size(), 0, GL_RGBA, GL_FLOAT, data.data());
        }

        // Добавлен аргумент modelRotation для отображения в UI
        bool renderUI(RenderSettings &settings, TFPreset &currentPreset, float fps,
                      const std::vector<VolumeMetadata> &presets, VolumeMetadata *outLoadRequest,
                      glm::vec3 *modelRotation)
        {
            bool changed = false;
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
            ImGui::Begin("Settings");
            ImGui::Text("FPS: %.1f", fps);

            if (ImGui::BeginTabBar("Tabs"))
            {
                if (ImGui::BeginTabItem("Rendering"))
                {
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

                    ImGui::Separator();
                    ImGui::Checkbox("Dynamic Quality", &settings.dynamicQuality);
                    ImGui::SliderFloat("Step Size", &settings.highQualityStep, 0.001f, 0.02f, "%.4f");
                    if (settings.dynamicQuality)
                        ImGui::TextColored(ImVec4(0.5, 0.5, 0.5, 1), "Active Step: %.4f", settings.stepSize);

                    ImGui::Separator();
                    ImGui::SliderFloat("Threshold", &settings.densityThreshold, 0.0f, 1.0f);
                    ImGui::SliderFloat("Brightness", &settings.opacityMultiplier, 0.1f, 5.0f);
                    ImGui::SliderFloat3("Light Dir", glm::value_ptr(settings.lightDir), -1.0f, 1.0f);

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Data & Transform"))
                {
                    ImGui::Text("Presets:");
                    for (const auto &m : presets)
                    {
                        if (ImGui::Button((m.name + "##Btn").c_str()))
                            *outLoadRequest = m;
                    }

                    ImGui::Separator();
                    ImGui::Text("Manual Scaling:");
                    ImGui::SliderFloat3("Scale", glm::value_ptr(settings.volumeScale), 0.1f, 3.0f);
                    if (ImGui::Button("Reset Scale"))
                        settings.volumeScale = glm::vec3(1.0f);

                    // --- НОВОЕ: Слайдеры для вращения ---
                    ImGui::Separator();
                    ImGui::Text("Rotation (Arrows + PageUp/Dn):");
                    ImGui::SliderFloat("Rot X", &modelRotation->x, 0.0f, 360.0f);
                    ImGui::SliderFloat("Rot Y", &modelRotation->y, 0.0f, 360.0f);
                    ImGui::SliderFloat("Rot Z", &modelRotation->z, 0.0f, 360.0f);
                    if (ImGui::Button("Reset Rotation"))
                        *modelRotation = glm::vec3(0.0f);
                    // ------------------------------------

                    ImGui::Separator();
                    ImGui::Text("Custom RAW:");
                    ImGui::InputText("Path", customPathBuf, 256);
                    ImGui::InputInt3("Dims", customDims);
                    ImGui::Checkbox("16-bit", &customIs16Bit);
                    if (ImGui::Button("Load Custom"))
                    {
                        outLoadRequest->name = "Custom";
                        outLoadRequest->filePath = customPathBuf;
                        outLoadRequest->width = customDims[0];
                        outLoadRequest->height = customDims[1];
                        outLoadRequest->depth = customDims[2];
                        outLoadRequest->type = customIs16Bit ? DataType::UInt16 : DataType::UInt8;
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::End();
            ImGui::Render();
            return changed;
        }

        // Принимаем glm::vec3 modelRot
        void renderFrame(const Camera &cam, const RenderSettings &settings, glm::vec3 modelRot, const std::string &fragPath)
        {
            if (programID == 0)
            {
                std::string vertSrc = R"(#version 460 core
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
                glUseProgram(programID);
                glUniform1i(glGetUniformLocation(programID, "u_VolumeTexture"), 0);
                glUniform1i(glGetUniformLocation(programID, "u_TransferFunction"), 1);
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
            glBindTexture(GL_TEXTURE_1D, tfTexture);

            float aspect = (float)currentWidth / (float)currentHeight;
            if (currentHeight == 0)
                aspect = 1.0f;
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 100.0f);
            glm::mat4 view = cam.getViewMatrix();

            // --- НОВОЕ: Вращение по 3 осям ---
            glm::mat4 model = glm::mat4(1.0f);

            // Применяем вращения пользователя
            model = glm::rotate(model, glm::radians(modelRot.x), glm::vec3(1, 0, 0)); // Pitch
            model = glm::rotate(model, glm::radians(modelRot.y), glm::vec3(0, 1, 0)); // Yaw
            model = glm::rotate(model, glm::radians(modelRot.z), glm::vec3(0, 0, 1)); // Roll

            // Коррекция системы координат (Z-up -> Y-up)
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));

            // Масштабирование
            model = glm::scale(model, settings.volumeScale);
            // --------------------------------

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

        void handleResize(int w, int h)
        {
            currentWidth = w;
            currentHeight = h;
            glViewport(0, 0, w, h);
        }
        SDL_Window *getWindow() { return window; }
    };
}