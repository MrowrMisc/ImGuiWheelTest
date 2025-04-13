#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

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

// Helper function to create a texture from text
GLuint CreateTextTexture(const std::string& text, ImFont* font, ImVec4 color, int textureSize) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    unsigned char* pixels = new unsigned char[textureSize * textureSize * 4];
    memset(pixels, 0, textureSize * textureSize * 4);

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2      textSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, text.c_str());
    ImVec2      textPos  = ImVec2((textureSize - textSize.x) / 2, (textureSize - textSize.y) / 2);

    drawList->AddText(textPos, ImGui::ColorConvertFloat4ToU32(color), text.c_str());

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureSize, textureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    delete[] pixels;
    return texture;
}

// Cache for text textures
std::unordered_map<std::string, GLuint> textTextureCache;

// Draw rotated text using textures
void DrawRotatedText(ImDrawList* drawList, const std::string& text, ImVec2 center, float angle, float radius, ImFont* font) {
    if (textTextureCache.find(text) == textTextureCache.end()) {
        textTextureCache[text] = CreateTextTexture(text, font, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 256);
    }

    GLuint texture = textTextureCache[text];
    ImVec2 textPos = ImVec2(center.x + radius * cosf(angle), center.y + radius * sinf(angle));

    drawList->AddImage((ImTextureID)(intptr_t)texture, textPos, ImVec2(textPos.x + 50, textPos.y + 50));
}

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

    // Remove the global doubling of the radius
    radius /= 2.0f;

    // Ensure the pie slices are drawn within the original circle size
    for (int i = 0; i < numSlices; ++i) {
        float startAngle = currentAngle + i * anglePerSlice;
        float endAngle   = startAngle + anglePerSlice;

        drawList->PathArcTo(center, radius, startAngle, endAngle, 50);
        drawList->PathLineTo(center);
        drawList->PathFillConvex(wheelSlices[i].color);
    }

    // Draw text over the pie slices, aligned to the angle of the slice
    for (int i = 0; i < numSlices; ++i) {
        float startAngle = currentAngle + i * anglePerSlice;
        float midAngle   = startAngle + anglePerSlice / 2;

        // Calculate the position for the text
        ImVec2 textPos = ImVec2(center.x + (radius * 0.5f) * cosf(midAngle), center.y + (radius * 0.5f) * sinf(midAngle));

        // Render the text
        int vtx_idx_begin = drawList->_VtxCurrentIdx;
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), wheelSlices[i].label.c_str());
        int vtx_idx_end = drawList->_VtxCurrentIdx;

        // Rotate the text using ShadeVertsTransformPos
        float  cos_a     = cosf(midAngle);
        float  sin_a     = sinf(midAngle);
        ImVec2 pivot_in  = textPos;
        ImVec2 pivot_out = textPos;
        ImGui::ShadeVertsTransformPos(drawList, vtx_idx_begin, vtx_idx_end, pivot_in, cos_a, sin_a, pivot_out);
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
            spinSpeed = std::max(0.0f, spinSpeed - 0.0002f);  // Adjusted linear decay rate for slower slowdown
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
                spinSpeed     = 0.2f;  // Reduced initial speed
                spinStartTime = std::chrono::steady_clock::now();
            }
        }

        ImVec2 wheelCenter = ImGui::GetCursorScreenPos();
        wheelCenter.x += 300.0f;  // Move the wheel further to the right to avoid overlapping the button
        wheelCenter.y += 150.0f;
        float wheelRadius = 400.0f;  // Increased the radius to make the circle 4 times larger

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