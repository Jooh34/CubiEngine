#pragma once

float Gaussian(float x, float mean, float stddev);

void CreateGaussianBlurWeight(float Weights[MAX_GAUSSIAN_KERNEL_SIZE], int KernelSize, float StdDev);

void CreateRandomFloats(float* RandomFloats, int NumFloats);

void GenerateSimpleTangentVector(const XMFLOAT3& InNormal, XMFLOAT3* OutTangent);

void GenerateSimpleTangentVectorList(
	std::vector<XMFLOAT3>& OutTangents,
	const std::vector<XMFLOAT3>& Positions,
	const std::vector<XMFLOAT3>& Normals,
	const std::vector<UINT>& Indice
);

void GenerateTangentVectorList(
	std::vector<XMFLOAT3>& OutTangents,
	const std::vector<XMFLOAT3>& Positions,
	const std::vector<XMFLOAT2>& TextureCoords,
	const std::vector<UINT>& Indice
);

inline float DegreeToRadian(float Degree)
{
	return Degree * (M_PI / 180.0f);
}
