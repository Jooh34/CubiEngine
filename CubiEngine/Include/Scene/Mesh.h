#pragma once

#include "Graphics/Raytracing.h"
#include "Graphics/Resource.h"

class FMesh
{
public:
    FMesh();

    void GenerateRaytracingGeometry(const FGraphicsDevice* const GraphicsDevice);

    void GatherRaytracingGeometry(std::vector<BLASMatrixPairType>& BLASMatrixPair);

    FBuffer PositionBuffer{};
    FBuffer TextureCoordsBuffer{};
    FBuffer NormalBuffer{};
    FBuffer TangentBuffer{};

    FBuffer IndexBuffer{};

    uint32_t IndicesCount{};

    uint32_t MaterialIndex{};

    XMMATRIX TransformMatrix{};

private:
    std::shared_ptr<FRaytracingGeometry> RaytracingGeometry;
};
