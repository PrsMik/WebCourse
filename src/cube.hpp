#pragma once

#include "glm/vec3.hpp"
#include <GL/glew.h>
static const glm::vec3 redColor = {1.0f, 0.0f, 0.0f};
static const glm::vec3 greenColor = {0.0f, 1.0f, 0.0f};
static const glm::vec3 blueColor = {0.0f, 0.0f, 1.0f};

// 1. Определение самого массива (static, чтобы он был доступен только в этом файле)
static const GLfloat g_vertex_buffer_data[] = {
    // ГРАНЬ ОДИН (КРАСНАЯ)
    -1.0f, 1.0f, -1.0f, // Треугольник 1 : начало
    1.0f, 1.0f, -1.0f,
    1.0f, -1.0f, -1.0f, // Треугольник 1 : конец
    1.0f, -1.0f, -1.0f, // Треугольник 2 : начало
    -1.0f, -1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f, // Треугольник 2 : конец

    // ГРАНЬ ДВА (ЗЕЛЁНАЯ)
    -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,

    // ГРАНЬ ТРИ (СИНЯЯ)
    -1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f,

    // ГРАНЬ ЧЕТЫРЕ (КРАСНАЯ)
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,

    // ГРАНЬ ПЯТЬ (ЗЕЛЁНАЯ)
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,

    // ГРАНЬ ШЕСТЬ (СИНЯЯ)
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f};

static const GLfloat g_color_buffer_data[] = {
    // 1
    redColor.x, redColor.y, redColor.z,
    redColor.x, redColor.y, redColor.z,
    redColor.x, redColor.y, redColor.z,
    redColor.x, redColor.y, redColor.z,
    redColor.x, redColor.y, redColor.z,
    redColor.x, redColor.y, redColor.z,

    // 2
    greenColor.x, greenColor.y, greenColor.z,
    greenColor.x, greenColor.y, greenColor.z,
    greenColor.x, greenColor.y, greenColor.z,
    greenColor.x, greenColor.y, greenColor.z,
    greenColor.x, greenColor.y, greenColor.z,
    greenColor.x, greenColor.y, greenColor.z,

    // 3
    blueColor.x, blueColor.y, blueColor.z,
    blueColor.x, blueColor.y, blueColor.z,
    blueColor.x, blueColor.y, blueColor.z,
    blueColor.x, blueColor.y, blueColor.z,
    blueColor.x, blueColor.y, blueColor.z,
    blueColor.x, blueColor.y, blueColor.z,

    // 4
    redColor.x, redColor.y, redColor.z,
    redColor.x, redColor.y, redColor.z,
    redColor.x, redColor.y, redColor.z,
    redColor.x, redColor.y, redColor.z,
    redColor.x, redColor.y, redColor.z,
    redColor.x, redColor.y, redColor.z,

    // 5
    greenColor.x, greenColor.y, greenColor.z,
    greenColor.x, greenColor.y, greenColor.z,
    greenColor.x, greenColor.y, greenColor.z,
    greenColor.x, greenColor.y, greenColor.z,
    greenColor.x, greenColor.y, greenColor.z,
    greenColor.x, greenColor.y, greenColor.z,

    // 6
    blueColor.x, blueColor.y, blueColor.z,
    blueColor.x, blueColor.y, blueColor.z,
    blueColor.x, blueColor.y, blueColor.z,
    blueColor.x, blueColor.y, blueColor.z,
    blueColor.x, blueColor.y, blueColor.z,
    blueColor.x, blueColor.y, blueColor.z};

// 2. Определение функции, возвращающей указатель на первый элемент массива
const GLfloat *getCubeData()
{
    return g_vertex_buffer_data;
}

const GLfloat *getColorData()
{
    return g_color_buffer_data;
}

// 3. Определения для размера массива (полезно для использования в других файлах)
const size_t CUBE_VERTEX_COUNT = sizeof(g_vertex_buffer_data) / sizeof(GLfloat) / 3;
const size_t CUBE_DATA_SIZE_BYTES = sizeof(g_vertex_buffer_data);
const size_t COLOR_DATA_SIZE_BYTES = sizeof(g_color_buffer_data);