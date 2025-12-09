module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

export module App.Camera;

export namespace App {

    class Camera {
    private:
        glm::vec3 position;
        glm::vec3 target;
        glm::vec3 up;
        
        // Векторы локальной системы координат
        glm::vec3 forward;
        glm::vec3 right;

        void updateVectors() {
            // Forward вычисляется как разница, либо управляется вращением
            forward = glm::normalize(target - position);
            
            glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
            right = glm::normalize(glm::cross(forward, worldUp));
            up = glm::normalize(glm::cross(forward, right));
        }

    public:
        Camera(glm::vec3 pos, glm::vec3 tgt) 
            : position(pos), target(tgt), up(0,1,0) {
            updateVectors();
        }

        glm::mat4 getViewMatrix() const {
            return glm::lookAt(position, target, up);
        }
        
        glm::vec3 getPosition() const { return position; }

        // --- ДВИЖЕНИЕ КАМЕРЫ ---

        void moveForward(float dist) {
            // ИСПРАВЛЕНИЕ: Движение строго по вектору взгляда (Free Fly)
            // Раньше тут было обнуление Y, что мешало лететь туда, куда смотришь.
            glm::vec3 offset = forward * dist;
            position += offset;
            target += offset;
        }

        void moveRight(float dist) {
            glm::vec3 offset = right * dist;
            position += offset;
            target += offset;
        }
        
        void moveUp(float dist) {
            // Движение строго вверх по мировой оси Y
            glm::vec3 globalUp(0, 1, 0); 
            glm::vec3 offset = globalUp * dist;
            position += offset;
            target += offset;
        }

        // --- ВРАЩЕНИЕ (FPS Style) ---

        void rotateYaw(float angleDeg) {
            glm::vec3 worldUp(0, 1, 0);
            // Вращаем вектор forward вокруг мировой оси Y
            forward = glm::rotate(forward, glm::radians(angleDeg), worldUp);
            // Обновляем target
            target = position + forward;
            updateVectors();
        }

        void rotatePitch(float angleDeg) {
            // Вращаем вектор forward вокруг локальной оси Right
            glm::vec3 newForward = glm::rotate(forward, glm::radians(angleDeg), right);
            
            // Ограничение, чтобы не перевернуться через голову
            float dot = glm::dot(newForward, glm::vec3(0, 1, 0));
            if (abs(dot) < 0.99f) {
                forward = newForward;
                target = position + forward;
                updateVectors();
            }
        }
    };
}