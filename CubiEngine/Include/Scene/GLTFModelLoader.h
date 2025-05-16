#pragma once

#include "Graphics/Resource.h"
#include "Graphics/Material.h"
#include "Graphics/Raytracing.h"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Scene/Mesh.h"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

struct FTransform
{
    Dx::XMFLOAT3 Rotation{ 0.0f, 0.0f, 0.0f };
    Dx::XMFLOAT3 Scale{ 1.0f, 1.0f, 1.0f };
    Dx::XMFLOAT3 Translate{ 0.0f, 0.0f, 0.0f };

    XMMATRIX TransformMatrix{ Dx::XMMatrixIdentity() };
    void Update();
};

class FGLTFModelLoader
{
public:
    FGLTFModelLoader(const FGraphicsDevice* const GraphicsDevice, const FModelCreationDesc& ModelCreationDesc);

    std::wstring ModelName;
    std::string ModelDir;

    std::vector<FSampler> Samplers;
    std::vector<std::shared_ptr<FPBRMaterial>> Materials;
    
    std::vector<FMesh> Meshes{};

private:
    FTransform Transform;

    void LoadSamplers(const FGraphicsDevice* const GraphicsDevice, const tinygltf::Model& GLTFModel);
    void LoadMaterials(const FGraphicsDevice* const GraphicsDevice, const tinygltf::Model& GLTFModel);
    void LoadNode(const FGraphicsDevice* const GraphicsDevice, const FModelCreationDesc& ModelCreationDesc,
         const uint32_t NodeIndex, const tinygltf::Model& GLTFModel);
};