#include "renderer.h"
#include <iostream>
#include <windows.h>
#include <commdlg.h>

Renderer::Renderer(int width, int height, const char* title) 
    : width(width), height(height), 
      camera(glm::vec3(0.0f, 2.0f, 8.0f)), // Move camera back and up a bit
      lastX(static_cast<float>(width)/2.0f), 
      lastY(static_cast<float>(height)/2.0f),
      firstMouse(true),
      deltaTime(0.0f), lastFrame(0.0f),
      lightPos(glm::vec3(2.0f, 4.0f, 2.0f)), // Adjust light position for better lighting
      lightColor(glm::vec3(1.0f)),
      ambientStrength(0.2f),
      diffuseStrength(0.8f),
      specularStrength(0.5f), 
      shininess(32.0f),
      model(nullptr),
      modelScale(glm::vec3(1.0f)),
      shader(nullptr) { // Initialize shader pointer to nullptr
    
    initGLFW();
    window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);

    // Set cursor mode AFTER window creation
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    initGLAD();
    initImGui();

    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);  // Add mouse button callback
    glfwSetWindowUserPointer(window, this);
}

Renderer::~Renderer() {
    cleanup();
}

void Renderer::Run() {
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Poll events before ImGui frame
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Check if window is minimized
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            // Even when minimized, we need to render ImGui and swap buffers
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
            continue;
        }

        // Process input after ImGui frame starts
        processInput();

        // Clear with a neutral gray background
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader->use();

        // View/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)width / (float)height, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader->setMat4("projection", projection);
        shader->setMat4("view", view);

        // World transformation
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        if (model != nullptr) {
            modelMatrix = glm::scale(modelMatrix, modelScale);
            // Center the model
            modelMatrix = glm::translate(modelMatrix, -model->getCenter());
        }
        shader->setMat4("model", modelMatrix);

        // Light properties
        shader->setVec3("lightPos", lightPos);
        shader->setVec3("lightColor", lightColor);
        shader->setVec3("viewPos", camera.Position);
        shader->setFloat("ambientStrength", ambientStrength);
        shader->setFloat("diffuseStrength", diffuseStrength);
        shader->setFloat("specularStrength", specularStrength);
        shader->setFloat("shininess", shininess);
        
        // Set default object color (light gray)
        shader->setVec3("objectColor", glm::vec3(0.8f, 0.8f, 0.8f));

        if (model != nullptr) {
            model->Draw(*shader);
        }

        renderUI();
        glfwSwapBuffers(window);
    }
}

void Renderer::initGLFW() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Enable cursor
    glfwWindowHint(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Renderer::initGLAD() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return;
    }
    
    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Enable sRGB framebuffer
    glEnable(GL_FRAMEBUFFER_SRGB);
    
    // Enable seamless cubemap sampling
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    
    // Set default texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create and compile shaders
    shader = new Shader("shaders/phong.vert", "shaders/phong.frag");
}

void Renderer::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // Enable keyboard navigation
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.FrameBorderSize = 1.0f;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Set initial window positions
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
}

