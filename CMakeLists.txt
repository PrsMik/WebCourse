cmake_minimum_required(VERSION 3.30.0)
project(Wordle++ VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in lib/SDL
add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)

find_package(OpenGL REQUIRED)

set(GLAD_ROOT_DIR "${PROJECT_SOURCE_DIR}/lib/glad")

add_subdirectory(lib/glad/cmake)

glad_add_library(
    glad_opengl 
    STATIC 
    API gl:core=4.6 
    REPRODUCIBLE
)

# --- IMGUI SETUP (ИСПРАВЛЕНИЕ ОШИБКИ) ---
set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/lib/imgui")

# 1. Список исходных файлов ImGui (Ядро + Бэкенды)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# 2. Создаем статическую библиотеку для ImGui
add_library(imgui STATIC ${IMGUI_SOURCES})

# 3. Указываем пути к заголовкам для самой библиотеки imgui
target_include_directories(imgui PUBLIC ${IMGUI_DIR})

# 4. Линкуем зависимости, нужные бэкендам ImGui (SDL для sdl3_impl, GLAD для opengl3_impl)
target_link_libraries(imgui PUBLIC SDL3::SDL3 glad_opengl OpenGL::GL)
# ----------------------------------------

set(GLM_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/glm")

# --- ДОБАВЛЕНИЕ ASSIMP ---
add_subdirectory(lib/assimp)

# --- ИСПРАВЛЕНИЕ ОШИБОК КОМПИЛЯЦИИ ASSIMP С CLANG-CL (-Werror) ---
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Clang detected. Applying C/CXX warning suppressions to assimp targets.")
    
    set(ASSIMP_CLANG_SUPPRESS_FLAGS 
        -Wno-unsafe-buffer-usage
        -Wno-switch-default
        -Wno-format
        -Wno-unreachable-code-break
        -Wno-deprecated-copy
        -Wno-unused-member-function
        -Wno-deprecated-non-prototype 
        -Wno-padded
        -Wno-nrvo
        -Wno-unique-object-duplication
        -Wno-missing-noreturn
        -Wno-unnecessary-virtual-specifier
        -Wno-c++-keyword
        -Wno-character-conversion
    )
 
    set(ASSIMP_CLANG_OPTIONS 
        $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:${ASSIMP_CLANG_SUPPRESS_FLAGS}>
    )

    target_compile_options(assimp PRIVATE ${ASSIMP_CLANG_OPTIONS})
    
    # Исправление для unit тестов assimp, если они собираются
    if(TARGET unit)
        target_compile_options(unit PRIVATE ${ASSIMP_CLANG_OPTIONS})
    endif()
    
    set_property(TARGET assimp APPEND PROPERTY RC_FLAGS "-c65001")
endif()
# --------------------------------------------------------------------

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
    "cmake/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
    @ONLY
)

# Объявляем исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCES})

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
    # Путь к ImGui уже подтянется через target_link_libraries, 
    # но можно оставить явно, если нужно
    "${IMGUI_DIR}" 
)

target_compile_definitions(Wordle++
    PRIVATE 
    DATA_DIR="${DATA_DIR}"
    SHADERS_DIR="${SHADERS_DIR}"
    # Указываем ImGui использовать GLAD (иначе он может пытаться грузить стандартный GL)
    IMGUI_IMPL_OPENGL_LOADER_GLAD 
)

# --- ЛИНКОВКА ---
# Добавили 'imgui' в список
target_link_libraries(${PROJECT_NAME} PRIVATE 
    SDL3::SDL3 
    OpenGL::GL 
    glad_opengl 
    assimp 
    imgui
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(COMPILE_COMMANDS_PATH "${CMAKE_BINARY_DIR}/compile_commands.json")
    set(DESTINATION_PATH "${CMAKE_SOURCE_DIR}/compile_commands.json")
    
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                "${COMPILE_COMMANDS_PATH}" 
                "${DESTINATION_PATH}"
        RESULT_VARIABLE COPY_STATUS
    )
    if(NOT COPY_STATUS EQUAL 0)
        message(STATUS "Не удалось скопировать compile_commands.json в ${CMAKE_SOURCE_DIR}")
    else()
        message(STATUS "Исправление регистра диска в ${DESTINATION_PATH} для clangd...")
        file(READ "${DESTINATION_PATH}" COMPILE_COMMANDS_CONTENT)
        string(REPLACE "D:" "d:" COMPILE_COMMANDS_CONTENT "${COMPILE_COMMANDS_CONTENT}")
        file(WRITE "${DESTINATION_PATH}" "${COMPILE_COMMANDS_CONTENT}")
        message(STATUS "Исправление завершено.")
    endif()
endif()