cmake_minimum_required(VERSION 3.10)
project(BasicRenderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GLFW
add_subdirectory(external/glfw)

# GLM
add_subdirectory(external/glm)

# Assimp
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)  # Build static library
add_subdirectory(external/assimp)

# ImGui
set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

# Add source files
set(SOURCES 
    src/main.cpp
    src/shader.cpp
    src/model.cpp
    src/camera.cpp
    src/mesh.cpp
    src/renderer.cpp
    src/glad.c
    ${IMGUI_SOURCES}
)

# Add header files
set(HEADERS
    include/shader.h
    include/model.h
    include/camera.h
    include/mesh.h
    include/renderer.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Make sure assimp is built before our project
add_dependencies(${PROJECT_NAME} assimp)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    external/glm
    external/assimp/include
    ${CMAKE_BINARY_DIR}/external/assimp/include
    external/imgui
    external/imgui/backends
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    assimp
)

# Copy shaders to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR}) 