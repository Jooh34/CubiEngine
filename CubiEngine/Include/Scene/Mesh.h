#pragma once

#include "Graphics/Raytracing.h"
#include "Graphics/Resource.h"
#include "ShaderInterlop/ConstantBuffers.hlsli"

struct FPBRMaterial;

class FMesh
{
public:
    FMesh();

    void GenerateRaytracingGeometry(const FGraphicsDevice* const GraphicsDevice);

    void GatherRaytracingGeometry(std::vector<FRaytracingGeometryContext>& RaytracingGeometryContextList, FPBRMaterial* Material);

    FBuffer PositionBuffer{};
    FBuffer TextureCoordsBuffer{};
    FBuffer NormalBuffer{};
    FBuffer TangentBuffer{};

    FBuffer IndexBuffer{};

    uint32_t IndicesCount{};

    uint32_t MaterialIndex{};

    XMMATRIX TransformMatrix{};

    // Raytracing Buffers
    std::vector<interlop::MeshVertex> MeshVertices{};
    std::vector<uint16_t> Indice{};

private:
    std::shared_ptr<FRaytracingGeometry> RaytracingGeometry;
};
