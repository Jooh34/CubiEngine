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

struct FGraphicsPipelineStateCreationDesc
{
    ShaderModule ShaderModule{};
    std::vector<DXGI_FORMAT> RtvFormats{DXGI_FORMAT_R16G16B16A16_FLOAT};
    uint32_t RtvCount{1u};
    DXGI_FORMAT DepthFormat{DXGI_FORMAT_D32_FLOAT};
    D3D12_COMPARISON_FUNC DepthComparisonFunc{ D3D12_COMPARISON_FUNC_GREATER_EQUAL }; // reversedZ
    FrontFaceWindingOrder FrontFaceWindingOrder{FrontFaceWindingOrder::ClockWise};
    D3D12_CULL_MODE CullMode{D3D12_CULL_MODE_BACK};
    std::wstring_view PipelineName{};

    // advanced
    D3D12_DEPTH_WRITE_MASK DepthWriteMask{ D3D12_DEPTH_WRITE_MASK_ALL };
};

struct FComputePipelineStateCreationDesc
{
    ShaderModule ShaderModule{};
    std::wstring_view PipelineName{};
};

struct FRaytracingPipelineStateCreationDesc
{
    std::wstring_view rtShaderPath{};
    std::wstring_view EntryPointRGS{ L"RayGen" };
    std::wstring_view EntryPointCHS{ L"Hit" };
    std::wstring_view EntryPointRMS{ L"Miss" };
	std::wstring_view EntryPointAHS{ L"" }; // empty means no any AHS
};

struct FAllocation
{
    void Update(const void* Data, const size_t Size);
    void Reset();

    wrl::ComPtr<D3D12MA::Allocation> DmaAllocation{};
    std::optional<void*> MappedPointer{};
    wrl::ComPtr<ID3D12Resource> Resource{};
};

struct FBuffer
{
    // To be used primarily for constant buffers.
    void Update(const void* data);

    FAllocation Allocation{};
    size_t SizeInBytes{};
    size_t NumElement{};
    size_t ElementSizeInBytes{};

    uint32_t SrvIndex{ INVALID_INDEX_U32 };
    uint32_t UavIndex{ INVALID_INDEX_U32 };
    uint32_t CbvIndex{ INVALID_INDEX_U32 };

    ID3D12Resource* GetResource() const
    {
        return Allocation.Resource.Get();
    }
};

enum class ETextureUsage
{
    DepthStencil,
    RenderTarget,
    TextureFromPath,
    TextureFromData,
    HDRTextureFromPath,
    DDSTextureFromPath,
    CubeMap,
    UAVTexture,
};

enum class EAlphaMode
{
    Opaque = 0,
    Mask,
    Blend,
};

struct FTextureCreationDesc
{
    ETextureUsage Usage{};
    uint32_t Width{};
    uint32_t Height{};
    DXGI_FORMAT Format{ DXGI_FORMAT_R8G8B8A8_UNORM };
    D3D12_RESOURCE_STATES InitialState { D3D12_RESOURCE_STATE_COMMON };
    uint32_t MipLevels{ 1u };
    uint32_t DepthOrArraySize{ 1u };
    std::wstring Name{};
    std::wstring Path{};
};

struct FTexture
{
    ID3D12Resource* GetResource() const;
    uint32_t Width{};
    uint32_t Height{};
    FAllocation Allocation{};
    ETextureUsage Usage{};
    DXGI_FORMAT Format{};
    
    uint32_t SrvIndex{ INVALID_INDEX_U32 };
    uint32_t UavIndex{ INVALID_INDEX_U32 };
    uint32_t DsvIndex{ INVALID_INDEX_U32 };
    uint32_t RtvIndex{ INVALID_INDEX_U32 };

    std::vector<uint32_t> MipUavIndex{};

    D3D12_RESOURCE_STATES ResourceState{};

    std::wstring DebugName{};

    bool IsPowerOfTwo();
    bool IsDepthFormat();

    static DXGI_FORMAT ConvertToLinearFormat(const DXGI_FORMAT Format);
    static bool IsSRGB(DXGI_FORMAT Format);
    static bool IsCompressedFormat(DXGI_FORMAT Format);
	static uint32_t GetBytesPerPixel(DXGI_FORMAT Format);
    static bool IsUAVAllowed(ETextureUsage Usage, DXGI_FORMAT Format);

