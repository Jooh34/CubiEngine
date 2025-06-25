#pragma once

#include "Graphics/Raytracing.h"
#include "Graphics/Resource.h"
#include "ShaderInterlop/RenderResources.hlsli"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "Math/Transform.h"

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

	XMMATRIX GetModelMatrix() const { return Transform.GetModelMatrix(); }
	XMMATRIX GetInverseModelMatrix() const { return Transform.GetInverseModelMatrix(); }

    FBuffer PositionBuffer{};
    FBuffer TextureCoordsBuffer{};
    FBuffer NormalBuffer{};
    FBuffer TangentBuffer{};
    FBuffer IndexBuffer{};
    uint32_t IndicesCount{};
    
    std::shared_ptr<FPBRMaterial> Material{};

    FTransform Transform{};

private:
    std::shared_ptr<FRaytracingGeometry> RaytracingGeometry;
};
