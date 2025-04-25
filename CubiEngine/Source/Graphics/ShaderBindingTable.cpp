#include "Graphics/ShaderBindingTable.h"
#include "Graphics/GraphicsDevice.h"

FShaderBindingTable::FShaderBindingTable(const FGraphicsDevice* const GraphicsDevice, ComPtr<ID3D12StateObjectProperties>& RTStateObjectProps)
{
    void* heapPointer = reinterpret_cast<void*>(GraphicsDevice->GetCbvSrvUavDescriptorHeap()->GetDescriptorHandleFromStart().GpuDescriptorHandle.ptr);

    sbtGenerator.AddRayGenerationProgram(L"RayGen", { heapPointer });
    sbtGenerator.AddMissProgram(L"Miss", {});
    sbtGenerator.AddHitGroup(L"HitGroup", {});

    uint32_t sbtSize = sbtGenerator.ComputeSBTSize();
    
    GraphicsDevice->CreateRawBuffer(
        SbtResource, sbtSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);

    if (!SbtResource)
    {
        throw std::runtime_error("Failed to create SBT resource");
    }

    sbtGenerator.Generate(SbtResource.Get(), RTStateObjectProps.Get());
}

D3D12_DISPATCH_RAYS_DESC FShaderBindingTable::CreateRayDesc(UINT Width, UINT Height)
{
    D3D12_DISPATCH_RAYS_DESC RayDesc = {};
    // The layout of the SBT is as follows:
    // ray generation shader, miss shaders, hit groups. As described in the CreateShaderBindingTable method,
    // all SBT entries of a given type have the same size to allow a fixed stride.

    // The ray generation shaders are always at the beginning of the SBT.
    uint32_t RayGenerationSectionSizeInBytes = sbtGenerator.GetRayGenSectionSize();
    RayDesc.RayGenerationShaderRecord.StartAddress = GetSBTStorageAddress();
    RayDesc.RayGenerationShaderRecord.SizeInBytes = RayGenerationSectionSizeInBytes;

    uint32_t MissSectionSizeInBytes = sbtGenerator.GetMissSectionSize();
    RayDesc.MissShaderTable.StartAddress = GetSBTStorageAddress() + RayGenerationSectionSizeInBytes;
    RayDesc.MissShaderTable.SizeInBytes = MissSectionSizeInBytes;
    RayDesc.MissShaderTable.StrideInBytes = sbtGenerator.GetMissEntrySize();

    // The hit groups section start after the miss shaders. In this sample we
    // have one 1 hit group for the triangle
    uint32_t HitGroupsSectionSize = sbtGenerator.GetHitGroupSectionSize();
    RayDesc.HitGroupTable.StartAddress = GetSBTStorageAddress() + RayGenerationSectionSizeInBytes + MissSectionSizeInBytes;
    RayDesc.HitGroupTable.SizeInBytes = HitGroupsSectionSize;
    RayDesc.HitGroupTable.StrideInBytes = sbtGenerator.GetHitGroupEntrySize();

    // Dimensions of the image to render, identical to a kernel launch dimension
    RayDesc.Width = Width;
    RayDesc.Height = Height;
    RayDesc.Depth = 1;

    return RayDesc;
}

