#include "Scene/Mesh.h"
#include "Graphics/Material.h"

FMesh::FMesh()
{
}

void FMesh::GenerateRaytracingGeometry(const FGraphicsDevice* const GraphicsDevice)
{
    std::pair<ComPtr<ID3D12Resource>, uint32_t> A = std::pair<ComPtr<ID3D12Resource>, uint32_t>(PositionBuffer.GetResource(), (uint32_t)PositionBuffer.NumElement);
    std::pair<ComPtr<ID3D12Resource>, uint32_t> B = std::pair<ComPtr<ID3D12Resource>, uint32_t>(IndexBuffer.GetResource(), (uint32_t)IndexBuffer.NumElement);

    RaytracingGeometry = make_shared<FRaytracingGeometry>( GraphicsDevice, A, B );
}

void FMesh::GatherRaytracingGeometry(std::vector<FRaytracingGeometryContext>& RaytracingGeometryContextList, FPBRMaterial* Material)
{
    if (RaytracingGeometry)
    {
        RaytracingGeometryContextList.emplace_back(
            this,
            Material,
            RaytracingGeometry->GetBLAS(),
            TransformMatrix
        );
    }
}
