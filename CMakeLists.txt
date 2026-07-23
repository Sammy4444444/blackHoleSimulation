cmake_minimum_required(VERSION 3.20)

project(BlackHoleSimulator
    VERSION 0.1.0
    DESCRIPTION "3D Black Hole Simulator"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Visual Studio 2022: use "Visual Studio 17 2022" generator with -A x64
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

set(BHS_SOURCES
    src/main.cpp
    src/Core/Application.cpp
    src/Core/Window.cpp
    src/Core/Log.cpp
    src/Core/Timer.cpp
    src/Rendering/OpenGLContext.cpp
    src/Rendering/Shader.cpp
    src/Rendering/Mesh.cpp
    src/Rendering/Primitives.cpp
    src/Rendering/Renderer.cpp
    src/Camera/Camera.cpp
    src/Camera/CameraController.cpp
    src/UI/ImGuiLayer.cpp
    src/Physics/PhysicsWorld.cpp
    src/Assets/AssetManager.cpp
)

add_executable(${PROJECT_NAME} ${BHS_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
    glm::glm
    imgui
)

# Copy runtime assets next to the executable
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
    COMMENT "Copying assets to output directory"
)

# Working directory when launching from Visual Studio
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

message(STATUS "Black Hole Simulator ${PROJECT_VERSION}")
message(STATUS "Configure for VS 2022: cmake -S . -B build -G \"Visual Studio 17 2022\" -A x64")