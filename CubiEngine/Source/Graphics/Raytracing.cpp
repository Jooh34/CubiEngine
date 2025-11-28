#include "Graphics/Raytracing.h"
#include "Graphics/D3D12DynamicRHI.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/Material.h"
#include "Scene/Mesh.h"
#include "Scene/Scene.h"

FRaytracingGeometry::FRaytracingGeometry(
    std::pair<wrl::ComPtr<ID3D12Resource>, uint32_t>& VertexBuffer,
    std::pair<wrl::ComPtr<ID3D12Resource>, uint32_t>& IndexBuffer)
{
    BLASGenerator bottomLevelASGenerator;
    bottomLevelASGenerator.AddVertexBuffer(
        VertexBuffer.first.Get(),
        0,
        VertexBuffer.second,
        sizeof(XMFLOAT3),
        IndexBuffer.first.Get(),
        0,
        IndexBuffer.second,
        0,
        0
    );

    UINT64 scratchSize, resultSize;
    bottomLevelASGenerator.ComputeASBufferSizes(RHIGetDevice(), false, &scratchSize, &resultSize);

    RHICreateRawBuffer(
        scratch,
        scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps
    );
    RHICreateRawBuffer(
        result,
        resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps
    );

    bottomLevelASGenerator.Generate(
        RHIGetCurrentGraphicsContext()->GetD3D12CommandList(),
        scratch.Get(),
        result.Get(),
        false,
        nullptr
    );
}

void FRaytracingScene::GenerateRaytracingScene(
    FGraphicsContext* const GraphicsContext,
    std::vector<FRaytracingGeometryContext>& RaytracingGeometryContextList
)
{
    if (pResult)
    {
        //Todo : rebuild if scene is changed
        return;
    }

    pScratch.Reset();
    pResult.Reset();
    pInstanceDesc.Reset();

    TopLevelASGenerator topLevelASGenerator;

    // Gather all the instances into the builder helper
    for (size_t i = 0; i < RaytracingGeometryContextList.size(); i++)
    {
        FRaytracingGeometryContext& Context = RaytracingGeometryContextList[i];
        topLevelASGenerator.AddInstance(Context.BLASResource,
            Context.ModelMatrix, static_cast<UINT>(i),
            static_cast<UINT>(0), Context.Material->AlphaMode == EAlphaMode::Opaque);
    }

    UINT64 scratchSize, resultSize, instanceDescsSize;

    topLevelASGenerator.ComputeASBufferSizes(RHIGetDevice(), true, &scratchSize,
        &resultSize, &instanceDescsSize);

    RHICreateRawBuffer(
        pScratch,
        scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        kDefaultHeapProps
    );

    RHICreateRawBuffer(
        pResult,
        resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        kDefaultHeapProps
    );

    RHICreateRawBuffer(
        pInstanceDesc,
        instanceDescsSize, D3D12_RESOURCE_FLAG_NONE,
        D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps
    );

    topLevelASGenerator.Generate(GraphicsContext->GetD3D12CommandList(),
        pScratch.Get(),
        pResult.Get(),
        pInstanceDesc.Get()
    );

    CreateTopLevelASResourceView();

    GenerateRaytracingBuffers(GraphicsContext, RaytracingGeometryContextList);
}

