#version 300 es

layout (location = 0) in vec3 aPos;

// Передаем во фрагментный шейдер
out vec3 vLocalPos; // Позиция в пространстве модели (для расчета текстурных координат)

uniform mat4 MVP;

void main()
{
    // aPos у нас от -1.0 до 1.0 (вершины куба)
    vLocalPos = aPos;
    
    gl_Position = MVP * vec4(aPos, 1.0);
}