module;

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl3.h"
#include "imgui.h"
#include <SDL3/SDL.h>
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

// Вершины куба остаются без изменений, пропускаем для краткости...
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

        // Вспомогательные функции loadShader и readFile остаются без изменений...
        GLuint loadShader(const std::string &vertexSrc, const std::string &fragmentSrc)
        {
            auto compile = [](GLenum type, const std::string &src) -> GLuint
            {
                GLuint shader = glCreateShader(type);
                const char *c_str = src.c_str();
                glShaderSource(shader, 1, &c_str, nullptr);
                glCompileShader(shader);
                // Проверка ошибок компиляции...
                GLint success;
                glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
                if (!success)
                {
                    char l[512];
                    glGetShaderInfoLog(shader, 512, 0, l);
                    std::cerr << "Shader Error: " << l << std::endl;
                }
                return shader;
            };
            GLuint vs = compile(GL_VERTEX_SHADER, vertexSrc);
            GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentSrc);
            GLuint prog = glCreateProgram();
            glAttachShader(prog, vs);
            glAttachShader(prog, fs);
            glLinkProgram(prog);
            glDeleteShader(vs);
            glDeleteShader(fs);
            return prog;
        }

        std::string readFile(const std::string &path)
        {
            std::ifstream file(path);
            if (!file.is_open())
                return "";
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }

    public:
        Renderer() = default;

        bool init(int w, int h, const char *title)
        {
            currentWidth = w;
            currentHeight = h;
            if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
                return false;
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            window = SDL_CreateWindow(title, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
            context = SDL_GL_CreateContext(window);
            SDL_GL_MakeCurrent(window, context);
            SDL_GL_SetSwapInterval(0); // VSync off
            if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
                return false;

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

        // ИСПРАВЛЕНИЕ: Добавлен интерфейс настройки шага
        bool renderUI(RenderSettings &settings, TFPreset &currentPreset, float fps)
        {
            bool changed = false;
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("MVP Raymarching Settings");
            ImGui::Text("FPS: %.1f", fps);

            // Пресеты
            int p = (int)currentPreset;
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

            // Настройка качества
            ImGui::Text("Quality Settings");
            ImGui::Checkbox("Dynamic Quality (Auto-reduce on move)", &settings.dynamicQuality);

            // Слайдер для основного шага (он же highQualityStep)
            // Чем меньше шаг, тем выше качество и ниже FPS
            // Диапазон от 0.001 (супер качество) до 0.02 (черновик)
            ImGui::SliderFloat("Target Step Size", &settings.highQualityStep, 0.001f, 0.02f, "%.4f");

            if (settings.dynamicQuality)
            {
                // Показываем, какой шаг используется прямо сейчас
                ImGui::TextColored(ImVec4(0.5, 0.5, 0.5, 1), "Active Step: %.4f", settings.stepSize);
            }

            ImGui::Separator();
            ImGui::SliderFloat("Threshold", &settings.densityThreshold, 0.0f, 1.0f);
            ImGui::SliderFloat("Brightness", &settings.opacityMultiplier, 0.1f, 5.0f);
            ImGui::SliderFloat3("Light Dir", glm::value_ptr(settings.lightDir), -1.0f, 1.0f);
            ImGui::End();

            ImGui::Render();
            return changed;
        }

        void renderFrame(const Camera &cam, const RenderSettings &settings, float modelRotY, const std::string &fragPath)
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
            glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(modelRotY), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            glm::mat4 mvp = projection * view * model;

            glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(locCamPos, 1, glm::value_ptr(cam.getPosition()));
            glUniform3fv(locLightDir, 1, glm::value_ptr(glm::normalize(settings.lightDir)));

            // Передаем актуальный stepSize
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