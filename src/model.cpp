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
    // No need to clean up textures anymore since we're using material colors
}

void Model::Draw(Shader &shader) {
    if (!m_isValid || meshes.empty()) {
        return;
    }

    // We always have material colors
    shader.setBool("hasTexture", true);
    for (auto& mesh : meshes) {
        mesh.Draw(shader);
    }
}

bool Model::loadModel(std::string path) {
    std::cout << "Loading model from path: " << path << std::endl;
    
    // Store the filename
    size_t last_slash = path.find_last_of("/\\");
    filename = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);
    
    // Configure Assimp to handle material textures properly
    importer.SetPropertyInteger(AI_CONFIG_IMPORT_FBX_READ_TEXTURES, 1);
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, 1);
    
    // FBX specific configurations
    importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);
    importer.SetPropertyInteger(AI_CONFIG_IMPORT_FBX_READ_MATERIALS, 1);
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_TEXTURES, true);
    
    // Now load with full processing
    unsigned int importFlags = 
        aiProcess_Triangulate | 
        aiProcess_GenNormals | 
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_PreTransformVertices |
        aiProcess_GenUVCoords |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ValidateDataStructure |
        aiProcess_FindInvalidData |
        aiProcess_FixInfacingNormals |
        aiProcess_OptimizeMeshes;
    
    // Add FBX-specific processing for files with .fbx extension
    if (path.substr(path.find_last_of(".") + 1) == "fbx") {
        importFlags |= aiProcess_ConvertToLeftHanded;  // FBX files often need this
    }
    
    scene = importer.ReadFile(path, importFlags);
    
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return false;
    }
    
    directory = path.substr(0, path.find_last_of("/\\"));
    std::cout << "\nModel directory: " << directory << std::endl;
    std::cout << "Number of materials: " << scene->mNumMaterials << std::endl;
    
    // Print detailed material info
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* material = scene->mMaterials[i];
        aiString name;
        material->Get(AI_MATKEY_NAME, name);
        std::cout << "\nMaterial " << i << ":" << std::endl;
        std::cout << "Name: " << name.C_Str() << std::endl;
        
        // Check for textures first
        aiString texPath;
        if(AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath)) {
            std::cout << "Diffuse texture path: " << texPath.C_Str() << std::endl;
        }
        
        // Then check material colors
        aiColor4D diffuse(1.0f);
        if(AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse)) {
            std::cout << "Diffuse color: " << diffuse.r << ", " << diffuse.g << ", " << diffuse.b << std::endl;
        }
        
        // Check for FBX specific properties
        aiColor4D baseColor(1.0f);
        if(AI_SUCCESS == material->Get(AI_MATKEY_BASE_COLOR, baseColor)) {
            std::cout << "Base color: " << baseColor.r << ", " << baseColor.g << ", " << baseColor.b << std::endl;
            // Use base color if diffuse wasn't set
            if (diffuse.r == 1.0f && diffuse.g == 1.0f && diffuse.b == 1.0f) {
                diffuse = baseColor;
            }
        }
    }

    try {
        processNode(scene->mRootNode, scene);
    } catch (const std::exception& e) {
        std::cerr << "ERROR::MODEL::LOADING: Exception while processing model: " << e.what() << std::endl;
        return false;
    }

    if (meshes.empty()) {
        std::cerr << "ERROR::MODEL::LOADING: No meshes were loaded from the model" << std::endl;
        return false;
    }

    std::cout << "Model loaded successfully with " << meshes.size() << " meshes" << std::endl;
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
        
        // Get material colors
        aiColor4D diffuse(1.0f, 1.0f, 1.0f, 1.0f);
        aiColor4D specular(1.0f, 1.0f, 1.0f, 1.0f);
        float shininess = 32.0f;

        aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse);
        aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &specular);
        aiGetMaterialFloat(material, AI_MATKEY_SHININESS, &shininess);

        std::cout << "Material properties:" << std::endl;
        std::cout << "Diffuse: " << diffuse.r << ", " << diffuse.g << ", " << diffuse.b << std::endl;
        std::cout << "Specular: " << specular.r << ", " << specular.g << ", " << specular.b << std::endl;
        std::cout << "Shininess: " << shininess << std::endl;
        
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, 
            aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        
        std::vector<Texture> specularMaps = loadMaterialTextures(material, 
            aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        // Store material properties in first texture
        if (!textures.empty()) {
            textures[0].diffuseColor = glm::vec3(diffuse.r, diffuse.g, diffuse.b);
        } else {
            // If no textures, create a dummy texture to store the material color
            Texture colorTexture;
            colorTexture.id = 0; // No texture
            colorTexture.type = "texture_diffuse";
            colorTexture.path = ""; // Empty path since it's just a color
            colorTexture.diffuseColor = glm::vec3(diffuse.r, diffuse.g, diffuse.b);
            textures.push_back(colorTexture);
        }
    } else {
        std::cout << "Mesh has no material" << std::endl;
        // Create default material
        Texture defaultTexture;
        defaultTexture.id = 0;
        defaultTexture.type = "texture_diffuse";
        defaultTexture.path = "";
        defaultTexture.diffuseColor = glm::vec3(0.8f); // Default gray color
        textures.push_back(defaultTexture);
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
    
    std::cout << "\nProcessing material textures of type: " << typeName << std::endl;
    std::cout << "Number of textures found: " << numTextures << std::endl;
    
    // Get material properties
    aiColor4D diffuse(1.0f);
    aiColor4D specular(1.0f);
    float shininess = 32.0f;
    
    // Try different material properties in order of preference
    if(AI_SUCCESS == mat->Get(AI_MATKEY_BASE_COLOR, diffuse)) {
        std::cout << "Found BASE_COLOR: " << diffuse.r << ", " << diffuse.g << ", " << diffuse.b << std::endl;
    } else if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse)) {
        std::cout << "Found COLOR_DIFFUSE: " << diffuse.r << ", " << diffuse.g << ", " << diffuse.b << std::endl;
    }
    
    if(AI_SUCCESS == mat->Get(AI_MATKEY_SPECULAR_FACTOR, specular)) {
        std::cout << "Found SPECULAR_FACTOR: " << specular.r << ", " << specular.g << ", " << specular.b << std::endl;
    } else if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_SPECULAR, specular)) {
        std::cout << "Found COLOR_SPECULAR: " << specular.r << ", " << specular.g << ", " << specular.b << std::endl;
    }
    
    if(AI_SUCCESS == mat->Get(AI_MATKEY_SHININESS, shininess)) {
        std::cout << "Found SHININESS: " << shininess << std::endl;
    }
    
    // First check for textures
    if (numTextures > 0) {
        for(unsigned int i = 0; i < numTextures; i++) {
            aiString texPath;
            if (AI_SUCCESS == mat->GetTexture(type, i, &texPath)) {
                std::cout << "Found texture path: " << texPath.C_Str() << std::endl;
                
                Texture texture;
                texture.id = TextureFromFile(texPath.C_Str(), directory);
                texture.type = typeName;
                texture.path = texPath.C_Str();
                texture.diffuseColor = glm::vec3(diffuse.r, diffuse.g, diffuse.b);
                texture.specularColor = glm::vec3(specular.r, specular.g, specular.b);
                texture.shininess = shininess;
                
                if (texture.id != 0) {
                    std::cout << "Successfully loaded texture with ID: " << texture.id << std::endl;
                    textures.push_back(texture);
                } else {
                    std::cout << "Failed to load texture, using material color as fallback" << std::endl;
                }
            }
        }
    }
    
    // If no valid textures were loaded, create a color-only texture
    if (textures.empty()) {
        std::cout << "No valid textures found, creating color-only texture" << std::endl;
        Texture texture;
        texture.id = 0;
        texture.type = typeName;
        texture.path = "";
        texture.diffuseColor = glm::vec3(diffuse.r, diffuse.g, diffuse.b);
        texture.specularColor = glm::vec3(specular.r, specular.g, specular.b);
        texture.shininess = shininess;
        textures.push_back(texture);
    }
    
    return textures;
}

