#include "Graphics/Dx12Helper.h"

D3D12_STATIC_SAMPLER_DESC GetStaticSamplerDesc(SamplerState samplerState, uint32_t shaderRegister,
    uint32_t registerSpace, D3D12_SHADER_VISIBILITY visibility)
{
    D3D12_STATIC_SAMPLER_DESC staticDesc = { };
    if (samplerState == SamplerState::Linear)
    {
        staticDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticDesc.MipLODBias = 0.0f;
        staticDesc.MaxAnisotropy = 1;
        staticDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        staticDesc.MinLOD = 0;
        staticDesc.MaxLOD = D3D12_FLOAT32_MAX;
    }
    else if (samplerState == SamplerState::LinearClamp)
    {
        staticDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticDesc.MipLODBias = 0.0f;
        staticDesc.MaxAnisotropy = 1;
        staticDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        staticDesc.MinLOD = 0;
        staticDesc.MaxLOD = D3D12_FLOAT32_MAX;
    }
    else if (samplerState == SamplerState::Anisotropic)
    {
        staticDesc.Filter = D3D12_FILTER_ANISOTROPIC;
        staticDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticDesc.MipLODBias = 0.0f;
        staticDesc.MaxAnisotropy = 16;
        staticDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        staticDesc.MinLOD = 0;
        staticDesc.MaxLOD = D3D12_FLOAT32_MAX;
    }

    staticDesc.ShaderRegister = shaderRegister;
    staticDesc.RegisterSpace = registerSpace;
    staticDesc.ShaderVisibility = visibility;

    return staticDesc;
}
