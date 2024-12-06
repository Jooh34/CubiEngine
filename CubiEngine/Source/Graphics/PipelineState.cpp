#include "Graphics/PipelineState.h"
#include "Graphics/ShaderCompiler.h"
#include "Core/FileSystem.h"

FPipelineState::FPipelineState(ID3D12Device5* const device,
    const FGraphicsPipelineStateCreationDesc& pipelineStateCreationDesc)
{
    constexpr D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {
       .BlendEnable = FALSE,
       .LogicOpEnable = FALSE,
       .SrcBlend = D3D12_BLEND_SRC_ALPHA,
       .DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
       .BlendOp = D3D12_BLEND_OP_ADD,
       .SrcBlendAlpha = D3D12_BLEND_ONE,
       .DestBlendAlpha = D3D12_BLEND_ZERO,
       .BlendOpAlpha = D3D12_BLEND_OP_ADD,
       .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
    };

    D3D12_BLEND_DESC blendDesc = {
        .AlphaToCoverageEnable = FALSE,
        .IndependentBlendEnable = FALSE,
    };

    for (const uint32_t i : std::views::iota(0u, pipelineStateCreationDesc.RtvCount))
    {
        blendDesc.RenderTarget[i] = renderTargetBlendDesc;
    }

    // Setup depth stencil state.
    const D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {
        .DepthEnable = pipelineStateCreationDesc.DepthFormat == DXGI_FORMAT_UNKNOWN ? FALSE : TRUE,
        .DepthWriteMask = pipelineStateCreationDesc.DepthWriteMask,
        .DepthFunc = pipelineStateCreationDesc.DepthComparisonFunc,
        .StencilEnable = FALSE,
        .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
        .StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
    };

    const auto& vertexShaderBlob =
        ShaderCompiler::Compile(
            ShaderTypes::Vertex,
            FFileSystem::GetFullPath(pipelineStateCreationDesc.ShaderModule.vertexShaderPath),
            pipelineStateCreationDesc.ShaderModule.vertexEntryPoint)
        .shaderBlob;

    const auto& pixelShaderBlob =
        ShaderCompiler::Compile(
            ShaderTypes::Pixel,
            FFileSystem::GetFullPath(pipelineStateCreationDesc.ShaderModule.pixelShaderPath),
            pipelineStateCreationDesc.ShaderModule.pixelEntryPoint)
        .shaderBlob;

    // Primitive topology type specifies how the pipeline interprets geometry or hull shader input primitives.
    // Basically, it sets up the rasterizer for the given primitive type. The primitive type must match with the IA
    // Topology type.

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature = FPipelineState::StaticRootSignature.Get(),
        .VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()),
        .PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()),
        .BlendState = blendDesc,
        .SampleMask = UINT32_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .DepthStencilState = depthStencilDesc,
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = pipelineStateCreationDesc.RtvCount,
        .DSVFormat = pipelineStateCreationDesc.DepthFormat,
        .SampleDesc{.Count = 1u, .Quality = 0u},
        .NodeMask = 0u,
    };

    // Modify rasterizer state if front face winding order is counter clock wise.
    if (pipelineStateCreationDesc.FrontFaceWindingOrder == FrontFaceWindingOrder::CounterClockWise)
    {
        psoDesc.RasterizerState.FrontCounterClockwise = true;
    }

    psoDesc.RasterizerState.CullMode = pipelineStateCreationDesc.CullMode;

    // Set RTV formats.
    for (const uint32_t i : std::views::iota(0u, pipelineStateCreationDesc.RtvCount))
    {
        psoDesc.RTVFormats[i] = pipelineStateCreationDesc.RtvFormats[i];
    }

    ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PipelineStateObject)));

    PipelineStateObject->SetName(pipelineStateCreationDesc.PipelineName.data());
}

FPipelineState::FPipelineState(ID3D12Device5* const device, const FComputePipelineStateCreationDesc& pipelineStateCreationDesc)
{
    const auto& ComputeShaderBlob =
        ShaderCompiler::Compile(ShaderTypes::Compute,
        FFileSystem::GetFullPath(pipelineStateCreationDesc.CsShaderPath),
        L"CsMain").shaderBlob;

    const D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature = FPipelineState::StaticRootSignature.Get(),
        .CS = {.pShaderBytecode = ComputeShaderBlob->GetBufferPointer(),
               .BytecodeLength = ComputeShaderBlob->GetBufferSize()},
        .NodeMask = 0u,
    };

    ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&PipelineStateObject)));

    PipelineStateObject->SetName(pipelineStateCreationDesc.PipelineName.data());
}

void FPipelineState::CreateBindlessRootSignature(ID3D12Device* const device, const std::wstring_view shaderPath)
{
    const auto path = FFileSystem::GetFullPath(shaderPath);
    const auto shader = ShaderCompiler::Compile(ShaderTypes::Vertex, path, L"VsMain", true);

    if (!shader.rootSignatureBlob.Get())
    {
        FatalError(std::format("Shader : {} not found.", wStringToString(path)));
    }

    ThrowIfFailed(device->CreateRootSignature(0u, shader.rootSignatureBlob->GetBufferPointer(),
        shader.rootSignatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&StaticRootSignature)));

    StaticRootSignature->SetName(L"Bindless Root Signature");
}
