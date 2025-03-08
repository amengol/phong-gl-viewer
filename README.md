# 3D Model Viewer with Phong Shading

A modern OpenGL-based 3D model viewer with Phong shading and interactive controls. This project demonstrates various computer graphics concepts including:

- Modern OpenGL usage with GLFW and GLAD
- 3D model loading with Assimp
- Phong shading implementation
- Interactive camera controls
- Real-time lighting parameter adjustments with ImGui

## Features

- Load and display 3D models in various formats (OBJ, FBX, etc.)
- Phong shading with adjustable parameters:
  - Ambient light intensity
  - Diffuse reflection strength
  - Specular highlights
  - Shininess
- Interactive camera controls:
  - WASD for movement
  - Mouse for looking around
  - Scroll wheel for zoom
  - Space/Ctrl for up/down movement
- Real-time light position and color adjustment
- Modern UI with ImGui

## Building

### Prerequisites

- CMake 3.10 or higher
- C++17 compatible compiler
- OpenGL 3.3 or higher capable GPU

### Build Steps

1. Clone the repository:
```bash
git clone <repository-url>
cd Basic-Renderer
```

2. Create a build directory and navigate to it:
```bash
mkdir build
cd build
```

3. Generate build files with CMake:
```bash
cmake ..
```

4. Build the project:
```bash
cmake --build .
```

## Usage

1. Run the executable:
```bash
./BasicRenderer
```

2. Controls:
- WASD - Move camera forward/left/backward/right
- Mouse - Look around
- Mouse wheel - Zoom in/out
- Space - Move up
- Left Control - Move down
- Escape - Exit program

3. UI Controls:
- Use the ImGui window to adjust lighting parameters
- Drag the light position gizmo to move the light source
- Adjust color picker to change light color
- Use sliders to modify Phong shading parameters

## Dependencies

All dependencies are automatically downloaded and built by CMake:

- GLFW (Window management and OpenGL context)
- GLM (Mathematics)
- Assimp (3D model loading)
- ImGui (User interface)
- GLAD (OpenGL function loading)

## License

This project is licensed under the MIT License - see the LICENSE file for details. 