void Renderer::processInput() {
    // Only process keyboard input if ImGui doesn't want it
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

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
    // Model loading window
    {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(250, 100), ImGuiCond_FirstUseEver);
        ImGui::Begin("Model Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        if (ImGui::Button("Load Model", ImVec2(120, 0))) {
            std::string filePath = openFileDialog();
            if (!filePath.empty()) {
                loadModel(filePath.c_str());
            }
        }
        
        ImGui::SameLine();
        if (model == nullptr) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No model");
        } else if (!model->isValid()) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Load failed");
        } else {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Loaded: %s", model->getFilename().c_str());
        }
        
        ImGui::End();
    }

    // Lighting controls window
    {
        ImGui::SetNextWindowPos(ImVec2(10, 120), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_FirstUseEver);
        ImGui::Begin("Lighting Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        ImGui::ColorEdit3("Light Color", &lightColor[0]);
        ImGui::DragFloat3("Light Position", &lightPos[0], 0.1f);
        
        ImGui::SliderFloat("Ambient", &ambientStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("Diffuse", &diffuseStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("Specular", &specularStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("Shininess", &shininess, 1.0f, 256.0f);
        
        ImGui::End();
    }

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
    // Don't update anything if the window is minimized
    if (width == 0 || height == 0)
        return;
        
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->width = width;
    renderer->height = height;
    glViewport(0, 0, width, height);
}

void Renderer::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    ImGuiIO& io = ImGui::GetIO();

    // Update ImGui's mouse button state
    if (action == GLFW_PRESS && button >= 0 && button < 5) {
        io.MouseDown[button] = true;
    }
    else if (action == GLFW_RELEASE && button >= 0 && button < 5) {
        io.MouseDown[button] = false;
    }
}

void Renderer::mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    ImGuiIO& io = ImGui::GetIO();

    // Update ImGui mouse position
    io.MousePos = ImVec2(static_cast<float>(xposIn), static_cast<float>(yposIn));

    // If ImGui wants to capture mouse, don't process camera movement
    if (io.WantCaptureMouse) {
        return;
    }

    // Only process camera movement if right mouse button is pressed
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS) {
        renderer->firstMouse = true;
        return;
    }

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (renderer->firstMouse) {
        renderer->lastX = xpos;
        renderer->lastY = ypos;
        renderer->firstMouse = false;
        return;
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
        return;
    }

    // Get model bounds
    glm::vec3 modelSize = model->getSize();
    glm::vec3 modelCenter = model->getCenter();
    
    // Calculate aspect ratios
    float screenAspect = static_cast<float>(width) / static_cast<float>(height);
    float modelAspect = modelSize.x / modelSize.y;
    
    // First, determine the target size we want the model to appear in view
    // Use the diagonal size of the model as a reference
    float modelDiagonal = glm::length(modelSize);
    float targetSize = 4.0f; // We want models to fit comfortably in view
    float scale = targetSize / modelDiagonal;
    
    // For very small models, ensure they're not too tiny
    float minScale = 2.0f;
    scale = std::max(scale, minScale);
    modelScale = glm::vec3(scale);

    // Calculate FOV and distance
    float desiredFOV = 45.0f; // Base FOV in degrees
    camera.Zoom = desiredFOV;
    
    // Calculate the required distance based on model's diagonal size
    float distance = modelDiagonal * 1.5f; // Base distance on diagonal for consistency
    
    // Ensure minimum viewing distance
    float minDistance = 5.0f;
    distance = std::max(distance, minDistance);

    // Calculate optimal camera position
    // Position camera higher up and at 45 degrees for better perspective
    float horizontalAngle = glm::radians(45.0f);
    float verticalAngle = glm::radians(35.0f); // Higher angle for better overview
    
    // Calculate camera position using spherical coordinates
    camera.Position = modelCenter + distance * glm::vec3(
        cos(verticalAngle) * cos(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * sin(horizontalAngle)
    );

    // Calculate camera orientation to look at center
    glm::vec3 direction = modelCenter - camera.Position;
    float yaw = glm::degrees(atan2(direction.z, direction.x));
    float pitch = glm::degrees(asin(direction.y / glm::length(direction)));
    
    // Set camera parameters
    camera.WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.Yaw = yaw;
    camera.Pitch = pitch;
    camera.updateCameraVectors();

    // Position light relative to model bounds
    lightPos = modelCenter + glm::vec3(
        modelSize.x * 0.5f,
        modelSize.y * 2.0f,
        modelSize.z * 0.5f
    );
    
    // Debug output
    std::cout << "Model diagonal size: " << modelDiagonal << std::endl;
    std::cout << "Applied scale: " << scale << std::endl;
    std::cout << "Camera distance: " << distance << std::endl;
    std::cout << "Camera position: " << camera.Position.x << ", " << camera.Position.y << ", " << camera.Position.z << std::endl;
} 