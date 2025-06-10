#include "Graphics/RaytracingPipelineState.h"
#include "Graphics/ShaderCompiler.h"
#include "Core/FileSystem.h"
#include "Graphics/Dx12Helper.h"

FRaytracingPipelineState::FRaytracingPipelineState(ID3D12Device5* const device, const FRaytracingPipelineStateCreationDesc& pipelineStateCreationDesc)
{
    const auto& ShaderRGS =
        ShaderCompiler::Compile(ShaderTypes::Raytracing,
            FFileSystem::GetFullPath(pipelineStateCreationDesc.rtShaderPath),
            pipelineStateCreationDesc.EntryPointRGS).shaderBlob;

    const auto& ShaderCHS =
        ShaderCompiler::Compile(ShaderTypes::Raytracing,
            FFileSystem::GetFullPath(pipelineStateCreationDesc.rtShaderPath),
            pipelineStateCreationDesc.EntryPointCHS).shaderBlob;

    const auto& ShaderRMS =
        ShaderCompiler::Compile(ShaderTypes::Raytracing,
            FFileSystem::GetFullPath(pipelineStateCreationDesc.rtShaderPath),
            pipelineStateCreationDesc.EntryPointRMS).shaderBlob;

    const auto& ShaderShadowCHS =
        ShaderCompiler::Compile(ShaderTypes::Raytracing,
            FFileSystem::GetFullPath(L"Shaders/Raytracing/Shadow.hlsl"),
            L"ShadowClosestHit").shaderBlob;

    const auto& ShaderShadowRMS =
        ShaderCompiler::Compile(ShaderTypes::Raytracing,
            FFileSystem::GetFullPath(L"Shaders/Raytracing/Shadow.hlsl"),
            L"ShadowMiss").shaderBlob;

    RayTracingPipelineGenerator PipelineGenerator(device);
    PipelineGenerator.AddLibrary(ShaderRGS.Get(), { std::wstring(pipelineStateCreationDesc.EntryPointRGS) });
    PipelineGenerator.AddLibrary(ShaderCHS.Get(), { std::wstring(pipelineStateCreationDesc.EntryPointCHS) });
    PipelineGenerator.AddLibrary(ShaderRMS.Get(), { std::wstring(pipelineStateCreationDesc.EntryPointRMS) });
    PipelineGenerator.AddLibrary(ShaderShadowCHS.Get(), { L"ShadowClosestHit" });
    PipelineGenerator.AddLibrary(ShaderShadowRMS.Get(), { L"ShadowMiss" });

    PipelineGenerator.AddHitGroup(L"HitGroup", std::wstring(pipelineStateCreationDesc.EntryPointCHS));
    PipelineGenerator.AddHitGroup(L"ShadowHitGroup", L"ShadowClosestHit");

    // Todo:
    PipelineGenerator.SetMaxPayloadSize(8 * sizeof(float)); // RGB + distance + path depth
    PipelineGenerator.SetMaxAttributeSize(2 * sizeof(float)); // barycentric coordinates
    PipelineGenerator.SetMaxRecursionDepth(10);

    PipelineGenerator.SetGlobalRootSignature(StaticGlobalRootSignature.Get());

    PipelineGenerator.Generate(RTStateObject);
    ThrowIfFailed(
        RTStateObject->QueryInterface(IID_PPV_ARGS(&RTStateObjectProps)));
}

void FRaytracingPipelineState::CreateRaytracingRootSignature(ID3D12Device* const device)
{
    // Raytracing shaders does not support root signature extraction. so manually create the root signature.
    // but thanks to bindless, we use a static root signature for all raytracing shaders.
    CreateGlobalRootSignature(device);
}

