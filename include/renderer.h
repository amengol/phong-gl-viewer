#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <string>
#include "camera.h"
#include "shader.h"
#include "model.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <memory>

class Renderer {
public:
    Renderer(int width, int height, const char* title);
    ~Renderer();
    
    void Run();
    void loadModel(const char* path);

private:
    GLFWwindow* window;
    Camera camera;
    std::unique_ptr<Model> model;
    std::unique_ptr<Shader> shader;
    glm::vec3 modelScale;  // Store model scale factor
    glm::vec3 rotationCenter;  // Point to orbit around
    bool updateRotationCenter;  // Flag to update rotation center
    
    // Lighting properties
    glm::vec3 lightPos;
    glm::vec3 lightColor;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;
    float shininess;

    // Window properties
    int width;
    int height;
    float lastX;
    float lastY;
    bool firstMouse;
    float deltaTime;
    float lastFrame;

    void initGLFW();
    void initGLAD();
    void initImGui();
    void processInput();
    void renderUI();
    void cleanup();
    std::string openFileDialog();

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
}; 