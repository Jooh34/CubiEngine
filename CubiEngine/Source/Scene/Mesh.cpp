#include "Scene/Mesh.h"
#include "Graphics/Material.h"
#include "Graphics/GraphicsContext.h"
#include "Scene/Scene.h"

FMesh::FMesh()
{
}

void FMesh::Render(const FGraphicsContext* const GraphicsContext,
    interlop::UnlitPassRenderResources& UnlitRenderResources) const
{
    GraphicsContext->SetIndexBuffer(IndexBuffer);

	UnlitRenderResources.modelMatrix = ModelMatrix;

    UnlitRenderResources.albedoTextureIndex = Material->AlbedoTexture.SrvIndex;
    UnlitRenderResources.albedoTextureSamplerIndex = Material->AlbedoSampler.SamplerIndex;

    UnlitRenderResources.materialBufferIndex = Material->MaterialBuffer.CbvIndex;

    UnlitRenderResources.positionBufferIndex = PositionBuffer.SrvIndex;
    UnlitRenderResources.textureCoordBufferIndex = TextureCoordsBuffer.SrvIndex;

    GraphicsContext->SetGraphicsRoot32BitConstants(&UnlitRenderResources);
    GraphicsContext->DrawIndexedInstanced(IndicesCount);
}

void FMesh::Render(const FGraphicsContext* const GraphicsContext, FScene* Scene,
	interlop::DeferredGPassRenderResources& DeferredGPassRenderResources) const
{
	GraphicsContext->SetIndexBuffer(IndexBuffer);

	DeferredGPassRenderResources.modelMatrix = ModelMatrix;
	DeferredGPassRenderResources.inverseModelMatrix = InverseModelMatrix;

	DeferredGPassRenderResources.albedoTextureIndex = Material->AlbedoTexture.SrvIndex;
	DeferredGPassRenderResources.albedoTextureSamplerIndex = Material->AlbedoSampler.SamplerIndex;

	DeferredGPassRenderResources.metalRoughnessTextureIndex = Material->MetalRoughnessTexture.SrvIndex;
	DeferredGPassRenderResources.metalRoughnessTextureSamplerIndex = Material->MetalRoughnessSampler.SamplerIndex;

	DeferredGPassRenderResources.normalTextureIndex = Material->NormalTexture.SrvIndex;
	DeferredGPassRenderResources.normalTextureSamplerIndex = Material->NormalSampler.SamplerIndex;

	DeferredGPassRenderResources.aoTextureIndex = Material->AOTexture.SrvIndex;
	DeferredGPassRenderResources.aoTextureSamplerIndex = Material->AOSampler.SamplerIndex;

	DeferredGPassRenderResources.emissiveTextureIndex = Material->EmissiveTexture.SrvIndex;
	DeferredGPassRenderResources.emissiveTextureSamplerIndex = Material->EmissiveSampler.SamplerIndex;

	DeferredGPassRenderResources.materialBufferIndex = Material->MaterialBuffer.CbvIndex;

	DeferredGPassRenderResources.positionBufferIndex = PositionBuffer.SrvIndex;
	DeferredGPassRenderResources.textureCoordBufferIndex = TextureCoordsBuffer.SrvIndex;
	DeferredGPassRenderResources.normalBufferIndex = NormalBuffer.SrvIndex;
	DeferredGPassRenderResources.tangentBufferIndex = TangentBuffer.SrvIndex;
	DeferredGPassRenderResources.debugBufferIndex = Scene->GetDebugBuffer().CbvIndex;

	GraphicsContext->SetGraphicsRoot32BitConstants(&DeferredGPassRenderResources);
	GraphicsContext->DrawIndexedInstanced(IndicesCount);
}

void FMesh::Render(const FGraphicsContext* const GraphicsContext,
	interlop::ShadowDepthPassRenderResource& ShadowDepthPassRenderResource) const
{
	GraphicsContext->SetIndexBuffer(IndexBuffer);

	ShadowDepthPassRenderResource.modelMatrix = ModelMatrix;

	ShadowDepthPassRenderResource.positionBufferIndex = PositionBuffer.SrvIndex;

	GraphicsContext->SetGraphicsRoot32BitConstants(&ShadowDepthPassRenderResource);
	GraphicsContext->DrawIndexedInstanced(IndicesCount);
}

void FMesh::GenerateRaytracingGeometry(const FGraphicsDevice* const GraphicsDevice)
{
    std::pair<ComPtr<ID3D12Resource>, uint32_t> A = std::pair<ComPtr<ID3D12Resource>, uint32_t>(PositionBuffer.GetResource(), (uint32_t)PositionBuffer.NumElement);
    std::pair<ComPtr<ID3D12Resource>, uint32_t> B = std::pair<ComPtr<ID3D12Resource>, uint32_t>(IndexBuffer.GetResource(), (uint32_t)IndexBuffer.NumElement);

    RaytracingGeometry = make_shared<FRaytracingGeometry>( GraphicsDevice, A, B );
}

void FMesh::GatherRaytracingGeometry(std::vector<FRaytracingGeometryContext>& RaytracingGeometryContextList)
{
    if (RaytracingGeometry)
    {
        RaytracingGeometryContextList.emplace_back(
            this,
            Material.get(),
            RaytracingGeometry->GetBLAS(),
            ModelMatrix
        );
    }
}
