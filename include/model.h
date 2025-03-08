#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <vector>
#include "mesh.h"
#include "shader.h"

class Model {
public:
    Model(const char* path);
    void Draw(Shader &shader);
    bool isValid() const { return m_isValid; }

private:
    std::vector<Mesh> meshes;
    std::string directory;
    bool m_isValid = false;

    bool loadModel(std::string path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Vertex> getVertices(aiMesh *mesh);
    std::vector<unsigned int> getIndices(aiMesh *mesh);
}; 