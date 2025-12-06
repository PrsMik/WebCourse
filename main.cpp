// --- START OF FILE main.cpp ---

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include <SDL3/SDL.h>
#include <algorithm> // Для std::max/min
#include <cstdlib>
#include <fstream>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

// --- IMGUI INCLUDES ---
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl3.h"
#include "imgui.h"

#include "src/Camera.hpp"
#include "src/cube.hpp"
#include "src/loadShader.cpp"

// --- НАСТРОЙКИ ДАННЫХ ---
// Убедитесь, что путь и размеры совпадают с вашим файлом!
#define VOLUME_FILE_PATH DATA_DIR "/vis_male_128x256x256_uint8.raw"
#define VOL_X 128
#define VOL_Y 256
#define VOL_Z 256

// Структура для одного вокселя (RGBA)
struct Voxel
{
    uint8_t r, g, b, a; // r,g,b - нормаль, a - плотность
};

// --- ПРЕСЕТЫ TF ---
enum TransferPreset
{
    TF_BONE,
    TF_MUSCLE,
    TF_RAINBOW
};

void UpdateTransferFunction(GLuint textureID, TransferPreset preset)
{
    std::vector<glm::vec4> tfData(256);

    for (int i = 0; i < 256; i++)
    {
        float t = i / 255.0f;
        glm::vec4 color(0.0f);

        if (preset == TF_BONE)
        {
            // Кости: убираем мягкие ткани, оставляем жесткую структуру
            if (t > 0.15f && t < 0.25f)
                color = glm::vec4(0.6f, 0.4f, 0.3f, 0.05f); // Кожа/жир (очень прозрачно)
            else if (t > 0.35f)
                color = glm::vec4(0.9f, 0.85f, 0.8f, 0.9f); // Кость
        }
        else if (preset == TF_MUSCLE)
        {
            // Мышцы: акцент на средние значения
            if (t > 0.1f)
                color = glm::vec4(0.6f, 0.1f, 0.1f, 0.05f);
            if (t > 0.25f)
                color = glm::vec4(0.8f, 0.4f, 0.3f, 0.6f); // Мышцы
            if (t > 0.5f)
                color = glm::vec4(1.0f, 1.0f, 1.0f, 0.9f); // Кость внутри
        }
        else if (preset == TF_RAINBOW)
        {
            // Отладка
            color.r = t;
            color.g = 1.0f - abs(t - 0.5f) * 2.0f;
            color.b = 1.0f - t;
            color.a = t * 0.5f;
        }
        tfData[i] = color;
    }

    glBindTexture(GL_TEXTURE_1D, textureID);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA, GL_FLOAT, tfData.data());
}

GLuint CreateTransferTexture()
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_1D, textureID);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_FLOAT, nullptr);
    return textureID;
}

