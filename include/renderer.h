#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "camera.h"
#include "shader.h"
#include "model.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

class Renderer {
public:
    Renderer(int width, int height, const char* title);
    ~Renderer();
    
    void Run();

private:
    GLFWwindow* window;
    Camera camera;
    Model* model;
    Shader* shader;

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

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
}; 