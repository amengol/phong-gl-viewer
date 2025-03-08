#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <vector>
#include <map>
#include "mesh.h"
#include "shader.h"

class Model {
public:
    Model(const char* path);
    ~Model();
    void Draw(Shader &shader);
    bool isValid() const { return m_isValid; }

    // Add getters for model dimensions
    glm::vec3 getCenter() const { return (maxBounds + minBounds) * 0.5f; }
    glm::vec3 getSize() const { return maxBounds - minBounds; }
    glm::vec3 getMinBounds() const { return minBounds; }
    glm::vec3 getMaxBounds() const { return maxBounds; }
    std::string getFilename() const { return filename; }

private:
    std::vector<Mesh> meshes;
    std::string directory;
    std::string filename;
    bool m_isValid = false;
    std::vector<Texture> textures_loaded;
    Assimp::Importer importer;  // Keep importer alive
    const aiScene* scene = nullptr;  // Store the scene for texture loading

    // Bounding box information
    glm::vec3 minBounds = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

    bool loadModel(std::string path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Vertex> getVertices(aiMesh *mesh);
    std::vector<unsigned int> getIndices(aiMesh *mesh);
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
    unsigned int TextureFromFile(const char *path, const std::string &directory);
    void updateBounds(const glm::vec3& point);
}; 