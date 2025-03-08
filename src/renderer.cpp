#include "renderer.h"
#include <iostream>
#include <windows.h>
#include <commdlg.h>

Renderer::Renderer(int width, int height, const char* title) 
    : width(width), height(height), camera(glm::vec3(0.0f, 0.0f, 3.0f)), 
      lastX(width/2.0f), lastY(height/2.0f), firstMouse(true),
      deltaTime(0.0f), lastFrame(0.0f),
      lightPos(glm::vec3(1.2f, 1.0f, 2.0f)), lightColor(glm::vec3(1.0f)),
      ambientStrength(0.1f), diffuseStrength(0.5f), specularStrength(0.5f), shininess(32.0f),
      model(nullptr) {
    
    initGLFW();
    window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);

    initGLAD();
    initImGui();

    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetWindowUserPointer(window, this);

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);

    // Create and compile shaders
    shader = new Shader("shaders/phong.vert", "shaders/phong.frag");
}

Renderer::~Renderer() {
    cleanup();
}

void Renderer::Run() {
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader->use();

        // View/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)width / (float)height, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader->setMat4("projection", projection);
        shader->setMat4("view", view);

        // World transformation
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        shader->setMat4("model", modelMatrix);

        // Light properties
        shader->setVec3("lightPos", lightPos);
        shader->setVec3("lightColor", lightColor);
        shader->setVec3("viewPos", camera.Position);
        shader->setFloat("ambientStrength", ambientStrength);
        shader->setFloat("diffuseStrength", diffuseStrength);
        shader->setFloat("specularStrength", specularStrength);
        shader->setFloat("shininess", shininess);

        if (model != nullptr) {
            model->Draw(*shader);
        }

        renderUI();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Renderer::initGLFW() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

void Renderer::initGLAD() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return;
    }
}

void Renderer::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void Renderer::processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Skip camera movement if ImGui is capturing keyboard
    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.ProcessKeyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            camera.ProcessKeyboard(DOWN, deltaTime);
    }
}

std::string Renderer::openFileDialog() {
    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = glfwGetWin32Window(window);
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "3D Models\0*.obj;*.fbx;*.gltf;*.glb\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(fileName);
    }
    return "";
}

void Renderer::renderUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Model loading window
    ImGui::Begin("Model Control");
    if (ImGui::Button("Load Model")) {
        std::string filePath = openFileDialog();
        if (!filePath.empty()) {
            loadModel(filePath.c_str());
        }
    }
    
    if (model == nullptr) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No model loaded");
    } else if (!model->isValid()) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Model failed to load");
    } else {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Model loaded successfully");
    }
    ImGui::End();

    // Lighting controls window
    ImGui::Begin("Lighting Controls");
    
    ImGui::ColorEdit3("Light Color", &lightColor[0]);
    ImGui::DragFloat3("Light Position", &lightPos[0], 0.1f);
    
    ImGui::SliderFloat("Ambient Strength", &ambientStrength, 0.0f, 1.0f);
    ImGui::SliderFloat("Diffuse Strength", &diffuseStrength, 0.0f, 1.0f);
    ImGui::SliderFloat("Specular Strength", &specularStrength, 0.0f, 1.0f);
    ImGui::SliderFloat("Shininess", &shininess, 1.0f, 256.0f);
    
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    delete shader;
    delete model;

    glfwDestroyWindow(window);
    glfwTerminate();
}

void Renderer::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void Renderer::mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    
    // Skip camera movement if ImGui is capturing mouse
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }
    
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (renderer->firstMouse) {
        renderer->lastX = xpos;
        renderer->lastY = ypos;
        renderer->firstMouse = false;
    }

    float xoffset = xpos - renderer->lastX;
    float yoffset = renderer->lastY - ypos;

    renderer->lastX = xpos;
    renderer->lastY = ypos;

    renderer->camera.ProcessMouseMovement(xoffset, yoffset);
}

void Renderer::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void Renderer::loadModel(const char* path) {
    // Delete existing model if any
    if (model != nullptr) {
        delete model;
        model = nullptr;
    }

    // Create new model
    model = new Model(path);
    
    // Check if model loaded successfully
    if (!model->isValid()) {
        delete model;
        model = nullptr;
        std::cerr << "ERROR::RENDERER: Failed to load model from path: " << path << std::endl;
    }
} 