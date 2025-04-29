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

    FBuffer TransformBuffer{};
    XMMATRIX TransformMatrix{ Dx::XMMatrixIdentity() };
    void Update();
};

struct FModelCreationDesc
{
    std::string_view ModelPath{};
    std::wstring_view ModelName{};

    Dx::XMFLOAT3 Rotation{0.0f, 0.0f, 0.0f};
    Dx::XMFLOAT3 Scale{1.0f, 1.0f, 1.0f};
    Dx::XMFLOAT3 Translate{0.0f, 0.0f, 0.0f};
};

class FModel
{
public:
    FModel(const FGraphicsDevice* const GraphicsDevice, const FModelCreationDesc& ModelCreationDesc);

    void Render(const FGraphicsContext* const GraphicsContext,
        interlop::UnlitPassRenderResources& UnlitRenderResources);
    void Render(const FGraphicsContext* const GraphicsContext, FScene* Scene,
        interlop::DeferredGPassRenderResources& DeferredGPassRenderResources);
    void Render(const FGraphicsContext* const GraphicsContext,
        interlop::ShadowDepthPassRenderResource& ShadowDepthPassRenderResource);

    void GatherRaytracingGeometry(std::vector<FRaytracingGeometryContext>& RaytracingGeometryContextList);

private:
    std::wstring ModelName;
    std::string ModelDir;

    std::vector<FSampler> Samplers;
    std::vector<FPBRMaterial> Materials;
    
    std::vector<FMesh> Meshes{};

    FTransform Transform;


    void LoadSamplers(const FGraphicsDevice* const GraphicsDevice, const tinygltf::Model& GLTFModel);
    void LoadMaterials(const FGraphicsDevice* const GraphicsDevice, const tinygltf::Model& GLTFModel);
    void LoadNode(const FGraphicsDevice* const GraphicsDevice, const FModelCreationDesc& ModelCreationDesc,
         const uint32_t NodeIndex, const tinygltf::Model& GLTFModel);
};