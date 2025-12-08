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
        glm::vec3 forward;
        glm::vec3 right;
        
        void updateVectors() {
            forward = glm::normalize(target - position); // LookAt style logic fix
            // Для FPS камеры логика пересчета forward от yaw/pitch была бы здесь
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

        // Методы управления (упрощенные для MVP примера)
        void moveForward(float dist) {
            glm::vec3 flatFwd = glm::normalize(glm::vec3(forward.x, 0, forward.z));
            position += flatFwd * dist;
            target += flatFwd * dist;
        }

        void moveRight(float dist) {
            position += right * dist;
            target += right * dist;
        }
        
        void moveUp(float dist) {
            position += up * dist;
            target += up * dist;
        }

        void rotateAroundTarget(float dYaw, float dPitch) {
            // Простая реализация орбитальной камеры или поворота головы
            // В вашем оригинале была смесь FPS и LookAt.
            // Оставим FPS-подобное вращение вектора взгляда:
            glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
            forward = glm::rotate(forward, glm::radians(dYaw), worldUp);
            forward = glm::rotate(forward, glm::radians(dPitch), right);
            target = position + forward;
            updateVectors();
        }
    };
}