void FRaytracingScene::GenerateRaytracingBuffers(FGraphicsContext* const GraphicsContext, std::vector<FRaytracingGeometryContext>& RaytracingGeometryContextList)
{
    // Update Buffers
    uint32_t vtxOffset = 0;
    uint32_t idxOffset = 0;

    std::vector<interlop::FRaytracingGeometryInfo> GeometryInfoList;
    std::vector<interlop::FRaytracingMaterial> MaterialList;
    GeometryInfoList.reserve(RaytracingGeometryContextList.size());
    MaterialList.reserve(RaytracingGeometryContextList.size());

    std::vector<interlop::MeshVertex> MeshVertexList;
    std::vector<uint32_t> IndexList;

    for (size_t i = 0; i < RaytracingGeometryContextList.size(); i++)
    {
        FRaytracingGeometryContext& Context = RaytracingGeometryContextList[i];
        FMesh* Mesh = Context.Mesh;
        FPBRMaterial* Material = Context.Material;
        uint32_t MaterialSrvIndex = Context.Material->GetAlbedoSrv();

        if (Mesh)
        {
            // Geometry
            GeometryInfoList.push_back(
                interlop::FRaytracingGeometryInfo{
					Context.Mesh->PositionBuffer.SrvIndex,
					Context.Mesh->TextureCoordsBuffer.SrvIndex,
					Context.Mesh->NormalBuffer.SrvIndex,
					Context.Mesh->TangentBuffer.SrvIndex,
					Context.Mesh->IndexBuffer.SrvIndex,
					(uint32_t)i,
                }
            );

            // Material
            MaterialList.push_back(
                interlop::FRaytracingMaterial{
                    .albedoTextureIndex = MaterialSrvIndex,
                    .albedoTextureSamplerIndex = Material->AlbedoSampler.SamplerIndex,
                    .metalRoughnessTextureIndex = Material->GetMetalRoughnessSrv(),
                    .metalRoughnessTextureSamplerIndex = Material->MetalRoughnessSampler.SamplerIndex,
                    .normalTextureIndex = Material->GetNormalSrv(),
                    .normalTextureSamplerIndex = Material->NormalSampler.SamplerIndex,
					.ormTextureIndex = Material->GetORMTextureSrv(),
					.ormTextureSamplerIndex = Material->ORMSampler.SamplerIndex,
                    .albedoColor = Material->MaterialBufferData.albedoColor,
					.metallic = Material->MaterialBufferData.metallicFactor,
					.roughness = Material->MaterialBufferData.roughnessFactor,
                    .emissiveColor = Material->MaterialBufferData.emissiveColor,
					.refractionFactor = Material->MaterialBufferData.refractionFactor,
					.IOR = Material->MaterialBufferData.IOR,
                }
            );
        }
    }

    GeometryInfoBuffer =
        RHICreateBuffer<interlop::FRaytracingGeometryInfo>(FBufferCreationDesc{
            .Usage = EBufferUsage::StructuredBuffer,
            .Name = L" Raytracing Geometry Buffer",
        }, GeometryInfoList);

    MaterialBuffer =
        RHICreateBuffer<interlop::FRaytracingMaterial>(FBufferCreationDesc{
            .Usage = EBufferUsage::StructuredBuffer,
            .Name = L" Raytracing Material Buffer",
            }, MaterialList);
}

void FRaytracingScene::CreateTopLevelASResourceView()
{
    FSrvCreationDesc SrvCreationDesc = {
        .SrvDesc = {
            .Format = DXGI_FORMAT_UNKNOWN,
            .ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .RaytracingAccelerationStructure = {
                .Location = pResult->GetGPUVirtualAddress()
            }
        }
    };

    SrvIndex = RHICreateSrv(SrvCreationDesc, nullptr);
    GPUVirtualAddress = pResult->GetGPUVirtualAddress();
}

/*-----------------------------------------------------------------------
Copyright (c) 2014-2018, NVIDIA. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Neither the name of its contributors may be used to endorse
or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------*/

// Helper to compute aligned buffer sizes
#ifndef ROUND_UP
#define ROUND_UP(v, powerOf2Alignment) (((v) + (powerOf2Alignment)-1) & ~((powerOf2Alignment)-1))
#endif

void BLASGenerator::AddVertexBuffer(ID3D12Resource* vertexBuffer,
    UINT64 vertexOffsetInBytes,
    uint32_t vertexCount,
    UINT vertexSizeInBytes,
    ID3D12Resource* indexBuffer,
    UINT64 indexOffsetInBytes,
    uint32_t indexCount,
    ID3D12Resource* transformBuffer,
    UINT64 transformOffsetInBytes, bool isOpaque
)
{
    // Create the DX12 descriptor representing the input data, assumed to be
    // opaque triangles, with 3xf32 vertex coordinates and 32-bit indices
    D3D12_RAYTRACING_GEOMETRY_DESC descriptor = {};
    descriptor.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    descriptor.Triangles.VertexBuffer.StartAddress =
        vertexBuffer->GetGPUVirtualAddress() + vertexOffsetInBytes;
    descriptor.Triangles.VertexBuffer.StrideInBytes = vertexSizeInBytes;
    descriptor.Triangles.VertexCount = vertexCount;
    descriptor.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    descriptor.Triangles.IndexBuffer =
        indexBuffer ? (indexBuffer->GetGPUVirtualAddress() + indexOffsetInBytes) : 0;
    descriptor.Triangles.IndexFormat = indexBuffer ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_UNKNOWN;
    descriptor.Triangles.IndexCount = indexCount;
    descriptor.Triangles.Transform3x4 =
        transformBuffer ? (transformBuffer->GetGPUVirtualAddress() + transformOffsetInBytes) : 0;
    descriptor.Flags =
        isOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

    m_vertexBuffers.push_back(descriptor);
}

