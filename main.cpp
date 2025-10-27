#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keycode.h"
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

    SDL_Window *mainWindow = SDL_CreateWindow("Hello, OpenGL!", 800, 800, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(mainWindow);
    SDL_GL_MakeCurrent(mainWindow, context);

    glewExperimental = true;
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to init glew!";
        return 1;
    }

    SDL_Event event;

    glEnable(GL_DEPTH_TEST);

    glDepthFunc(GL_LESS);

    glClearColor(0.0F, 0.0F, 0.4F, 0.0F);

    GLuint vertexArrayID;
    glGenVertexArrays(1, &vertexArrayID);
    glBindVertexArray(vertexArrayID);

    GLuint programID = LoadShaders(SHADERS_DIR "/VertexShader.glsl",
                                   SHADERS_DIR "/FragmentShader.glsl");

    GLuint matrixID = glGetUniformLocation(programID, "MVP");

    Camera camera(glm::vec3(4.0f, 3.0f, 3.0f), // Немного выше и дальше для лучшего обзора
                  glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, 1.0f, 0.0f));

    // Используем speed как "скорость перемещения"
    float speed = 0.2f;       // Немного увеличим для заметного эффекта
    float rotateSpeed = 1.2f; // Немного увеличим для заметного эффекта

    glm::mat4 projectionMartix = glm::perspective(glm::radians(45.0F), 1.0F / 1.0F, 0.1F, 100.0F);

    glm::mat4 viewMatrix = camera.getViewMatrix();
    // glm::mat4 viewMatrix = glm::lookAt(
    //     glm::vec3(4, 3, 3),
    //     glm::vec3(0, 0, 0),
    //     glm::vec3(0, 1, 0));
    glm::mat4 modelMatrix = glm::mat4(1.0F);

    glm::mat4 MVPMatrix = projectionMartix * viewMatrix * modelMatrix;

    const GLfloat *vertexData = getCubeData();
    const GLfloat *colorData = getColorData();

    // Размер в байтах также можно получить из заголовка
    size_t vertexDataSize = CUBE_DATA_SIZE_BYTES;
    size_t colorDataSize = COLOR_DATA_SIZE_BYTES;

    // ... ваш код для GL (например, glBufferData)
    // glBufferData(GL_ARRAY_BUFFER, size_bytes, data, GL_STATIC_DRAW);

    GLuint vertexBuffer;

    glGenBuffers(1, &vertexBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    glBufferData(GL_ARRAY_BUFFER,
                 vertexDataSize,
                 vertexData,
                 GL_STATIC_DRAW);

    GLuint colorBuffer;

    glGenBuffers(1, &colorBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);

    glBufferData(GL_ARRAY_BUFFER,
                 colorDataSize,
                 colorData,
                 GL_STATIC_DRAW);

    float currentAngel = 0.0F;

    while (true)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                return 0;
            }
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                // // Здесь используем методы moveRight() и moveForward() класса Camera
                if (event.key.key == SDLK_A)
                {
                    // Движение влево (отрицательное по оси right)
                    camera.moveRight(-speed);
                }
                else if (event.key.key == SDLK_D)
                {
                    // Движение вправо (положительное по оси right)
                    camera.moveRight(speed);
                }
                else if (event.key.key == SDLK_W)
                {
                    // Движение вперед (положительное по оси forward)
                    camera.moveForward(speed);
                }
                else if (event.key.key == SDLK_S)
                {
                    // Движение назад (отрицательное по оси forward)
                    camera.moveForward(-speed);
                }
                else if (event.key.key == SDLK_E)
                {
                    camera.rotateYaw(-rotateSpeed);
                    std::cout << currentAngel - rotateSpeed << "\n";
                    currentAngel -= rotateSpeed;
                }
                else if (event.key.key == SDLK_Q)
                {
                    camera.rotateYaw(rotateSpeed);
                    std::cout << currentAngel + rotateSpeed << "\n";
                    currentAngel += rotateSpeed;
                }
                else if (event.key.key == SDLK_TAB)
                {
                    camera.rotatePitch(rotateSpeed);
                }
                else if (event.key.key == SDLK_CAPSLOCK)
                {
                    camera.rotatePitch(-rotateSpeed);
                }
                if (event.key.key == SDLK_UP)
                {
                    camera.moveUp(speed);
                }
                if (event.key.key == SDLK_DOWN)
                {
                    camera.moveUp(-speed);
                }
            }
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        viewMatrix = camera.getViewMatrix();

        MVPMatrix = projectionMartix * viewMatrix * modelMatrix;

        glUseProgram(programID);

        glUniformMatrix4fv(matrixID,
                           1,
                           GL_FALSE,
                           &MVPMatrix[0][0]);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glVertexAttribPointer(0,
                              3,
                              GL_FLOAT,
                              GL_FALSE,
                              0,
                              (void *)0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            (void *)0);

        glDrawArrays(GL_TRIANGLES, 0, 12 * 3);

        glDisableVertexAttribArray(0);

        SDL_GL_SwapWindow(mainWindow);

        SDL_Delay(10);
    }

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(mainWindow);
    SDL_Quit();
}
