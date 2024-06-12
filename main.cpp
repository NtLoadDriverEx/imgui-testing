 #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "stdio.h"
#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <array>

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

struct image{
    ImTextureID texture_id = 0;
    int width = 0;
    int height = 0;
};

image LoadTextureFromFile(const char* filename) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data) {
        printf("stb_image.h failed to load %s\n", filename);
        return {};
    }
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, nrChannels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    return {(ImTextureID)(intptr_t)textureID, width /2, height /2};
}

struct bounds {
    ImVec2 min;
    ImVec2 max;
};

struct rect {
    ImVec2 top_left;
    ImVec2 bottom_right;
};

//affine transformation
struct atransform{
    float scale_x;
    float offset_x;
    float scale_y;
    float offset_y;
};

// map constants
// [[79,-64.5],[-66.5,67.4]] factory

// 79, -64.5 top right
// -66.5, 67.4 bottom left

// "transform": [1.629, 119.9, 1.629, 139.3],
constexpr atransform transform = {1.629f, 119.9f, 1.629f, 139.3f};
constexpr rect map_rect = {ImVec2{79.0f, 67.4f}, ImVec2{-66.5f, -64.5f}};
constexpr ImVec2 map_size = ImVec2{141.8f, 131.57f};
constexpr float coordinate_rotation = 90.f; // degrees

// between boilers constant coordinate 56.20, 6.23
constexpr ImVec2 boiler_position = ImVec2{9.f, -20.f};

// degrees to radains
template <typename T>
constexpr T pi = T(3.14159265358979323846);
template <typename T>
constexpr T d2r = pi<T> / T(180);
template <typename T>
constexpr T r2d = T(180) / pi<T>;


// Rotate point takes two points one to rotate around the other and the angle in degrees
ImVec2 RotatePoint(const ImVec2& point, const ImVec2& center, float angleDegrees) {
    float angleRadians = angleDegrees * d2r<float>;
    float s = sin(angleRadians);
    float c = cos(angleRadians);

    // translate point back to origin:
    ImVec2 p = point - center;

    // rotate point
    ImVec2 rotatedPoint = ImVec2(
        p.x * c - p.y * s,
        p.x * s + p.y * c
    );

    // translate point back:
    return rotatedPoint + center;
}


ImVec2 TransformPoint(const ImVec2& point, int width, int height, float rotationDegrees) {
    // 1. Rotate the point first, taking into account z, x order
    ImVec2 rotatedPoint = RotatePoint(ImVec2(point.y, point.x), {0,0}, rotationDegrees); 

    // convert from map coordinates to 0,1 space scaled
    float x = ((rotatedPoint.x - map_rect.bottom_right.x) / map_size.x) * 1.629f;
    float y = ((rotatedPoint.y - map_rect.bottom_right.y) / map_size.y) * 1.629f;

    printf("Transformed point: %.2f, %.2f\n", x, y);

    // convert from 0,1 to texture space
    x *= width;
    y *= height;

    printf("Scaled point: %.2f, %.2f\n", x, y);


    return ImVec2(x, y);
}


int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        return -1;
    }

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui Test Project", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader (GLEW or glad, depending on your preference)
    if (glewInit() != GLEW_OK) {
        printf("Failed to init glew!\n");
        return -1;
    }

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    io.IniFilename = nullptr;

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    const auto factory_image = LoadTextureFromFile("1st_floor.png");
    const auto image_size = ImVec2{(float)factory_image.width, (float)factory_image.height};


    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events
        glfwPollEvents();

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        auto draw_list = ImGui::GetBackgroundDrawList();
        

        ImVec2 display_center = ImVec2{io.DisplaySize.x * .5f, io.DisplaySize.y * .5f};
        ImVec2 texture_center = image_size * .5f;
        ImVec2 render_position = display_center - texture_center;


        draw_list->AddImage(factory_image.texture_id, ImVec2{0,0} + render_position, image_size + render_position);

        const auto transformed_boiler_position = TransformPoint(boiler_position, factory_image.width, factory_image.height, coordinate_rotation);

        draw_list->AddCircleFilled(transformed_boiler_position + render_position, 5.f, IM_COL32(255, 0, 0, 255));
        draw_list->AddCircleFilled(display_center, 5.f, IM_COL32(255, 255, 0, 255));


        char buffer[0x512];
        snprintf(buffer, sizeof(buffer), "Boiler Position: %.2f, %.2f", boiler_position.x, boiler_position.y);
        draw_list->AddText(ImVec2{10, 10}, IM_COL32(255, 255, 255, 255), buffer);
        printf(buffer); printf("\n");

        snprintf(buffer, sizeof(buffer), "Transformed Boiler Position: %.2f, %.2f", transformed_boiler_position.x, transformed_boiler_position.y);
        draw_list->AddText(ImVec2{10, 30}, IM_COL32(255, 255, 255, 255), buffer);
        printf(buffer); printf("\n");
       
        snprintf(buffer, sizeof(buffer), "Cursor Pos: %.2f, %.2f", io.MousePos.x, io.MousePos.y);
        draw_list->AddText(ImVec2{10, 50}, IM_COL32(255, 255, 255, 255), buffer);
        printf(buffer); printf("\n");

        snprintf(buffer, sizeof(buffer), "Display size: %.2f, %.2f", io.DisplaySize.x, io.DisplaySize.y);
        draw_list->AddText(ImVec2{10, 70}, IM_COL32(255, 255, 255, 255), buffer);
        printf(buffer); printf("\n");

        snprintf(buffer, sizeof(buffer), "Image size: %.2f, %.2f", image_size.x, image_size.y);
        draw_list->AddText(ImVec2{10, 90}, IM_COL32(255, 255, 255, 255), buffer);
        printf(buffer); printf("\n");


        // Create a simple window
        ImGui::Begin("Settings");

        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.f, 0.f, 0.f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
