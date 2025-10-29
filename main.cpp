#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include <GL/glew.h>  // Подключение GLEW
#include <SDL3/SDL.h> // Предполагаем, что вы используете SDL3
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <numbers>

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/trigonometric.hpp"
#include "shaders\loadShader.cpp"
#include "src/Camera.hpp"
#include "src/cube.hpp"

glm::vec4 getTranslatedVec(glm::vec4 vector, float xOffset,
                           float yOffset, float zOffset)
{
    glm::mat4 translationMatrix = glm::translate(glm::mat4(),
                                                 glm::vec3(xOffset,
                                                           yOffset,
                                                           zOffset));
    return translationMatrix * vector;
}

glm::vec4 getScaledVec(glm::vec4 vector, float xScale,
                       float yScale, float zScale)
{
    glm::mat4 scalingMatrix = glm::scale(glm::vec3(xScale,
                                                   yScale,
                                                   zScale));
    return scalingMatrix * vector;
}

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        std::cout << SDL_GetError();
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); // For 4x MSAA

    const SDL_DisplayMode *display = SDL_GetCurrentDisplayMode(SDL_GetDisplays(nullptr)[0]);
    int displayWidth = display->w;
    int displayHeight = display->h;

    float aspect = static_cast<float>(displayWidth) / displayHeight;

    int windowWidth = 800;
    int windowHeight = windowWidth / aspect;

    std::cout << displayWidth << "/" << displayHeight << "=" << aspect << "\n";

    SDL_Window *mainWindow = SDL_CreateWindow("Hello, OpenGL!", windowWidth, windowHeight,
                                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext context = SDL_GL_CreateContext(mainWindow);
    SDL_GL_MakeCurrent(mainWindow, context);
    SDL_GL_SetSwapInterval(1);

    glewExperimental = true;
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to init glew!";
        return 1;
    }

    bool mouseHandle = true;

    if (SDL_SetWindowRelativeMouseMode(mainWindow, mouseHandle)) // Используем SDL_TRUE для включения
    {
        std::cout << "Warning: Could not enable relative mouse mode: " << SDL_GetError() << "\n";
    }

    SDL_Event event;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    glDepthFunc(GL_LESS);

    glClearColor(0.0F, 0.0F, 0.4F, 0.0F);

    GLuint vertexArrayID;
    glGenVertexArrays(1, &vertexArrayID);
    glBindVertexArray(vertexArrayID);

    GLuint programID = LoadShaders(SHADERS_DIR "/VertexShader.glsl",
                                   SHADERS_DIR "/FragmentShader.glsl");

    GLuint matrixID = glGetUniformLocation(programID, "MVP");

    Camera camera(glm::vec3(4.0f, 0.0f, 0.0f), // Немного выше и дальше для лучшего обзора
                  glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, 1.0f, 0.0f));

    // Используем speed как "скорость перемещения"
    float speed = 0.2f;       // Немного увеличим для заметного эффекта
    float rotateSpeed = 1.2f; // Немного увеличим для заметного эффекта

    glm::mat4 projectionMartix = glm::perspective(glm::radians(45.0F), aspect, 0.1F, 100.0F);

    glm::mat4 viewMatrix = camera.getViewMatrix();

    glm::mat4 modelMatrix1 = glm::mat4(1.0F);

    // Матрица модели для второго куба (например, сдвинутого по оси X на -4.0)
    glm::mat4 modelMatrix2 = glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, -4.0f, -4.0f));

    glm::mat4 MVPMatrix = projectionMartix * viewMatrix * modelMatrix1;

    const GLfloat *firstVertexData = getCubeData();
    const GLfloat *firstColorData = getColorData();

    // Размер в байтах также можно получить из заголовка
    size_t vertexDataSize = CUBE_DATA_SIZE_BYTES;
    size_t colorDataSize = COLOR_DATA_SIZE_BYTES;

    // ... ваш код для GL (например, glBufferData)
    // glBufferData(GL_ARRAY_BUFFER, size_bytes, data, GL_STATIC_DRAW);

    GLuint firstVertexBuffer;
    GLuint firstColorBuffer;

    glGenBuffers(1, &firstVertexBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, firstVertexBuffer);

    glBufferData(GL_ARRAY_BUFFER,
                 vertexDataSize,
                 firstVertexData,
                 GL_STATIC_DRAW);

    glGenBuffers(1, &firstColorBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, firstColorBuffer);

    glBufferData(GL_ARRAY_BUFFER,
                 colorDataSize,
                 firstColorData,
                 GL_STATIC_DRAW);

    const float mouseSensitivity = 0.1f;

    float currentAngel = 0.0F;

    while (true)
    {
        const bool *key_states = SDL_GetKeyboardState(nullptr);

        if (key_states[SDL_SCANCODE_A])
        {
            // Движение влево (отрицательное по оси right)
            camera.moveRight(speed);
        }
        if (key_states[SDL_SCANCODE_D])
        {
            // Движение вправо (положительное по оси right)
            camera.moveRight(-speed);
        }
        if (key_states[SDL_SCANCODE_W])
        {
            // Движение вперед (положительное по оси forward)
            camera.moveForward(speed);
        }
        if (key_states[SDL_SCANCODE_S])
        {
            // Движение назад (отрицательное по оси forward)
            camera.moveForward(-speed);
        }
        if (key_states[SDL_SCANCODE_E])
        {
            camera.rotateYaw(-rotateSpeed);
            std::cout << currentAngel - rotateSpeed << "\n";
            currentAngel -= rotateSpeed;
        }
        if (key_states[SDL_SCANCODE_Q])
        {
            camera.rotateYaw(rotateSpeed);
            std::cout << currentAngel + rotateSpeed << "\n";
            currentAngel += rotateSpeed;
        }
        if (key_states[SDL_SCANCODE_TAB])
        {
            camera.rotatePitch(rotateSpeed);
        }
        if (key_states[SDL_SCANCODE_CAPSLOCK])
        {
            camera.rotatePitch(-rotateSpeed);
        }
        if (key_states[SDL_SCANCODE_SPACE])
        {
            camera.moveUp(speed);
        }
        if (key_states[SDL_SCANCODE_LSHIFT])
        {
            camera.moveUp(-speed);
        }

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                return 0;
            }
            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                int new_width = event.window.data1;
                int new_height = event.window.data2;
                aspect = static_cast<float>(new_width) / new_height;
                std::cout << new_width << "/" << new_height << "=" << aspect << "\n";
                projectionMartix = glm::perspective(glm::radians(45.0F), 1.6F, 0.1F, 100.0F);
                glViewport(0, 0, new_width, new_height);
            }
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (event.key.key == SDLK_ESCAPE)
                {
                    mouseHandle = !mouseHandle;
                    SDL_SetWindowRelativeMouseMode(mainWindow, mouseHandle);
                }
            }
            if (mouseHandle && event.type == SDL_EVENT_MOUSE_MOTION)
            {
                float deltaX = (float)event.motion.xrel;
                float deltaY = (float)event.motion.yrel;

                camera.rotateYaw(deltaX * mouseSensitivity);

                camera.rotatePitch(-deltaY * mouseSensitivity);
            }
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        viewMatrix = camera.getViewMatrix();

        glUseProgram(programID);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, firstVertexBuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, firstColorBuffer);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

        glm::mat4 MVPMatrix1 = projectionMartix * viewMatrix * modelMatrix1;

        glUniformMatrix4fv(matrixID,
                           1,
                           GL_FALSE,
                           &MVPMatrix1[0][0]);

        glDrawArrays(GL_TRIANGLES, 0, 12 * 3);

        glm::mat4 MVPMatrix2 = projectionMartix * viewMatrix * modelMatrix2;

        glUniformMatrix4fv(matrixID,
                           1,
                           GL_FALSE,
                           &MVPMatrix2[0][0]);

        glDrawArrays(GL_TRIANGLES, 0, 12 * 3);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        SDL_GL_SwapWindow(mainWindow);

        SDL_Delay(10);
    }

    glDeleteBuffers(1, &firstVertexBuffer);
    glDeleteBuffers(1, &firstColorBuffer);
    glDeleteVertexArrays(1, &vertexArrayID);
    glDeleteProgram(programID);

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(mainWindow);
    SDL_Quit();
}
