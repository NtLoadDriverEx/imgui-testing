cmake_minimum_required(VERSION 3.10)

project(MyImGuiProject)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories for ImGui
include_directories(imgui)
include_directories(imgui/backends)
include_directories(${GLEW_INCLUDE_DIRS})

# Find packages for OpenGL, GLFW, and GLEW
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)

# Add source files
set(SOURCES
    main.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

# Add executable
add_executable(MyImGuiProject ${SOURCES})

# Link libraries
target_link_libraries(MyImGuiProject PRIVATE glfw OpenGL::GL GLEW::GLEW)
