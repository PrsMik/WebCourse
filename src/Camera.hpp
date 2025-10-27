#pragma once
#include "glm/trigonometric.hpp"
#include <ranges>
#define GLM_ENABLE_EXPERIMENTAL

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/rotate_vector.hpp>

class Camera
{
private:
    // Позиция камеры в мировых координатах
    glm::vec3 position;
    // Точка, на которую смотрит камера
    glm::vec3 target;
    // Вектор "вверх" (обычно (0, 1, 0))
    glm::vec3 up;

    // Векторы для определения "перед", "вправо" и "вверх" в локальном пространстве камеры
    glm::vec3 forward;
    glm::vec3 right;

    // Углы Эйлера (Euler angles) для ориентации
    float yaw = 0;   // Рыскание (поворот вокруг оси Y)
    float pitch = 0; // Тангаж (поворот вокруг оси X)
    // Roll (крен) обычно не используется в простых камерах
    glm::mat4 rotationMatrix;
    /**
     * @brief Обновляет векторы forward и right на основе углов yaw и pitch.
     * Должен вызываться после любого изменения yaw или pitch.
     */
    void updateCameraVectors()
    {
        forward = glm::normalize(forward);

        right = glm::normalize(glm::cross(up, forward));

        // вычисляем повторно вектор up как перпендикуляр к плоскости, заданной forward и right
        up = glm::normalize(glm::cross(forward, right));

        // // Вычисление вектора right (поперечный вектор)
        // right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f))); // Вектор "вправо"

        // // Пересчет вектора up (чтобы избежать крена)
        // up = glm::normalize(glm::cross(right, forward));
    }

public:
    /**
     * @brief Конструктор камеры
     * @param initialPos Начальная позиция камеры.
     * @param initialTarget Точка, на которую смотрит камера.
     * @param worldUp Вектор "вверх" мировых координат.
     */
    Camera(glm::vec3 initialPos, glm::vec3 initialTarget, glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f))
        : position(initialPos), target(initialTarget), up(worldUp), yaw(-90.0f), pitch(0.0f)
    {
        // Инициализация target-камеры (LookAt-камера)
        // В этой реализации мы фокусируемся на том, чтобы target был фиксированным
        // (как в вашем коде - (0, 0, 0)), а position двигалась.
        // Для "свободной" камеры (FPS-style) нужно использовать углы yaw/pitch
        // и updateCameraVectors().
        // Для вашего случая, где камера всегда смотрит в (0, 0, 0),
        // достаточно только position и getTarget().

        // Для демонстрации "свободной" камеры (FPS-style), которая может быть более гибкой:
        // Position будет управляться, а target будет = position + forward.
        forward = target - position;
        this->updateCameraVectors();
        this->target = position + forward; // Цель всегда впереди (для FPS-style)
    }

    /**
     * @brief Возвращает матрицу вида (View Matrix) для OpenGL.
     * @return glm::mat4 Матрица вида.
     */
    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(position, target, up);
    }

    // ---------------------- Методы перемещения (для FPS-style или LookAt-style) ----------------------

    /**
     * @brief Перемещает камеру по оси X (вправо/влево).
     * @param distance Смещение. Положительное значение - вправо, отрицательное - влево.
     */
    void moveRight(float distance)
    {
        // Движение перпендикулярно направлению forward, используя вектор right
        position += right * distance;
        target += right * distance; // Перемещаем target вместе с position
    }

    /**
     * @brief Перемещает камеру по оси Z (вперед/назад).
     * @param distance Смещение. Положительное значение - вперед (вдоль forward), отрицательное - назад.
     */
    void moveForward(float distance)
    {
        // Движение вперед/назад, но только в плоскости XZ (чтобы избежать "полета")
        glm::vec3 flatForward = glm::normalize(glm::vec3(forward.x, forward.y, forward.z));
        position += flatForward * distance;
        target += flatForward * distance; // Перемещаем target вместе с position
    }

    /**
     * @brief Перемещает камеру по оси Y (вверх/вниз).
     * @param distance Смещение. Положительное значение - вверх, отрицательное - вниз.
     */
    void moveUp(float distance)
    {
        position += up * distance;
        target += up * distance; // Перемещаем target вместе с position
    }

    // ---------------------- Методы вращения (для FPS-style камеры) ----------------------

    /**
     * @brief Поворачивает камеру вокруг оси Y (рыскание).
     * @param degrees Угол в градусах.
     */
    void rotateYaw(float degrees)
    {
        yaw += degrees;

        glm::quat orientation = glm::quat_cast(glm::yawPitchRoll(glm::radians(degrees), 0.0F, 0.0F));

        forward = forward * orientation;
        // Ограничиваем угол, если нужно (например, 0-360), хотя GLM это выдержит
        updateCameraVectors();
        // Обновляем target для FPS-style
        target = position + forward;
    }

    /**
     * @brief Поворачивает камеру вокруг оси X (тангаж).
     * @param degrees Угол в градусах.
     */
    void rotatePitch(float degrees)
    {
        pitch += degrees;
        glm::quat orientation = glm::quat_cast(glm::yawPitchRoll(0.0F, glm::radians(degrees), 0.0F));
        forward = forward * orientation;
        // Ограничение тангажа для предотвращения "переворота"
        // forward = glm::rotateZ(forward, glm::radians(degrees));
        // pitch = std::min(pitch, 89.0f);
        // pitch = std::max(pitch, -89.0f);
        updateCameraVectors();
        // Обновляем target для FPS-style
        target = position + forward;
    }

    // ---------------------- Методы для Look-At Circle камеры (как в вашем коде) ----------------------

    /**
     * @brief Устанавливает новую позицию камеры.
     * @param newPos Новая позиция.
     */
    void setPosition(const glm::vec3 &newPos)
    {
        position = newPos;
    }

    /**
     * @brief Устанавливает новую точку, на которую смотрит камера.
     * @param newTarget Новая цель.
     */
    void setTarget(const glm::vec3 &newTarget)
    {
        target = newTarget;
    }

    glm::vec3 getPosition() const { return position; }
};