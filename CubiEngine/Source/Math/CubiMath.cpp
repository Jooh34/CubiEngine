#include "Math/CubiMath.h"
#include <cmath>
#include <random>

float Gaussian(float x, float mean, float stddev)
{
    const float a = (x - mean) / stddev;
    return exp(-0.5 * a * a);
}

void CreateGaussianBlurWeight(float Weights[MAX_GAUSSIAN_KERNEL_SIZE], int KernelSize, float StdDev)
{
    assert(KernelSize <= MAX_GAUSSIAN_KERNEL_SIZE);

    for (int i = 0; i < (KernelSize + 1) / 2; ++i)
        Weights[i] = Weights[KernelSize - 1 - i] = Gaussian(i, (KernelSize - 1) / 2.0, StdDev);

    float WeightSum = 0.0;
    for (int i = 0; i < KernelSize; i++)
        WeightSum += Weights[i];

    for (int i = 0; i < (KernelSize + 1) / 2; ++i)
        Weights[i] = Weights[KernelSize - 1 - i] = Weights[i] / WeightSum;
}

void CreateRandomFloats(float* RandomFloats, int NumFloats)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (int i = 0; i < NumFloats; ++i) {
        float value = dist(gen);
        RandomFloats[i] = value;
    }
}

void GenerateSimpleTangentVector(const XMFLOAT3& InNormal, XMFLOAT3* OutTangent)
{
    XMVECTOR NormalVector = XMLoadFloat3(&InNormal);
    NormalVector = XMVector3Normalize(NormalVector);

    XMVECTOR UpVector = (fabs(XMVectorGetZ(NormalVector)) < 0.999f) ? XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f) : XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR Tangent = XMVector3Normalize(XMVector3Cross(UpVector, NormalVector));

    XMStoreFloat3(OutTangent, Tangent);
}

void GenerateSimpleTangentVectorList(std::vector<XMFLOAT3>& OutTangents,
    const std::vector<XMFLOAT3>& Positions, const std::vector<XMFLOAT3>& Normals, const std::vector<UINT>& Indice)
{
    OutTangents.resize(Positions.size(), XMFLOAT3(0.0f, 0.0f, 0.0f));

    // If no normal texture, set tangents to fake
    for (size_t i = 0; i < Indice.size(); i = i + 3)
    {
        UINT index0 = Indice[i];
        UINT index1 = Indice[i + 1];
        UINT index2 = Indice[i + 2];

        GenerateSimpleTangentVector(Normals[index0], &OutTangents[index0]);
        GenerateSimpleTangentVector(Normals[index1], &OutTangents[index1]);
        GenerateSimpleTangentVector(Normals[index2], &OutTangents[index2]);
    }
}

void GenerateTangentVectorList(std::vector<XMFLOAT3>& OutTangents,
    const std::vector<XMFLOAT3>& Positions, const std::vector<XMFLOAT2>& TextureCoords, const std::vector<UINT>& Indice)
{
    OutTangents.resize(Positions.size(), XMFLOAT3(0.0f, 0.0f, 0.0f));

    for (size_t i = 0; i < Indice.size(); i += 3)
    {
        UINT index0 = Indice[i];
        UINT index1 = Indice[i + 1];
        UINT index2 = Indice[i + 2];

        const XMFLOAT3& pos0 = Positions[index0];
        const XMFLOAT3& pos1 = Positions[index1];
        const XMFLOAT3& pos2 = Positions[index2];

        const XMFLOAT2& uv0 = TextureCoords[index0];
        const XMFLOAT2& uv1 = TextureCoords[index1];
        const XMFLOAT2& uv2 = TextureCoords[index2];

        // Calculate edges
        XMFLOAT3 edge1 = XMFLOAT3(pos1.x - pos0.x, pos1.y - pos0.y, pos1.z - pos0.z);
        XMFLOAT3 edge2 = XMFLOAT3(pos2.x - pos0.x, pos2.y - pos0.y, pos2.z - pos0.z);

        float deltaU1 = uv1.x - uv0.x;
        float deltaV1 = uv1.y - uv0.y;
        float deltaU2 = uv2.x - uv0.x;
        float deltaV2 = uv2.y - uv0.y;

        float f = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);

        XMFLOAT3 tangent = {
            f * (deltaV2 * edge1.x - deltaV1 * edge2.x),
            f * (deltaV2 * edge1.y - deltaV1 * edge2.y),
            f * (deltaV2 * edge1.z - deltaV1 * edge2.z)
        };

        // Accumulate the tangent
        OutTangents[index0].x += tangent.x;
        OutTangents[index0].y += tangent.y;
        OutTangents[index0].z += tangent.z;

        OutTangents[index1].x += tangent.x;
        OutTangents[index1].y += tangent.y;
        OutTangents[index1].z += tangent.z;

        OutTangents[index2].x += tangent.x;
        OutTangents[index2].y += tangent.y;
        OutTangents[index2].z += tangent.z;
    }
}
