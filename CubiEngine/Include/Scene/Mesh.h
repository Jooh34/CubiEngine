#pragma once

#include "Graphics/Raytracing.h"
#include "Graphics/Resource.h"
#include "ShaderInterlop/RenderResources.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"

class FPBRMaterial;
class FGraphicsContext;

class FMesh
{
public:
    FMesh();

    void Render(const FGraphicsContext* const GraphicsContext,
         interlop::UnlitPassRenderResources& UnlitRenderResources) const;
	void Render(const FGraphicsContext* const GraphicsContext, FScene* Scene,
         interlop::DeferredGPassRenderResources& DeferredGPassRenderResources) const;
    void Render(const FGraphicsContext* const GraphicsContext,
         interlop::ShadowDepthPassRenderResource& ShadowDepthPassRenderResource) const;

    void GenerateRaytracingGeometry(const FGraphicsDevice* const GraphicsDevice);

    void GatherRaytracingGeometry(std::vector<FRaytracingGeometryContext>& RaytracingGeometryContextList);

    FBuffer PositionBuffer{};
    FBuffer TextureCoordsBuffer{};
    FBuffer NormalBuffer{};
    FBuffer TangentBuffer{};

    FBuffer IndexBuffer{};

    uint32_t IndicesCount{};
    
    std::shared_ptr<FPBRMaterial> Material{};

    XMMATRIX ModelMatrix{};
    XMMATRIX InverseModelMatrix{};

    // Raytracing Buffers
    std::vector<interlop::MeshVertex> MeshVertices{};
    std::vector<uint16_t> Indice{};

private:
    std::shared_ptr<FRaytracingGeometry> RaytracingGeometry;
};