unsigned int Model::TextureFromFile(const char *path, const std::string &directory) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    std::string filename;
    std::string pathStr(path);
    
    // Handle different path formats
    if (pathStr.find(":/") != std::string::npos || pathStr.find(":\\") != std::string::npos) {
        // Absolute path
        filename = pathStr;
    } else if (directory.empty()) {
        filename = pathStr;
    } else {
        // Try multiple possible texture locations
        std::vector<std::string> possiblePaths = {
            directory + "/" + pathStr,
            directory + "/Textures/" + pathStr,
            directory + "/../Textures/" + pathStr,
            pathStr
        };
        
        bool found = false;
        for (const auto& testPath : possiblePaths) {
            FILE* file = fopen(testPath.c_str(), "rb");
            if (file) {
                fclose(file);
                filename = testPath;
                found = true;
                std::cout << "Found texture at: " << filename << std::endl;
                break;
            }
        }
        
        if (!found) {
            std::cout << "Failed to find texture in any of these locations:" << std::endl;
            for (const auto& path : possiblePaths) {
                std::cout << "  - " << path << std::endl;
            }
            return 0;
        }
    }
    
    std::cout << "Loading texture from: " << filename << std::endl;

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    
    if (data) {
        GLenum format;
        GLenum internalFormat;
        if (nrComponents == 1) {
            format = GL_RED;
            internalFormat = GL_RED;
        }
        else if (nrComponents == 3) {
            format = GL_RGB;
            internalFormat = GL_SRGB8;
        }
        else if (nrComponents == 4) {
            format = GL_RGBA;
            internalFormat = GL_SRGB8_ALPHA8;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "Texture loaded successfully: " << width << "x" << height << " with " << nrComponents << " components" << std::endl;
    }
    else {
        std::cout << "Failed to load texture: " << filename << std::endl;
        std::cout << "STB Error: " << stbi_failure_reason() << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }

    return textureID;
} 