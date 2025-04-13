#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

void glfw_error_callback(int error, const char* description) { std::cerr << "Glfw Error " << error << ": " << description << std::endl; }

struct WheelSlice {
    std::string label;
    ImU32       color;
};

static std::vector<WheelSlice> wheelSlices = {
    {"Option 1", IM_COL32(255, 0,   0,   255)},
    {"Option 2", IM_COL32(0,   255, 0,   255)},
    {"Option 3", IM_COL32(0,   0,   255, 255)},
    {"Option 4", IM_COL32(255, 255, 0,   255)},
    {"Option 5", IM_COL32(0,   255, 255, 255)},
    {"Option 6", IM_COL32(255, 0,   255, 255)}
};

static bool  isSpinning    = false;
static float currentAngle  = 0.0f;
static float spinSpeed     = 0.0f;
static auto  spinStartTime = std::chrono::steady_clock::now();

void DrawWheel(ImDrawList* drawList, ImVec2 center, float radius) {
    const int numSlices = wheelSlices.size();

// Define IM_PI if not already defined
#ifndef IM_PI
    #define IM_PI 3.14159265358979323846f
#endif

    // Initialize anglePerSlice properly
    const float anglePerSlice = 2.0f * IM_PI / numSlices;

    // Clip the pie slices to the circular boundary
    ImVec4 clipRect = ImVec4(center.x - radius, center.y - radius, center.x + radius, center.y + radius);
    drawList->PushClipRect(ImVec2(clipRect.x, clipRect.y), ImVec2(clipRect.z, clipRect.w), true);

    // Draw proper pie slices to fill the circle
    for (int i = 0; i < numSlices; ++i) {
        float startAngle = currentAngle + i * anglePerSlice;
        float endAngle   = startAngle + anglePerSlice;

        // Draw a filled arc for each slice
        drawList->PathArcTo(center, radius, startAngle, endAngle, 50);
        drawList->PathLineTo(center);
        drawList->PathFillConvex(wheelSlices[i].color);
    }

    // Add a circular outline for the wheel
    drawList->AddCircle(center, radius, IM_COL32(255, 255, 255, 255), 100, 2.0f);

    drawList->PopClipRect();
}

void UpdateWheel() {
    if (isSpinning) {
        auto  now     = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - spinStartTime).count();

        if (elapsed > 20.0f) {
            isSpinning = false;
            spinSpeed  = 0.0f;
        } else {
            spinSpeed = std::max(0.0f, spinSpeed - 0.0005f);  // Slower linear decay for a more gradual slowdown
            currentAngle += spinSpeed;
        }
    }
}

int main() {
    // Setup GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create window with OpenGL context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui Example", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // Enable vsync

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events
        glfwPollEvents();

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Example ImGui window
        ImGui::Begin("Wheel Spinner");
        if (ImGui::Button("Spin the Wheel")) {
            if (!isSpinning) {
                isSpinning    = true;
                spinSpeed     = 0.3f;  // Reduced initial speed
                spinStartTime = std::chrono::steady_clock::now();
            }
        }

        ImVec2 wheelCenter = ImGui::GetCursorScreenPos();
        wheelCenter.x += 150.0f;  // Offset for center
        wheelCenter.y += 150.0f;
        float wheelRadius = 100.0f;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        DrawWheel(drawList, wheelCenter, wheelRadius);

        ImGui::End();

        UpdateWheel();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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