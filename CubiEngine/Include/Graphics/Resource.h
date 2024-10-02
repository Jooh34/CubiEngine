#pragma once

static uint32_t INVALID_INDEX_U32 = 0xFFFFFFFF;

struct ShaderModule
{
    std::wstring_view vertexShaderPath{};
    std::wstring_view vertexEntryPoint{ L"VsMain" };

    std::wstring_view pixelShaderPath{};
    std::wstring_view pixelEntryPoint{ L"PsMain" };

    std::wstring_view computeShaderPath{};
    std::wstring_view computeEntryPoint{ L"CsMain" };
};

enum class FrontFaceWindingOrder
{
    ClockWise,
    CounterClockWise
};

struct GraphicsPipelineStateCreationDesc
{
    ShaderModule shaderModule{};
    std::vector<DXGI_FORMAT> rtvFormats{DXGI_FORMAT_R16G16B16A16_FLOAT};
    uint32_t rtvCount{1u};
    DXGI_FORMAT depthFormat{DXGI_FORMAT_D32_FLOAT};
    D3D12_COMPARISON_FUNC depthComparisonFunc{D3D12_COMPARISON_FUNC_LESS};
    FrontFaceWindingOrder frontFaceWindingOrder{FrontFaceWindingOrder::ClockWise};
    D3D12_CULL_MODE cullMode{D3D12_CULL_MODE_BACK};
    std::wstring_view pipelineName{};
};

struct Texture
{
    uint32_t Width{};
    uint32_t Height{};

    wrl::ComPtr<ID3D12Resource> Resource{};

    uint32_t SrvIndex{ INVALID_INDEX_U32 };
    uint32_t UavIndex{ INVALID_INDEX_U32 };
    uint32_t DsvIndex{ INVALID_INDEX_U32 };
    uint32_t RtvIndex{ INVALID_INDEX_U32 };
};
