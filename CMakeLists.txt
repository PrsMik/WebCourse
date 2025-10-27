cmake_minimum_required(VERSION 3.30.0)
project(Wordle++ VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in lib/SDL
add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

find_package(OpenGL REQUIRED)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared GLEW library" FORCE) 
set(GLEW_BUILD_TESTS OFF CACHE BOOL "Disable GLEW tests" FORCE)
add_subdirectory(lib/glew-2.2.0/build/cmake)

set(GLM_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/glm")

file(GLOB CONTROLLER_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB SHADERS CONFIGURE_DEPENDS "shaders/*.cpp")

set(SOURCES
    main.cpp
    ${CONTROLLER_SOURCES}
    ${SHADERS}
)

set(DATA_DIR "${PROJECT_SOURCE_DIR}/data")
set(SHADERS_DIR "${PROJECT_SOURCE_DIR}/shaders")

configure_file(
    "cmake/config.h.in"  # Create this template file
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
    @ONLY
)

# Объявляем исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCES})

if(WIN32)
    # Определяем макросы архитектуры, которые ожидают некоторые библиотеки
    if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64-битная архитектура
        target_compile_definitions(${PROJECT_NAME} PRIVATE "_AMD64_") 
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32-битная архитектура
        target_compile_definitions(${PROJECT_NAME} PRIVATE "_X86_")
    endif()
    
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    endif()
endif()

# Add the directory containing the generated header to your include paths
target_include_directories(Wordle++ PRIVATE 
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/lib/glew-2.2.0/include"
    "${GLM_INCLUDE_DIR}"
)
# Pass the actual path to the compiler as a preprocessor definition
target_compile_definitions(Wordle++
    PRIVATE 
    # DATA_DIR содержит путь к папке data, включая кавычки
    DATA_DIR="${DATA_DIR}"
    SHADERS_DIR="${SHADERS_DIR}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 OpenGL::GL glew)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

if(WIN32 AND (NOT GLEW_STATIC))
    # GLEW_DLL_SOURCE: Получаем полный путь к скомпилированной DLL (glew.dll/glew32.dll).
    # <TARGET_FILE:glew> — это генераторное выражение, которое надежно находит выходной файл цели 'glew',
    # созданной подпроектом.
    set(GLEW_DLL_SOURCE $<TARGET_FILE:glew>)
    
    # GLEW_DLL_DESTINATION: Получаем путь к папке, куда был собран исполняемый файл.
    set(GLEW_DLL_DESTINATION $<TARGET_FILE_DIR:${PROJECT_NAME}>)

    # Добавляем команду после сборки Wordle++, чтобы скопировать DLL
    add_custom_command(
        TARGET ${PROJECT_NAME} 
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GLEW_DLL_SOURCE}"
            "${GLEW_DLL_DESTINATION}"
        COMMENT "Copying GLEW DLL to executable directory"
    )
endif()

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(COMPILE_COMMANDS_PATH "${CMAKE_BINARY_DIR}/compile_commands.json")
    set(DESTINATION_PATH "${CMAKE_SOURCE_DIR}/compile_commands.json")
    
    # 1. Сначала генерируем файл (он создаётся во время сборки)
    # Используем команду execute_process, которая будет выполняться на этапе конфигурации
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                "${COMPILE_COMMANDS_PATH}" 
                "${DESTINATION_PATH}"
        RESULT_VARIABLE COPY_STATUS
        # ERROR_VARIABLE COPY_ERROR # Можно использовать для отладки
    )
    if(NOT COPY_STATUS EQUAL 0)
        message(STATUS "Не удалось скопировать compile_commands.json в ${CMAKE_SOURCE_DIR}")
    else()
        # 2. Манипуляция содержимым файла для исправления регистра диска
        message(STATUS "Исправление регистра диска в ${DESTINATION_PATH} для clangd...")
        
        # Читаем содержимое файла в переменную
        file(READ "${DESTINATION_PATH}" COMPILE_COMMANDS_CONTENT)
        
        # Заменяем D: на d:
        string(REPLACE "D:" "d:" COMPILE_COMMANDS_CONTENT "${COMPILE_COMMANDS_CONTENT}")
        
        # Записываем измененное содержимое обратно в файл
        file(WRITE "${DESTINATION_PATH}" "${COMPILE_COMMANDS_CONTENT}")
        
        message(STATUS "Исправление завершено.")
    endif()
endif()
