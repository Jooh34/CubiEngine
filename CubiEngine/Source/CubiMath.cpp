#include "CubiMath.h"
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