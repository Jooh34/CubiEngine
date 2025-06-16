#pragma once

float Gaussian(float x, float mean, float stddev);

void CreateGaussianBlurWeight(float Weights[MAX_GAUSSIAN_KERNEL_SIZE], int KernelSize, float StdDev);

void CreateRandomFloats(float* RandomFloats, int NumFloats);

void GenerateSimpleTangentVector(XMFLOAT3& InNormal, XMFLOAT3* OutTangent);
