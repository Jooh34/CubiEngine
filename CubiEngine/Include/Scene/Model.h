#pragma once

#include "Graphics/Resource.h"
#include "Graphics/Material.h"

class FGraphicsDevice;

struct FTransform
{
    Dx::XMFLOAT3 Rotation{ 0.0f, 0.0f, 0.0f };
    Dx::XMFLOAT3 Scale{ 1.0f, 1.0f, 1.0f };
    Dx::XMFLOAT3 Translate{ 0.0f, 0.0f, 0.0f };

    //Buffer transformBuffer{};

    void update();

};

struct FModelCreationDesc
{
    std::string_view ModelPath{};
    std::string_view ModelName{};

    Dx::XMFLOAT3 Rotation{0.0f, 0.0f, 0.0f};
    Dx::XMFLOAT3 Scale{1.0f, 1.0f, 1.0f};
    Dx::XMFLOAT3 Translate{0.0f, 0.0f, 0.0f};
};

struct FMesh
{
    FBuffer PositionBuffer{};
    FBuffer TextureCoordsBuffer{};
    FBuffer NormalBuffer{};

    FBuffer IndexBuffer{};

    uint32_t IndicesCount{};

    uint32_t MaterialIndex{};
};

class FModel
{
public:
    FModel(const FGraphicsDevice* const GraphicsDevice, const FModelCreationDesc& ModelCreationDesc);

private:
    std::string ModelName;
    std::string ModelDir;

    std::vector<FSampler> Samplers;
    std::vector<FPBRMaterial> Materials;
    
    std::vector<FMesh> Meshes{};

    void LoadSamplers(const FGraphicsDevice* const GraphicsDevice, const tinygltf::Model& GLTFModel);
    void LoadMaterials(const FGraphicsDevice* const GraphicsDevice, const tinygltf::Model& GLTFModel);
    void LoadNode(const FGraphicsDevice* const GraphicsDevice, const FModelCreationDesc& ModelCreationDesc,
         const uint32_t NodeIndex, const tinygltf::Model& GLTFModel);
};