// --- НОВАЯ ФУНКЦИЯ ЗАГРУЗКИ С ПРЕДРАСЧЕТОМ ГРАДИЕНТОВ ---
GLuint Load3DVolumeWithGradients(const char *filepath, int width, int height, int depth)
{
    std::cout << "Loading volume from: " << filepath << " [" << width << "x" << height << "x" << depth << "]" << std::endl;

    // 1. Читаем "сырую" плотность
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "ERROR: Could not open volume file: " << filepath << std::endl;
        return 0;
    }
    std::vector<uint8_t> densityData(std::istreambuf_iterator<char>(file), {});
    file.close();

    if (densityData.size() != width * height * depth)
    {
        std::cerr << "WARNING: File size mismatch! Expected " << width * height * depth
                  << " bytes, got " << densityData.size() << ". Check dimensions." << std::endl;
        // Если размер не совпадает, программа может упасть при доступе к массиву.
        // Для теста лучше вернуть 0 или попробовать продолжить на свой страх и риск.
    }

    // 2. Подготовка буфера для RGBA (Нормаль + Плотность)
    std::vector<Voxel> textureData(width * height * depth);

    std::cout << "Pre-computing gradients (Optimization step)... ";

    // 3. Расчет градиентов (Central Differences)
    for (int z = 0; z < depth; ++z)
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int index = x + y * width + z * width * height;
                uint8_t density = densityData[index];

                // Получаем значения соседей. Если на границе - берем текущее значение
                int x1 = (x > 0) ? densityData[index - 1] : density;
                int x2 = (x < width - 1) ? densityData[index + 1] : density;

                int y1 = (y > 0) ? densityData[index - width] : density;
                int y2 = (y < height - 1) ? densityData[index + width] : density;

                int z1 = (z > 0) ? densityData[index - width * height] : density;
                int z2 = (z < depth - 1) ? densityData[index + width * height] : density;

                // Вектор градиента
                glm::vec3 grad;
                grad.x = (float)(x1 - x2);
                grad.y = (float)(y1 - y2);
                grad.z = (float)(z1 - z2);

                // Упаковка нормали в [0, 255]
                // Нормаль [-1, 1] -> [0, 1] -> [0, 255]
                if (glm::length(grad) > 0.0f)
                {
                    grad = glm::normalize(grad);
                    grad = grad * 0.5f + 0.5f;
                }
                else
                {
                    grad = glm::vec3(0.5f, 0.5f, 0.5f); // "Нулевая" нормаль
                }

                textureData[index].r = (uint8_t)(grad.x * 255.0f);
                textureData[index].g = (uint8_t)(grad.y * 255.0f);
                textureData[index].b = (uint8_t)(grad.z * 255.0f);
                textureData[index].a = density; // Альфа-канал - это сама плотность
            }
        }
    }
    std::cout << "Done." << std::endl;

    // 4. Загрузка в OpenGL
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_3D, textureID);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // ВАЖНО: InternalFormat = GL_RGBA8, Format = GL_RGBA
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // На всякий случай
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData.data());

    return textureID;
}

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        return 1;

    // Установка OpenGL 4.6
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    int windowWidth = 1280;
    int windowHeight = 720;
    SDL_Window *mainWindow = SDL_CreateWindow("Optimized Raymarching", windowWidth, windowHeight,
                                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext context = SDL_GL_CreateContext(mainWindow);
    SDL_GL_MakeCurrent(mainWindow, context);
    SDL_GL_SetSwapInterval(0); // VSync OFF для замера реального FPS

    if (gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0)
        return 1;

    // IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForOpenGL(mainWindow, context);
    ImGui_ImplOpenGL3_Init("#version 460");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Загрузка
    GLuint programID = LoadShaders(DATA_DIR "/shaders/VertexShader.glsl", DATA_DIR "/shaders/FragmentShader.glsl");

    // !!! ИСПОЛЬЗУЕМ НОВУЮ ФУНКЦИЮ ЗАГРУЗКИ !!!
    GLuint volumeTexture = Load3DVolumeWithGradients(VOLUME_FILE_PATH, VOL_X, VOL_Y, VOL_Z);

    GLuint tfTexture = CreateTransferTexture();
    UpdateTransferFunction(tfTexture, TF_BONE);

    // Куб
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, CUBE_DATA_SIZE_BYTES, getCubeData(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

    Camera camera(glm::vec3(0.0f, 0.0f, 3.5f), glm::vec3(0.0f, 0.0f, 0.0f));

    // Uniform locations
    GLint locMVP = glGetUniformLocation(programID, "MVP");
    GLint locModel = glGetUniformLocation(programID, "u_ModelMatrix");
    GLint locCamPos = glGetUniformLocation(programID, "u_CameraPos");
    GLint locLightDir = glGetUniformLocation(programID, "u_LightDir");
    GLint locStepSize = glGetUniformLocation(programID, "u_StepSize");
    GLint locDensityThresh = glGetUniformLocation(programID, "u_DensityThreshold");
    GLint locOpacityMult = glGetUniformLocation(programID, "u_OpacityMultiplier");

    glUseProgram(programID);
    glUniform1i(glGetUniformLocation(programID, "u_VolumeTexture"), 0);
    glUniform1i(glGetUniformLocation(programID, "u_TransferFunction"), 1);

    bool isRunning = true;
    bool mouseHandle = false;
    SDL_Event event;
    float modelRotY = 0.0f;

    // Настройки сцены
    float uiThreshold = 0.20f; // Чуть выше, чтобы отсечь шум
    float uiOpacity = 1.0f;
    float uiLightDir[3] = {-0.5f, 0.5f, -1.0f}; // Свет спереди-сверху
    int currentPreset = 0;

    // Настройки качества (шаг)
    // Увеличили шаг для лучшей производительности
    float currentStepSize = 0.01f;
    float highQualityStep = 0.005f; // Статика
    float lowQualityStep = 0.015f;  // ~60 шагов (быстро при движении)
    bool isInteracting = false;

    while (isRunning)
    {
        isInteracting = false;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                isRunning = false;
            if (event.type == SDL_EVENT_WINDOW_RESIZED)
                glViewport(0, 0, event.window.data1, event.window.data2);
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
            {
                mouseHandle = !mouseHandle;
                SDL_SetWindowRelativeMouseMode(mainWindow, mouseHandle);
            }
            if (mouseHandle && event.type == SDL_EVENT_MOUSE_MOTION)
            {
                camera.rotateYaw(event.motion.xrel * 0.1f);
                camera.rotatePitch(-event.motion.yrel * 0.1f);
                isInteracting = true;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::Begin("Settings");
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("Samples per ray: %.0f", 1.0f / currentStepSize);
            ImGui::Separator();
            if (ImGui::RadioButton("Bone", &currentPreset, 0))
                UpdateTransferFunction(tfTexture, TF_BONE);
            ImGui::SameLine();
            if (ImGui::RadioButton("Muscle", &currentPreset, 1))
                UpdateTransferFunction(tfTexture, TF_MUSCLE);
            ImGui::SameLine();
            if (ImGui::RadioButton("Rainbow", &currentPreset, 2))
                UpdateTransferFunction(tfTexture, TF_RAINBOW);
            ImGui::Separator();
            ImGui::SliderFloat("Threshold", &uiThreshold, 0.0f, 1.0f);
            ImGui::SliderFloat("Brightness", &uiOpacity, 0.1f, 5.0f);
            ImGui::SliderFloat3("Light Dir", uiLightDir, -1.0f, 1.0f);
            ImGui::Separator();
            ImGui::SliderFloat("Step (Static)", &highQualityStep, 0.002f, 0.01f, "%.4f");
            ImGui::SliderFloat("Step (Move)", &lowQualityStep, 0.01f, 0.05f, "%.4f");
            ImGui::End();
        }

        // Движение камеры
        const bool *keys = SDL_GetKeyboardState(nullptr);
        float speed = 0.05f;
        if (mouseHandle)
        {
            if (keys[SDL_SCANCODE_W])
                camera.moveForward(speed);
            if (keys[SDL_SCANCODE_S])
                camera.moveForward(-speed);
            if (keys[SDL_SCANCODE_A])
                camera.moveRight(speed);
            if (keys[SDL_SCANCODE_D])
                camera.moveRight(-speed);
            if (keys[SDL_SCANCODE_SPACE])
                camera.moveUp(speed);
            if (keys[SDL_SCANCODE_LSHIFT])
                camera.moveUp(-speed);

            if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_A] ||
                keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_LSHIFT])
                isInteracting = true;
        }
        else
        {
            if (keys[SDL_SCANCODE_LEFT])
            {
                modelRotY -= 1.0f;
                isInteracting = true;
            }
            if (keys[SDL_SCANCODE_RIGHT])
            {
                modelRotY += 1.0f;
                isInteracting = true;
            }
        }

        currentStepSize = isInteracting ? lowQualityStep : highQualityStep;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Очистка буфера глубины тоже!

        glUseProgram(programID);

        // ВАЖНО: Выключаем смешивание для Raymarching, шейдер сам все смешивает
        glDisable(GL_BLEND);
        // Включаем тест глубины и отсечение задних граней, чтобы рисовать только "перед" куба
        glEnable(GL_DEPTH_TEST);
        // glEnable(GL_CULL_FACE);
        // glCullFace(GL_BACK); // Рисуем только переднюю грань куба

        // Биндинг текстур
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, volumeTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, tfTexture);

        float aspect = (float)io.DisplaySize.x / io.DisplaySize.y;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(modelRotY), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        glm::mat4 mvp = projection * view * model;

        glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(locCamPos, 1, glm::value_ptr(camera.getPosition()));
        glUniform3fv(locLightDir, 1, glm::value_ptr(glm::normalize(glm::make_vec3(uiLightDir))));
        glUniform1f(locStepSize, currentStepSize);
        glUniform1f(locDensityThresh, uiThreshold);
        glUniform1f(locOpacityMult, uiOpacity);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Включаем смешивание обратно для ImGui (иначе UI будет черным квадратом)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST); // UI рисуется поверх всего
        glDisable(GL_CULL_FACE);

        // Отрисовка UI
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(mainWindow);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteTextures(1, &volumeTexture);
    glDeleteTextures(1, &tfTexture);
    glDeleteProgram(programID);
    SDL_DestroyWindow(mainWindow);
    SDL_Quit();
    return 0;
}