void FRaytracingPipelineState::CreateGlobalRootSignature(ID3D12Device* const device)
{
    // RayTrace root signature
    D3D12_DESCRIPTOR_RANGE1 uavRanges[1] = {};
    uavRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRanges[0].NumDescriptors = 1;
    uavRanges[0].BaseShaderRegister = 0;
    uavRanges[0].RegisterSpace = 0;
    uavRanges[0].OffsetInDescriptorsFromTableStart = 0;

    D3D12_ROOT_PARAMETER1 rootParameters[NumRTRootParams] = {};

    // Standard SRV descriptors
    //rootParameters[RTParams_StandardDescriptors].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    //rootParameters[RTParams_StandardDescriptors].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    //rootParameters[RTParams_StandardDescriptors].DescriptorTable.pDescriptorRanges = DX12::GlobalSRVDescriptorRanges();
    //rootParameters[RTParams_StandardDescriptors].DescriptorTable.NumDescriptorRanges = DX12::NumGlobalSRVDescriptorRanges;

    // Acceleration structure SRV descriptor
    rootParameters[RTParams_SceneDescriptor].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    rootParameters[RTParams_SceneDescriptor].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[RTParams_SceneDescriptor].Descriptor.ShaderRegister = 0;
    rootParameters[RTParams_SceneDescriptor].Descriptor.RegisterSpace = 200;

    // UAV descriptor
    //rootParameters[RTParams_UAVDescriptor].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    //rootParameters[RTParams_UAVDescriptor].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    //rootParameters[RTParams_UAVDescriptor].DescriptorTable.pDescriptorRanges = uavRanges;
    //rootParameters[RTParams_UAVDescriptor].DescriptorTable.NumDescriptorRanges = ArraySize_(uavRanges);

    // RootConstants Buffer
    rootParameters[RTParams_CBuffer].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[RTParams_CBuffer].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[RTParams_CBuffer].Constants.Num32BitValues = 48;
    rootParameters[RTParams_CBuffer].Descriptor.RegisterSpace = 0;
    rootParameters[RTParams_CBuffer].Descriptor.ShaderRegister = 0;

    // LightCBuffer
    //rootParameters[RTParams_LightCBuffer].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    //rootParameters[RTParams_LightCBuffer].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    //rootParameters[RTParams_LightCBuffer].Descriptor.RegisterSpace = 0;
    //rootParameters[RTParams_LightCBuffer].Descriptor.ShaderRegister = 1;
    //rootParameters[RTParams_LightCBuffer].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

    // AppSettings
    //rootParameters[RTParams_AppSettings].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    //rootParameters[RTParams_AppSettings].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    //rootParameters[RTParams_AppSettings].Descriptor.RegisterSpace = 0;
    //rootParameters[RTParams_AppSettings].Descriptor.ShaderRegister = AppSettings::CBufferRegister;
    //rootParameters[RTParams_AppSettings].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;

    D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
    staticSamplers[0] = GetStaticSamplerDesc(SamplerState::Linear, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
    staticSamplers[1] = GetStaticSamplerDesc(SamplerState::LinearClamp, 1, 0, D3D12_SHADER_VISIBILITY_ALL);

    D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = ArraySize_(rootParameters);
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumStaticSamplers = ArraySize_(staticSamplers);
    rootSignatureDesc.pStaticSamplers = staticSamplers;
    rootSignatureDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

    // Create the root signature from its descriptor
    ID3DBlob* pSigBlob;
    ID3DBlob* pErrorBlob;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc = { };
    versionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    versionedDesc.Desc_1_1 = rootSignatureDesc;

    HRESULT hr = D3D12SerializeVersionedRootSignature(&versionedDesc, &pSigBlob, &pErrorBlob);

    if (FAILED(hr))
    {
        throw std::logic_error("Cannot serialize root signature");
    }

    hr = device->CreateRootSignature(0, pSigBlob->GetBufferPointer(), pSigBlob->GetBufferSize(),
        IID_PPV_ARGS(&StaticGlobalRootSignature));
    if (FAILED(hr))
    {
        throw std::logic_error("Cannot create root signature");
    }

    StaticGlobalRootSignature->SetName(L"GlobalRootSignature");
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

#include "dxcapi.h"
#include <unordered_set>
#include <stdexcept>

//--------------------------------------------------------------------------------------------------
// The pipeline helper requires access to the device.
RayTracingPipelineGenerator::RayTracingPipelineGenerator(ID3D12Device5* device)
    : m_device(device)
{
}

//--------------------------------------------------------------------------------------------------
//
// Add a DXIL library to the pipeline. Note that this library has to be
// compiled with dxc, using a lib_6_3 target. The exported symbols must correspond exactly to the
// names of the shaders declared in the library, although unused ones can be omitted.
void RayTracingPipelineGenerator::AddLibrary(IDxcBlob* dxilLibrary,
    const std::vector<std::wstring>& symbolExports)
{
    m_libraries.emplace_back(Library(dxilLibrary, symbolExports));
}

//--------------------------------------------------------------------------------------------------
//
// In DXR the hit-related shaders are grouped into hit groups. Such shaders are:
// - The intersection shader, which can be used to intersect custom geometry, and is called upon
//   hitting the bounding box the the object. A default one exists to intersect triangles
// - The any hit shader, called on each intersection, which can be used to perform early
//   alpha-testing and allow the ray to continue if needed. Default is a pass-through.
// - The closest hit shader, invoked on the hit point closest to the ray start.
// The shaders in a hit group share the same root signature, and are only referred to by the
// hit group name in other places of the program.
void RayTracingPipelineGenerator::AddHitGroup(const std::wstring& hitGroupName,
    const std::wstring& closestHitSymbol,
    const std::wstring& anyHitSymbol /*= L""*/,
    const std::wstring& intersectionSymbol /*= L""*/)
{
    m_hitGroups.emplace_back(
        HitGroup(hitGroupName, closestHitSymbol, anyHitSymbol, intersectionSymbol));
}

//--------------------------------------------------------------------------------------------------
//
// The payload is the way hit or miss shaders can exchange data with the shader that called
// TraceRay. When several ray types are used (e.g. primary and shadow rays), this value must be
// the largest possible payload size. Note that to optimize performance, this size must be kept
// as low as possible.
void RayTracingPipelineGenerator::SetMaxPayloadSize(UINT sizeInBytes)
{
    m_maxPayLoadSizeInBytes = sizeInBytes;
}

//--------------------------------------------------------------------------------------------------
//
// When hitting geometry, a number of surface attributes can be generated by the intersector.
// Using the built-in triangle intersector the attributes are the barycentric coordinates, with a
// size 2*sizeof(float).
void RayTracingPipelineGenerator::SetMaxAttributeSize(UINT sizeInBytes)
{
    m_maxAttributeSizeInBytes = sizeInBytes;
}

//--------------------------------------------------------------------------------------------------
//
// Upon hitting a surface, a closest hit shader can issue a new TraceRay call. This parameter
// indicates the maximum level of recursion. Note that this depth should be kept as low as
// possible, typically 2, to allow hit shaders to trace shadow rays. Recursive ray tracing
// algorithms must be flattened to a loop in the ray generation program for best performance.
void RayTracingPipelineGenerator::SetMaxRecursionDepth(UINT maxDepth)
{
    m_maxRecursionDepth = maxDepth;
}

//--------------------------------------------------------------------------------------------------
//
// Compiles the raytracing state object
void RayTracingPipelineGenerator::Generate(ComPtr<ID3D12StateObject>& rtStateObject)
{
    // The pipeline is made of a set of sub-objects, representing the DXIL libraries, hit group
    // declarations, root signature associations, plus some configuration objects
    UINT64 subobjectCount =
        m_libraries.size() +                     // DXIL libraries
        m_hitGroups.size() +                     // Hit group declarations
        1 +                                      // Shader configuration
        1 +                                      // Empty global signatures
        1;                                       // Final pipeline subobject

    // Initialize a vector with the target object count. It is necessary to make the allocation before
    // adding subobjects as some subobjects reference other subobjects by pointer. Using push_back may
    // reallocate the array and invalidate those pointers.
    std::vector<D3D12_STATE_SUBOBJECT> subobjects(subobjectCount);

    UINT currentIndex = 0;

    // Add all the DXIL libraries
    for (const Library& lib : m_libraries)
    {
        D3D12_STATE_SUBOBJECT libSubobject = {};
        libSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        libSubobject.pDesc = &lib.m_libDesc;

        subobjects[currentIndex++] = libSubobject;
    }

    // Add all the hit group declarations
    for (const HitGroup& group : m_hitGroups)
    {
        D3D12_STATE_SUBOBJECT hitGroup = {};
        hitGroup.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        hitGroup.pDesc = &group.m_desc;

        subobjects[currentIndex++] = hitGroup;
    }

    // Add a subobject for the shader payload configuration
    D3D12_RAYTRACING_SHADER_CONFIG shaderDesc = {};
    shaderDesc.MaxPayloadSizeInBytes = m_maxPayLoadSizeInBytes;
    shaderDesc.MaxAttributeSizeInBytes = m_maxAttributeSizeInBytes;

    D3D12_STATE_SUBOBJECT shaderConfigObject = {};
    shaderConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    shaderConfigObject.pDesc = &shaderDesc;

    subobjects[currentIndex++] = shaderConfigObject;
    
#if 0
    // The pipeline construction always requires an empty global root signature
    D3D12_STATE_SUBOBJECT globalRootSig;
    globalRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    ID3D12RootSignature* dgSig = m_dummyGlobalRootSignature;
    globalRootSig.pDesc = &dgSig;
#else
    // use global RS
    D3D12_STATE_SUBOBJECT globalRootSig;
    globalRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    globalRootSig.pDesc = &GlobalRootSignature;
#endif
    subobjects[currentIndex++] = globalRootSig;

    // Add a subobject for the ray tracing pipeline configuration
    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
    pipelineConfig.MaxTraceRecursionDepth = m_maxRecursionDepth;

    D3D12_STATE_SUBOBJECT pipelineConfigObject = {};
    pipelineConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    pipelineConfigObject.pDesc = &pipelineConfig;

    subobjects[currentIndex++] = pipelineConfigObject;

    // Describe the ray tracing pipeline state object
    D3D12_STATE_OBJECT_DESC pipelineDesc = {};
    pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    pipelineDesc.NumSubobjects = static_cast<UINT>(subobjects.size());
    pipelineDesc.pSubobjects = subobjects.data();

    // Create the state object
    HRESULT hr = m_device->CreateStateObject(&pipelineDesc, IID_PPV_ARGS(&rtStateObject));
    if (FAILED(hr))
    {
        throw std::logic_error("Could not create the raytracing state object");
    }
}

//--------------------------------------------------------------------------------------------------
//
// Store data related to a DXIL library: the library itself, the exported symbols, and the
// associated descriptors
RayTracingPipelineGenerator::Library::Library(IDxcBlob* dxil,
    const std::vector<std::wstring>& exportedSymbols)
    : m_dxil(dxil), m_exportedSymbols(exportedSymbols), m_exports(exportedSymbols.size())
{
    // Create one export descriptor per symbol
    for (size_t i = 0; i < m_exportedSymbols.size(); i++)
    {
        m_exports[i] = {};
        m_exports[i].Name = m_exportedSymbols[i].c_str();
        m_exports[i].ExportToRename = nullptr;
        m_exports[i].Flags = D3D12_EXPORT_FLAG_NONE;
    }

    // Create a library descriptor combining the DXIL code and the export names
    m_libDesc.DXILLibrary.BytecodeLength = dxil->GetBufferSize();
    m_libDesc.DXILLibrary.pShaderBytecode = dxil->GetBufferPointer();
    m_libDesc.NumExports = static_cast<UINT>(m_exportedSymbols.size());
    m_libDesc.pExports = m_exports.data();
}

//--------------------------------------------------------------------------------------------------
//
// This copy constructor has to be defined so that the export descriptors are set correctly. Using
// the default constructor would copy the string pointers of the symbols into the descriptors, which
// would cause issues when the original Library object gets out of scope
RayTracingPipelineGenerator::Library::Library(const Library& source)
    : Library(source.m_dxil, source.m_exportedSymbols)
{
}

//--------------------------------------------------------------------------------------------------
//
// Create a hit group descriptor from the input hit group name and shader symbols
RayTracingPipelineGenerator::HitGroup::HitGroup(std::wstring hitGroupName,
    std::wstring closestHitSymbol,
    std::wstring anyHitSymbol /*= L""*/,
    std::wstring intersectionSymbol /*= L""*/)
    : m_hitGroupName(std::move(hitGroupName)), m_closestHitSymbol(std::move(closestHitSymbol)),
    m_anyHitSymbol(std::move(anyHitSymbol)), m_intersectionSymbol(std::move(intersectionSymbol))
{
    // Indicate which shader program is used for closest hit, leave the other
    // ones undefined (default behavior), export the name of the group
    m_desc.HitGroupExport = m_hitGroupName.c_str();
    m_desc.ClosestHitShaderImport = m_closestHitSymbol.empty() ? nullptr : m_closestHitSymbol.c_str();
    m_desc.AnyHitShaderImport = m_anyHitSymbol.empty() ? nullptr : m_anyHitSymbol.c_str();
    m_desc.IntersectionShaderImport =
        m_intersectionSymbol.empty() ? nullptr : m_intersectionSymbol.c_str();
}

//--------------------------------------------------------------------------------------------------
//
// This copy constructor has to be defined so that the export descriptors are set correctly. Using
// the default constructor would copy the string pointers of the symbols into the descriptors, which
// would cause issues when the original HitGroup object gets out of scope
RayTracingPipelineGenerator::HitGroup::HitGroup(const HitGroup& source)
    : HitGroup(source.m_hitGroupName, source.m_closestHitSymbol, source.m_anyHitSymbol,
        source.m_intersectionSymbol)
{
}