    operator bool() const
    {
        return GetResource() != nullptr;
    }

    ~FTexture()
    {
        // notify to DebugTexture
    }
};

struct FIntRect
{
    uint32_t Width;
    uint32_t Height;
};

struct FSceneTexture
{
    FIntRect Size{};
    std::unique_ptr<FTexture> GBufferA; // Albedo
    std::unique_ptr<FTexture> GBufferB; // Normal
    std::unique_ptr<FTexture> GBufferC; // AO + MetalRoughness
    std::unique_ptr<FTexture> VelocityTexture;

    std::unique_ptr<FTexture> DepthTexture;
    std::unique_ptr<FTexture> PrevDepthTexture;

    std::unique_ptr<FTexture> LDRTexture;
    std::unique_ptr<FTexture> HDRTexture;
};

struct FCbvCreationDesc
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC CbvDesc;
};

struct FSrvCreationDesc
{
    D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc;
};

struct FUavCreationDesc
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc;
};

struct FDsvCreationDesc
{
    D3D12_DEPTH_STENCIL_VIEW_DESC DsvDesc;
};

struct FRtvCreationDesc
{
    D3D12_RENDER_TARGET_VIEW_DESC RtvDesc;
};

struct FSamplerCreationDesc
{
    D3D12_SAMPLER_DESC SamplerDesc{};
};

struct FSampler
{
    uint32_t SamplerIndex{ INVALID_INDEX_U32 };
};

struct FResourceCreationDesc
{
    D3D12_RESOURCE_DESC ResourceDesc{};

    static FResourceCreationDesc CreateBufferResourceCreationDesc(const uint64_t size)
    {
        FResourceCreationDesc resourceCreationDesc = {
            .ResourceDesc{
                .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                .Width = size,
                .Height = 1u,
                .DepthOrArraySize = 1u,
                .MipLevels = 1u,
                .Format = DXGI_FORMAT_UNKNOWN,
                .SampleDesc{.Count = 1u, .Quality = 0u},
                .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                .Flags = D3D12_RESOURCE_FLAG_NONE,
            },
        };

        return resourceCreationDesc;
    }
};

enum class EBufferUsage
{
    UploadBuffer,
    IndexBuffer,
    StructuredBuffer,
    ConstantBuffer,
    StructuredBufferUAV
};

struct FBufferCreationDesc
{
    EBufferUsage Usage{};
    std::wstring_view Name{};
};

struct FModelCreationDesc
{
    std::string_view ModelPath{};
    std::wstring_view ModelName{};

    Dx::XMFLOAT3 Rotation{ 0.0f, 0.0f, 0.0f };
    Dx::XMFLOAT3 Scale{ 1.0f, 1.0f, 1.0f };
    Dx::XMFLOAT3 Translate{ 0.0f, 0.0f, 0.0f };

    Dx::XMFLOAT3 OverrideBaseColorValue{ -1.0f, -1.0f, -1.0f };
    float OverrideRoughnessValue{ -1.0f };
    float OverrideMetallicValue{ -1.0f };
	Dx::XMFLOAT3 OverrideEmissiveValue{ -1.0f, -1.0f, -1.0f };

    float RefractionFactor{ 0 };
    float IOR{ 1 };
};

struct FMeshCreationDesc
{
    std::wstring_view Name{};

    Dx::XMFLOAT3 Rotation{ 0.0f, 0.0f, 0.0f };
    Dx::XMFLOAT3 Scale{ 1.0f, 1.0f, 1.0f };
    Dx::XMFLOAT3 Translate{ 0.0f, 0.0f, 0.0f };

    Dx::XMFLOAT3 BaseColorValue{ -1.0f, -1.0f, -1.0f };
    float RoughnessValue{ -1.0f };
    float MetallicValue{ -1.0f };
    Dx::XMFLOAT3 EmissiveValue{ -1.0f, -1.0f, -1.0f };

    float RefractionFactor{ 0 };
    float IOR{ 1 };
};

enum class EShadowMethod
{
    None = 0,
    ShadowMap,
    Raytracing,
};

