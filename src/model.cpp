#include "model.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <iostream>

Model::Model(const char* path) {
    if (!path) {
        std::cerr << "ERROR::MODEL::CONSTRUCTOR: Null path provided" << std::endl;
        m_isValid = false;
        return;
    }
    m_isValid = loadModel(path);
}

Model::~Model() {
    // Clean up textures
    for (const auto& texture : textures_loaded) {
        if (texture.id != 0) {
            glDeleteTextures(1, &texture.id);
        }
    }
}

void Model::Draw(Shader &shader) {
    if (!m_isValid || meshes.empty()) {
        // Silently return if model isn't valid or has no meshes
        return;
    }

    shader.setBool("hasTexture", !textures_loaded.empty());
    for (auto& mesh : meshes) {
        mesh.Draw(shader);
    }
}

bool Model::loadModel(std::string path) {
    std::cout << "Loading model from path: " << path << std::endl;
    
    scene = importer.ReadFile(path, 
        aiProcess_Triangulate | 
        aiProcess_GenNormals | 
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_PreTransformVertices |
        aiProcess_GenUVCoords);  // Add this flag for GLB files
    
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return false;
    }
    
    // For GLB files, we don't need the directory as textures are embedded
    directory = "";
    std::cout << "Number of materials: " << scene->mNumMaterials << std::endl;
    std::cout << "Number of meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "Number of embedded textures: " << scene->mNumTextures << std::endl;

    try {
        processNode(scene->mRootNode, scene);
    } catch (const std::exception& e) {
        std::cerr << "ERROR::MODEL::LOADING: Exception while processing model: " << e.what() << std::endl;
        return false;
    }

    // Verify we loaded at least one mesh
    if (meshes.empty()) {
        std::cerr << "ERROR::MODEL::LOADING: No meshes were loaded from the model" << std::endl;
        return false;
    }

    std::cout << "Model loaded successfully with " << meshes.size() << " meshes and " << textures_loaded.size() << " textures" << std::endl;
    return true;
}

void Model::processNode(aiNode *node, const aiScene *scene) {
    // process all the node's meshes (if any)
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    // then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
    std::cout << "Processing mesh: " << (mesh->mName.length > 0 ? mesh->mName.C_Str() : "unnamed") << std::endl;
    std::cout << "Has texture coords: " << (mesh->mTextureCoords[0] != nullptr ? "yes" : "no") << std::endl;
    
    std::vector<Vertex> vertices = getVertices(mesh);
    std::vector<unsigned int> indices = getIndices(mesh);
    std::vector<Texture> textures;

    if(mesh->mMaterialIndex >= 0) {
        std::cout << "Processing material index: " << mesh->mMaterialIndex << std::endl;
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, 
            aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        
        std::vector<Texture> specularMaps = loadMaterialTextures(material, 
            aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    } else {
        std::cout << "Mesh has no material" << std::endl;
    }
    
    std::cout << "Mesh processed with " << vertices.size() << " vertices, " << indices.size() << " indices, and " << textures.size() << " textures" << std::endl;
    return Mesh(vertices, indices, textures);
}

void Model::updateBounds(const glm::vec3& point) {
    minBounds.x = std::min(minBounds.x, point.x);
    minBounds.y = std::min(minBounds.y, point.y);
    minBounds.z = std::min(minBounds.z, point.z);
    
    maxBounds.x = std::max(maxBounds.x, point.x);
    maxBounds.y = std::max(maxBounds.y, point.y);
    maxBounds.z = std::max(maxBounds.z, point.z);
}

std::vector<Vertex> Model::getVertices(aiMesh *mesh) {
    std::vector<Vertex> vertices;
    
    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        
        // Process vertex positions, normals and texture coordinates
        glm::vec3 vector;
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;
        
        // Update bounding box
        updateBounds(vector);
        
        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.Normal = vector;
        
        if(mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        }
        else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        
        vertices.push_back(vertex);
    }
    return vertices;
}

std::vector<unsigned int> Model::getIndices(aiMesh *mesh) {
    std::vector<unsigned int> indices;
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    return indices;
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName) {
    std::vector<Texture> textures;
    unsigned int numTextures = mat->GetTextureCount(type);
    std::cout << "Loading " << numTextures << " textures of type " << typeName << std::endl;
    
    for(unsigned int i = 0; i < numTextures; i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::cout << "Processing texture path: " << str.C_Str() << std::endl;
        
        // Check if texture was loaded before
        bool skip = false;
        for(unsigned int j = 0; j < textures_loaded.size(); j++) {
            if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                textures.push_back(textures_loaded[j]);
                skip = true;
                std::cout << "Reusing already loaded texture" << std::endl;
                break;
            }
        }
        if(!skip) {   // If texture hasn't been loaded already, load it
            Texture texture;
            texture.id = TextureFromFile(str.C_Str(), directory);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            textures_loaded.push_back(texture);
            std::cout << "Loaded new texture with ID: " << texture.id << std::endl;
        }
    }
    return textures;
}

unsigned int Model::TextureFromFile(const char *path, const std::string &directory) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    // Check if this is an embedded texture (path starts with *)
    if (path[0] == '*') {
        // Get the embedded texture index
        int textureIndex = std::stoi(path + 1);  // Skip the '*' character
        const aiTexture* texture = scene->mTextures[textureIndex - 1]; // Assimp indices are 1-based
        
        std::cout << "Loading embedded texture " << textureIndex << " (format: " << texture->achFormatHint << ")" << std::endl;
        
        int width, height, nrComponents;
        unsigned char* data = nullptr;
        
        if (texture->mHeight == 0) {
            // Compressed texture data
            if (strncmp(texture->achFormatHint, "png", 3) == 0 ||
                strncmp(texture->achFormatHint, "jpg", 3) == 0) {
                // Handle compressed image formats
                data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(texture->pcData), 
                                           texture->mWidth, &width, &height, &nrComponents, 4);
                nrComponents = 4; // Force RGBA
            } else {
                std::cout << "Unsupported texture format: " << texture->achFormatHint << std::endl;
                return textureID;
            }
        } else {
            // Raw texture data
            width = texture->mWidth;
            height = texture->mHeight;
            data = reinterpret_cast<unsigned char*>(texture->pcData);
            nrComponents = 4; // Assuming RGBA
        }

        if (data) {
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            std::cout << "Embedded texture loaded successfully: " << width << "x" << height 
                      << " with " << nrComponents << " components" << std::endl;

            if (texture->mHeight == 0) { // Only free if we used stbi_load
                stbi_image_free(data);
            }
        } else {
            std::cout << "Failed to load embedded texture " << textureIndex << std::endl;
        }
        return textureID;
    }

    // Regular external texture loading
    std::string filename = std::string(path);
    filename = directory + '/' + filename;
    std::cout << "Loading external texture from: " << filename << std::endl;

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        std::cout << "External texture loaded successfully: " << width << "x" << height 
                  << " with " << nrComponents << " components" << std::endl;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "External texture failed to load at path: " << filename << std::endl;
        stbi_image_free(data);
    }

    return textureID;
} 