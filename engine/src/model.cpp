#include "model.h"

#include <iostream>

#include "glad/glad.h"

Model::Model(std::string path) {
    loadModel(path);
}

Model::Model(std::string path, std::shared_ptr<Material> material) : materialOverride(material) {
    loadModel(path);
}

void
Model::Draw(Shader& shader) {
    for (auto& mesh : meshes) {
        mesh.Draw(shader);
    }
}

void
Model::loadModel(std::string path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
    }

    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
}

// recursively load all meshes in the node tree
void
Model::processNode(aiNode* node, const aiScene* scene) {
    std::cout << "Process node: " << node->mName.C_Str() << std::endl;

    // process all of this node's meshes if it has any
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    // continue with children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

// convert assimp mesh to our own mesh class
Mesh
Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Material material;

    if (materialOverride) {
        material = *materialOverride;
    }

    // vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        // position
        glm::vec3 position;
        position.x = mesh->mVertices[i].x;
        position.y = mesh->mVertices[i].y;
        position.z = mesh->mVertices[i].z;

        vertex.position = position;

        // normal
        glm::vec3 normal;
        normal.x = mesh->mNormals[i].x;
        normal.y = mesh->mNormals[i].y;
        normal.z = mesh->mNormals[i].z;

        vertex.normal = normal;

        // texture coordinates
        if (mesh->mTextureCoords[0]) {
            glm::vec2 textureCoordinates;
            textureCoordinates.x = mesh->mTextureCoords[0][i].x;
            textureCoordinates.y = mesh->mTextureCoords[0][i].y;
            vertex.textureCoordinates = textureCoordinates;
        }
        else {
            vertex.textureCoordinates = glm::vec2(0.0f, 0.0f);
        }

        // tangents
        glm::vec3 tangent;
        tangent.x = mesh->mTangents[0].x;
        tangent.y = mesh->mTangents[0].y;
        tangent.z = mesh->mTangents[0].z;
        vertex.tangent = tangent;

        // bitangents
        glm::vec3 bitangent;
        bitangent.x = mesh->mBitangents[0].x;
        bitangent.y = mesh->mBitangents[0].y;
        bitangent.z = mesh->mBitangents[0].z;
        vertex.bitangent = bitangent;

        vertices.push_back(vertex);
    }

    // indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // material
    if (!materialOverride) {
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* aiMaterial = scene->mMaterials[mesh->mMaterialIndex];

            // albedo
            if (aiMaterial->GetTextureCount(aiTextureType_DIFFUSE)) {
                material.useTextureAlbedo = true;
                material.textureAlbedo = loadMaterialTexture(aiMaterial, aiTextureType_DIFFUSE);
            }

            // metallicRoughness (in gltf 2.0 they are combined in one texture)
            if (aiMaterial->GetTextureCount(aiTextureType_UNKNOWN)) {
                // defined here in assimp https://github.com/assimp/assimp/blob/master/include/assimp/pbrmaterial.h#L57
                material.useTextureMetallicRoughness = true;
                material.textureMetallicRoughness = loadMaterialTexture(aiMaterial, aiTextureType_UNKNOWN);
            }

            // normal
            if (aiMaterial->GetTextureCount(aiTextureType_NORMALS)) {
                material.useTextureNormal = true;
                material.textureNormal = loadMaterialTexture(aiMaterial, aiTextureType_NORMALS);
            }

            // ambient occlusion
            if (aiMaterial->GetTextureCount(aiTextureType_LIGHTMAP)) {
                material.useTextureAmbientOcclusion = true;
                material.textureAmbientOcclusion = loadMaterialTexture(aiMaterial, aiTextureType_LIGHTMAP);
            }

            // emissive
            if (aiMaterial->GetTextureCount(aiTextureType_EMISSIVE)) {
                material.useTextureEmissive = true;
                material.textureEmissive = loadMaterialTexture(aiMaterial, aiTextureType_EMISSIVE);
            }
        }
    }

    return Mesh(vertices, indices, material);
}

// loads the first texture of given type
std::shared_ptr<Texture>
Model::loadMaterialTexture(aiMaterial* material, aiTextureType type) {
    aiString path;
    material->GetTexture(type, 0, &path);

    // check if we already have it loaded and use that if so
    auto iterator = texturesLoaded.find(std::string(path.C_Str()));
    if (iterator != texturesLoaded.end()) {
        return iterator->second;
    }

    auto texture = std::make_shared<Texture>();

    std::cout << "Process material: " << path.C_Str() << std::endl;

    texture->id = textureFromFile(path.C_Str(), directory, type);
    texture->path = path.C_Str();

    // cache it for future lookups
    texturesLoaded.insert(std::pair<std::string, std::shared_ptr<Texture>>(path.C_Str(), texture));

    return texture;
}

unsigned int
Model::textureFromFile(const char* fileName, std::string directory, aiTextureType type) {
    int width, height, numChannels;

    std::string relativePath = fileName;
    std::string path = directory + '/' + relativePath;

    unsigned char* data = stbi_load(path.c_str(), &width, &height, &numChannels, 0);

    if (!data) {
        std::cout << "Failed to load texture data" << std::endl;
        stbi_image_free(data);
    }

    GLenum format;

    switch (numChannels) {
    case 1:
        format = GL_RED;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 4:
        format = GL_RGBA;
        break;
    }

    GLenum internalFormat = format;

    // account for sRGB textures here
    //
    // diffuse textures are in sRGB space (non-linear)
    // metallic/roughness/normals are usually in linear
    // AO depends
    if (type == aiTextureType_DIFFUSE) {
        if (internalFormat == GL_RGB) {
            internalFormat = GL_SRGB;
        }
        else if (internalFormat == GL_RGBA) {
            internalFormat = GL_SRGB_ALPHA;
        }
    }

    unsigned int textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // generate the texture
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // texture wrapping/filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // image is resized using bilinear filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // image is enlarged using bilinear filtering

    // free the image data
    stbi_image_free(data);

    return textureId;
}