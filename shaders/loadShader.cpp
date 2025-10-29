#include <SDL3/SDL.h> // Предполагаем, что вы используете SDL3
#include <format>
#include <fstream>
#include <glad/gl.h> // Подключение GLEW
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


static GLuint LoadShaders(const char *vertexFilePath,
                          const char *fragmentFilePath)
{
    GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    std::string vertexShaderCode;
    std::ifstream vertexShaderStream(vertexFilePath,
                                     std::ios::in);
    if (vertexShaderStream.is_open())
    {
        std::stringstream sstr;
        sstr << vertexShaderStream.rdbuf();
        vertexShaderCode = sstr.str();
        vertexShaderStream.close();
    }

    std::string fragmentShaderCode;
    std::ifstream fragmentShaderStream(fragmentFilePath,
                                       std::ios::in);
    if (fragmentShaderStream.is_open())
    {
        std::stringstream sstr;
        sstr << fragmentShaderStream.rdbuf();
        fragmentShaderCode = sstr.str();
        fragmentShaderStream.close();
    }

    GLint result = GL_FALSE;
    int infoLogLength;
    std::cout << std::format("Компиляция шейдера {}\n", vertexFilePath);
    char const *vertexSourcePointer = vertexShaderCode.c_str();
    glShaderSource(vertexShaderID, 1,
                   &vertexSourcePointer, nullptr);
    glCompileShader(vertexShaderID);

    glGetShaderiv(vertexShaderID,
                  GL_COMPILE_STATUS, &result);
    glGetShaderiv(vertexShaderID,
                  GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0)
    {
        std::vector<char> vertexShaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(vertexShaderID,
                           infoLogLength,
                           nullptr,
                           &vertexShaderErrorMessage[0]);
        std::cout << std::format("{}", &vertexShaderErrorMessage[0]);
    }

    std::cout << std::format("Компиляция шейдера {}\n", fragmentFilePath);
    char const *fragmentSourcePointer = fragmentShaderCode.c_str();
    glShaderSource(fragmentShaderID, 1,
                   &fragmentSourcePointer, nullptr);
    glCompileShader(fragmentShaderID);

    glGetShaderiv(fragmentShaderID,
                  GL_COMPILE_STATUS, &result);
    glGetShaderiv(fragmentShaderID,
                  GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0)
    {
        std::vector<char> fragmentShaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(fragmentShaderID,
                           infoLogLength,
                           nullptr,
                           &fragmentShaderErrorMessage[0]);
        std::cout << std::format("{}", &fragmentShaderErrorMessage[0]);
    }
    std::cout << "Создание шейдерной программы\n";
    GLuint programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);

    glGetProgramiv(programID,
                   GL_COMPILE_STATUS, &result);
    glGetProgramiv(programID,
                   GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0)
    {
        std::vector<char> programErrorMessage(infoLogLength + 1);
        glGetProgramInfoLog(programID,
                            infoLogLength,
                            nullptr,
                            &programErrorMessage[0]);
        std::cout << std::format("{}", &programErrorMessage[0]);
    }

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    return programID;
}