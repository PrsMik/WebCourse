cmake_minimum_required(VERSION 3.30.0)
project(Wordle++ VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# --- ВАЖНОЕ ИСПРАВЛЕНИЕ ---
# Отключаем сканирование модулей по умолчанию.
# Это спасет ImGui и SDL от ошибок "compiler does not provide a way...".
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# --- ЗАВИСИМОСТИ ---
# SDL3
add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

# OpenGL
find_package(OpenGL REQUIRED)

# GLAD
set(GLAD_ROOT_DIR "${PROJECT_SOURCE_DIR}/lib/glad")
add_subdirectory(lib/glad/cmake)
glad_add_library(glad_opengl STATIC API gl:core=4.6 REPRODUCIBLE)

# ImGui
set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/lib/imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_DIR})
target_link_libraries(imgui PUBLIC SDL3::SDL3 glad_opengl OpenGL::GL)

# Убеждаемся, что для ImGui сканирование точно выключено
set_target_properties(imgui PROPERTIES CXX_SCAN_FOR_MODULES OFF)

set(GLM_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/glm")

# Assimp
add_subdirectory(lib/assimp)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Подавление варнингов (ваш блок кода)
    set(ASSIMP_CLANG_SUPPRESS_FLAGS 
        -Wno-unsafe-buffer-usage -Wno-switch-default -Wno-format 
        -Wno-unreachable-code-break -Wno-deprecated-copy -Wno-unused-member-function
        -Wno-deprecated-non-prototype -Wno-padded -Wno-nrvo 
        -Wno-unique-object-duplication -Wno-missing-noreturn 
        -Wno-unnecessary-virtual-specifier -Wno-c++-keyword -Wno-character-conversion
    )
    target_compile_options(assimp PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:${ASSIMP_CLANG_SUPPRESS_FLAGS}>)
    if(TARGET unit)
        target_compile_options(unit PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:${ASSIMP_CLANG_SUPPRESS_FLAGS}>)
    endif()
    set_property(TARGET assimp APPEND PROPERTY RC_FLAGS "-c65001")
endif()

# --- КОНФИГУРАЦИЯ ---
set(DATA_DIR "${PROJECT_SOURCE_DIR}/data")
set(SHADERS_DIR "${PROJECT_SOURCE_DIR}/shaders")

configure_file(
    "cmake/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
    @ONLY
)

# --- ОСНОВНОЙ ТАРГЕТ ---
add_executable(${PROJECT_NAME} 
    main.cpp
)

# Добавляем модули
target_sources(${PROJECT_NAME} 
    PUBLIC 
    FILE_SET CXX_MODULES FILES
        src/Types.cppm
        src/Camera.cppm
        src/Model.cppm
        src/View.cppm
        src/Presenter.cppm
)

# --- ВКЛЮЧАЕМ СКАНИРОВАНИЕ ТОЛЬКО ЗДЕСЬ ---
# Включаем сканирование модулей только для нашего проекта.
# Если вы используете Ninja, это заставит его строить граф зависимостей.
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_SCAN_FOR_MODULES ON)

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8) 
        target_compile_definitions(${PROJECT_NAME} PRIVATE "_AMD64_") 
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) 
        target_compile_definitions(${PROJECT_NAME} PRIVATE "_X86_")
    endif()
endif()

target_include_directories(Wordle++ PRIVATE 
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/lib/glad/include"
    "${GLM_INCLUDE_DIR}"
    "${IMGUI_DIR}"
)

target_compile_definitions(Wordle++
    PRIVATE 
    DATA_DIR="${DATA_DIR}"
    SHADERS_DIR="${SHADERS_DIR}"
    IMGUI_IMPL_OPENGL_LOADER_GLAD 
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    SDL3::SDL3 
    OpenGL::GL 
    glad_opengl 
    assimp 
    imgui
)
if(CMAKE_EXPORT_COMPILE_COMMANDS)
# ... (Остальной код для compile_commands.json без изменений)
    set(COMPILE_COMMANDS_PATH "${CMAKE_BINARY_DIR}/compile_commands.json")
    set(DESTINATION_PATH "${CMAKE_SOURCE_DIR}/compile_commands.json")
    
    # 1. Сначала генерируем файл (он создаётся во время сборки)
    # Используем команду execute_process, которая будет выполняться на этапе конфигурации
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                "${COMPILE_COMMANDS_PATH}" 
                "${DESTINATION_PATH}"
        RESULT_VARIABLE COPY_STATUS
        ERROR_VARIABLE COPY_ERROR # Можно использовать для отладки
    )
    if(NOT COPY_STATUS EQUAL 0)
        message(STATUS "Не удалось скопировать compile_commands.json в ${CMAKE_SOURCE_DIR}")
        message(COPY_ERROR)
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