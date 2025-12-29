cmake_minimum_required(VERSION 3.30.0)
project(Wordle++ VERSION 0.1.0 LANGUAGES C CXX)

# Понижаем до 20, так как поддержка модулей C++23 в Emscripten может быть нестабильной (конфликты ABI)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# Отключаем сканирование модулей по умолчанию
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    add_compile_options(-fno-exceptions -fno-rtti)
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
    find_package(OpenGL REQUIRED)
    
    set(GLAD_ROOT_DIR "${PROJECT_SOURCE_DIR}/lib/glad")
    add_subdirectory(lib/glad/cmake)
    glad_add_library(glad_opengl STATIC API gl:core=4.6 REPRODUCIBLE)
endif()

# SDL3 через FetchContent
include(FetchContent)
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG preview-3.1.3
)
FetchContent_MakeAvailable(SDL3)

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

if(EMSCRIPTEN)
    target_link_libraries(imgui PUBLIC SDL3::SDL3)
else()
    target_link_libraries(imgui PUBLIC SDL3::SDL3 glad_opengl OpenGL::GL)
endif()

set_target_properties(imgui PROPERTIES CXX_SCAN_FOR_MODULES OFF)

set(GLM_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/glm")

# --- КОНФИГУРАЦИЯ ---
set(DATA_DIR "/data")
if(NOT EMSCRIPTEN)
    set(DATA_DIR "${PROJECT_SOURCE_DIR}/data")
endif()

# --- ОСНОВНОЙ ТАРГЕТ ---
add_executable(${PROJECT_NAME} main.cpp)

target_sources(${PROJECT_NAME} 
    PUBLIC 
    FILE_SET CXX_MODULES FILES
        src/Types.cppm
        src/Camera.cppm
        src/Model.cppm
        src/View.cppm
        src/Presenter.cppm
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_SCAN_FOR_MODULES ON)

target_include_directories(${PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${GLM_INCLUDE_DIR}"
    "${IMGUI_DIR}"
)

if(NOT EMSCRIPTEN)
    target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/lib/glad/include")
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE 
    DATA_DIR="${DATA_DIR}"
)

if(EMSCRIPTEN)
    # Линковка для WEB
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 imgui)
    
    # Флаги Emscripten.
    # ВАЖНО: Используем синтаксис "-sFLAG=VAL" без пробелов, чтобы CMake передал их корректно.
    target_link_options(${PROJECT_NAME} PRIVATE
        "-sUSE_WEBGL2=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sNO_EXIT_RUNTIME=0"
        "--preload-file" "${PROJECT_SOURCE_DIR}/data@/data"
    )
else()
    # Линковка для Desktop
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 OpenGL::GL glad_opengl imgui)
endif()