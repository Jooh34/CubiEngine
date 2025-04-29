#pragma once

enum class SamplerState : uint64_t
{
    Linear = 0,
    LinearClamp = 0,
    Anisotropic,

    NumValues
};

D3D12_STATIC_SAMPLER_DESC GetStaticSamplerDesc(SamplerState samplerState, uint32_t shaderRegister = 0, uint32_t registerSpace = 0,
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_PIXEL);