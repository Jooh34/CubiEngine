#pragma once

#include "Graphics/Resource.h"

class FPipelineState
{
public:
    FPipelineState(ID3D12Device5* const device,
        const GraphicsPipelineStateCreationDesc& pipelineStateCreationDesc);
    
    // The shader path passed in needs to be relative (with respect to root directory), it will internally find the
    // complete path (with respect to the executable).
    static void CreateBindlessRootSignature(ID3D12Device* const device, const std::wstring_view shaderPath);
    
    wrl::ComPtr<ID3D12PipelineState> PipelineStateObject{};

private:
    static inline wrl::ComPtr<ID3D12RootSignature> StaticRootSignature{};
};