//--------------------------------------------------------------------------------------------------
// Compute the size of the scratch space required to build the acceleration
// structure, as well as the size of the resulting structure. The allocation of
// the buffers is then left to the application
void BLASGenerator::ComputeASBufferSizes(
    ID3D12Device5* device, // Device on which the build will be performed
    bool allowUpdate,                        // If true, the resulting acceleration structure will
    // allow iterative updates
    UINT64* scratchSizeInBytes,              // Required scratch memory on the GPU to build
    // the acceleration structure
    UINT64* resultSizeInBytes                // Required GPU memory to store the acceleration
    // structure
)
{
    // The generated AS can support iterative updates. This may change the final
    // size of the AS as well as the temporary memory requirements, and hence has
    // to be set before the actual build
    m_flags = allowUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE
        : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    // Describe the work being requested, in this case the construction of a
    // (possibly dynamic) bottom-level hierarchy, with the given vertex buffers
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc;
    prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    prebuildDesc.NumDescs = static_cast<UINT>(m_vertexBuffers.size());
    prebuildDesc.pGeometryDescs = m_vertexBuffers.data();
    prebuildDesc.Flags = m_flags;

    // This structure is used to hold the sizes of the required scratch memory and resulting AS
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};

    // Building the acceleration structure (AS) requires some scratch space, as well as space to store
    // the resulting structure This function computes a conservative estimate of the memory
    // requirements for both, based on the geometry size.
    device->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &info);

    // Buffer sizes need to be 256-byte-aligned
    *scratchSizeInBytes =
        ROUND_UP(info.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    *resultSizeInBytes =
        ROUND_UP(info.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    // Store the memory requirements for use during build
    m_scratchSizeInBytes = *scratchSizeInBytes;
    m_resultSizeInBytes = *resultSizeInBytes;
}

//--------------------------------------------------------------------------------------------------
// Enqueue the construction of the acceleration structure on a command list, using
// application-provided buffers and possibly a pointer to the previous acceleration structure in
// case of iterative updates. Note that the update can be done in place: the result and
// previousResult pointers can be the same.
void BLASGenerator::Generate(
    ID3D12GraphicsCommandList4* commandList, // Command list on which the build will be enqueued
    ID3D12Resource* scratchBuffer, // Scratch buffer used by the builder to
    // store temporary data
    ID3D12Resource* resultBuffer,  // Result buffer storing the acceleration structure
    bool updateOnly,               // If true, simply refit the existing
    // acceleration structure
    ID3D12Resource* previousResult // Optional previous acceleration
    // structure, used if an iterative update
    // is requested
)
{

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = m_flags;
    // The stored flags represent whether the AS has been built for updates or not. If yes and an
    // update is requested, the builder is told to only update the AS instead of fully rebuilding it
    if (flags == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE && updateOnly)
    {
        flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    }

    // Sanity checks
    if (m_flags != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE && updateOnly)
    {
        throw std::logic_error("Cannot update a bottom-level AS not originally built for updates");
    }
    if (updateOnly && previousResult == nullptr)
    {
        throw std::logic_error("Bottom-level hierarchy update requires the previous hierarchy");
    }

    if (m_resultSizeInBytes == 0 || m_scratchSizeInBytes == 0)
    {
        throw std::logic_error("Invalid scratch and result buffer sizes - ComputeASBufferSizes needs "
            "to be called before Build");
    }
    // Create a descriptor of the requested builder work, to generate a
    // bottom-level AS from the input parameters
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildDesc.Inputs.NumDescs = static_cast<UINT>(m_vertexBuffers.size());
    buildDesc.Inputs.pGeometryDescs = m_vertexBuffers.data();
    buildDesc.DestAccelerationStructureData = { resultBuffer->GetGPUVirtualAddress() };
    //                                             m_resultSizeInBytes};
    buildDesc.ScratchAccelerationStructureData = { scratchBuffer->GetGPUVirtualAddress() };
    //                                              m_scratchSizeInBytes};
    buildDesc.SourceAccelerationStructureData =
        previousResult ? previousResult->GetGPUVirtualAddress() : 0;
    buildDesc.Inputs.Flags = flags;

    // Build the AS
    commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // Wait for the builder to complete by setting a barrier on the resulting buffer. This is
    // particularly important as the construction of the top-level hierarchy may be called right
    // afterwards, before executing the command list.
    D3D12_RESOURCE_BARRIER uavBarrier;
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource = resultBuffer;
    uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    commandList->ResourceBarrier(1, &uavBarrier);
}

//--------------------------------------------------------------------------------------------------
//
// Add an instance to the top-level acceleration structure. The instance is
// represented by a bottom-level AS, a transform, an instance ID and the index
// of the hit group indicating which shaders are executed upon hitting any
// geometry within the instance
void TopLevelASGenerator::AddInstance(
    ID3D12Resource* bottomLevelAS,      // Bottom-level acceleration structure containing the
    // actual geometric data of the instance
    const DirectX::XMMATRIX& transform, // Transform matrix to apply to the instance, allowing the
    // same bottom-level AS to be used at several world-space
    // positions
    UINT instanceID,                    // Instance ID, which can be used in the shaders to
    // identify this specific instance
    UINT hitGroupIndex,                  // Hit group index, corresponding the the index of the
    // hit group in the Shader Binding Table that will be
    // invocated upon hitting the geometry
    bool bOpaque
)
{
    m_instances.emplace_back(Instance(bottomLevelAS, transform, instanceID, hitGroupIndex, bOpaque));
}

//--------------------------------------------------------------------------------------------------
//
// Compute the size of the scratch space required to build the acceleration
// structure, as well as the size of the resulting structure. The allocation of
// the buffers is then left to the application
void TopLevelASGenerator::ComputeASBufferSizes(
    ID3D12Device5* device, // Device on which the build will be performed
    bool allowUpdate,                        // If true, the resulting acceleration structure will
    // allow iterative updates
    UINT64* scratchSizeInBytes,              // Required scratch memory on the GPU to build
    // the acceleration structure
    UINT64* resultSizeInBytes,               // Required GPU memory to store the acceleration
    // structure
    UINT64* descriptorsSizeInBytes           // Required GPU memory to store instance
    // descriptors, containing the matrices,
    // indices etc.
)
{
    // The generated AS can support iterative updates. This may change the final
    // size of the AS as well as the temporary memory requirements, and hence has
    // to be set before the actual build
    m_flags = allowUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE
        : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    // Describe the work being requested, in this case the construction of a
    // (possibly dynamic) top-level hierarchy, with the given instance descriptors
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc = {};
    prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    prebuildDesc.NumDescs = static_cast<UINT>(m_instances.size());
    prebuildDesc.Flags = m_flags;

    // This structure is used to hold the sizes of the required scratch memory and
    // resulting AS
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};

    // Building the acceleration structure (AS) requires some scratch space, as
    // well as space to store the resulting structure This function computes a
    // conservative estimate of the memory requirements for both, based on the
    // number of bottom-level instances.
    device->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &info);

    // Buffer sizes need to be 256-byte-aligned
    info.ResultDataMaxSizeInBytes =
        ROUND_UP(info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
    info.ScratchDataSizeInBytes =
        ROUND_UP(info.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

    m_resultSizeInBytes = info.ResultDataMaxSizeInBytes;
    m_scratchSizeInBytes = info.ScratchDataSizeInBytes;
    // The instance descriptors are stored as-is in GPU memory, so we can deduce
    // the required size from the instance count
    m_instanceDescsSizeInBytes =
        ROUND_UP(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * static_cast<UINT64>(m_instances.size()),
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

    *scratchSizeInBytes = m_scratchSizeInBytes;
    *resultSizeInBytes = m_resultSizeInBytes;
    *descriptorsSizeInBytes = m_instanceDescsSizeInBytes;
}

//--------------------------------------------------------------------------------------------------
//
// Enqueue the construction of the acceleration structure on a command list,
// using application-provided buffers and possibly a pointer to the previous
// acceleration structure in case of iterative updates. Note that the update can
// be done in place: the result and previousResult pointers can be the same.
void TopLevelASGenerator::Generate(
    ID3D12GraphicsCommandList4* commandList, // Command list on which the build will be enqueued
    ID3D12Resource* scratchBuffer,     // Scratch buffer used by the builder to
    // store temporary data
    ID3D12Resource* resultBuffer,      // Result buffer storing the acceleration structure
    ID3D12Resource* descriptorsBuffer, // Auxiliary result buffer containing the instance
    // descriptors, has to be in upload heap
    bool updateOnly /*= false*/,       // If true, simply refit the existing
    // acceleration structure
    ID3D12Resource* previousResult /*= nullptr*/ // Optional previous acceleration
    // structure, used if an iterative update
    // is requested
)
{
    // Copy the descriptors in the target descriptor buffer
    D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
    descriptorsBuffer->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs));
    if (!instanceDescs)
    {
        throw std::logic_error("Cannot map the instance descriptor buffer - is it "
            "in the upload heap?");
    }

    auto instanceCount = static_cast<UINT>(m_instances.size());

    // Initialize the memory to zero on the first time only
    if (!updateOnly)
    {
        ZeroMemory(instanceDescs, m_instanceDescsSizeInBytes);
    }

    // Create the description for each instance
    for (uint32_t i = 0; i < instanceCount; i++)
    {
        // Instance ID visible in the shader in InstanceID()
        instanceDescs[i].InstanceID = m_instances[i].instanceID;
        // Index of the hit group invoked upon intersection
        instanceDescs[i].InstanceContributionToHitGroupIndex = m_instances[i].hitGroupIndex;
        // Instance flags, including backface culling, winding, etc - TODO: should
        // be accessible from outside
        instanceDescs[i].Flags = m_instances[i].bOpaque ? D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE : D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;
        // Instance transform matrix
        DirectX::XMMATRIX m = XMMatrixTranspose(
            m_instances[i].transform); // GLM is column major, the INSTANCE_DESC is row major
        memcpy(instanceDescs[i].Transform, &m, sizeof(instanceDescs[i].Transform));
        // Get access to the bottom level
        instanceDescs[i].AccelerationStructure = m_instances[i].bottomLevelAS->GetGPUVirtualAddress();
        // Visibility mask, always visible here - TODO: should be accessible from
        // outside
        instanceDescs[i].InstanceMask = 0xFF;
    }

    descriptorsBuffer->Unmap(0, nullptr);

    // If this in an update operation we need to provide the source buffer
    D3D12_GPU_VIRTUAL_ADDRESS pSourceAS = updateOnly ? previousResult->GetGPUVirtualAddress() : 0;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = m_flags;
    // The stored flags represent whether the AS has been built for updates or
    // not. If yes and an update is requested, the builder is told to only update
    // the AS instead of fully rebuilding it
    if (flags == D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE && updateOnly)
    {
        flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    }

    // Sanity checks
    if (m_flags != D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE && updateOnly)
    {
        throw std::logic_error("Cannot update a top-level AS not originally built for updates");
    }
    if (updateOnly && previousResult == nullptr)
    {
        throw std::logic_error("Top-level hierarchy update requires the previous hierarchy");
    }

    // Create a descriptor of the requested builder work, to generate a top-level
    // AS from the input parameters
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildDesc.Inputs.InstanceDescs = descriptorsBuffer->GetGPUVirtualAddress();
    buildDesc.Inputs.NumDescs = instanceCount;
    buildDesc.DestAccelerationStructureData = { resultBuffer->GetGPUVirtualAddress() };
    buildDesc.ScratchAccelerationStructureData = { scratchBuffer->GetGPUVirtualAddress() };
    buildDesc.SourceAccelerationStructureData = pSourceAS;
    buildDesc.Inputs.Flags = flags;

    // Build the top-level AS
    commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // Wait for the builder to complete by setting a barrier on the resulting
    // buffer. This can be important in case the rendering is triggered
    // immediately afterwards, without executing the command list
    D3D12_RESOURCE_BARRIER uavBarrier;
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource = resultBuffer;
    uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    commandList->ResourceBarrier(1, &uavBarrier);
}

//--------------------------------------------------------------------------------------------------
//
//
TopLevelASGenerator::Instance::Instance(ID3D12Resource* blAS, const DirectX::XMMATRIX& tr, UINT iID,
    UINT hgId, bool bOpaque)
    : bottomLevelAS(blAS), transform(tr), instanceID(iID), hitGroupIndex(hgId), bOpaque(bOpaque)
{
}

