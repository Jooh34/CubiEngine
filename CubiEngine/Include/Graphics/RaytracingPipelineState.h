#pragma once

#include "Graphics/Resource.h"

typedef std::tuple<UINT, /* BaseShaderRegister, */ UINT, /* NumDescriptors */ UINT,
    /* RegisterSpace */ D3D12_DESCRIPTOR_RANGE_TYPE,
    /* RangeType */ UINT /* OffsetInDescriptorsFromTableStart */> HeapRangeTupleType;

enum RTRootParams : uint32_t
{
    //RTParams_StandardDescriptors,
    RTParams_SceneDescriptor,
    //RTParams_UAVDescriptor,
    RTParams_CBuffer,
    //RTParams_LightCBuffer,
    //RTParams_AppSettings,

    NumRTRootParams
};

class FRaytracingPipelineState
{
public:
    FRaytracingPipelineState();
    FRaytracingPipelineState(ID3D12Device5* const device, const FRaytracingPipelineStateCreationDesc& pipelineStateCreationDesc);
    
    // The shader path passed in needs to be relative (with respect to root directory), it will internally find the
    // complete path (with respect to the executable).
    static void CreateRaytracingRootSignature(ID3D12Device* const device);

    static void CreateGlobalRootSignature(ID3D12Device* const device);

    // Raytracing
    static inline ComPtr<ID3D12RootSignature> StaticGlobalRootSignature{};
    
    ComPtr<ID3D12StateObjectProperties>& GetRTStateObjectProps() { return RTStateObjectProps; }
    ComPtr<ID3D12StateObject>& GetRTStateObject() { return RTStateObject; }
    ID3D12StateObject* GetRTStateObjectPtr() const { return RTStateObject.Get(); }

private:
    ComPtr<ID3D12StateObject> RTStateObject{};
    // Ray tracing pipeline state properties, retaining the shader identifiers
    // to use in the Shader Binding Table
    ComPtr<ID3D12StateObjectProperties> RTStateObjectProps{};
};

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

#include "d3d12.h"
#include "dxcapi.h"

#include <string>
#include <vector>

/// Helper class to create raytracing pipelines
class RayTracingPipelineGenerator
{
public:
    /// The pipeline helper requires access to the device.
    RayTracingPipelineGenerator(ID3D12Device5* device);

    /// Add a DXIL library to the pipeline. Note that this library has to be
    /// compiled with dxc, using a lib_6_3 target. The exported symbols must correspond exactly to the
    /// names of the shaders declared in the library, although unused ones can be omitted.
    void AddLibrary(IDxcBlob* dxilLibrary, const std::vector<std::wstring>& symbolExports);

    /// In DXR the hit-related shaders are grouped into hit groups. Such shaders are:
    /// - The intersection shader, which can be used to intersect custom geometry, and is called upon
    ///   hitting the bounding box the the object. A default one exists to intersect triangles
    /// - The any hit shader, called on each intersection, which can be used to perform early
    ///   alpha-testing and allow the ray to continue if needed. Default is a pass-through.
    /// - The closest hit shader, invoked on the hit point closest to the ray start.
    /// The shaders in a hit group share the same root signature, and are only referred to by the
    /// hit group name in other places of the program.
    void AddHitGroup(const std::wstring& hitGroupName, const std::wstring& closestHitSymbol,
        const std::wstring& anyHitSymbol = L"",
        const std::wstring& intersectionSymbol = L"");

    /// The payload is the way hit or miss shaders can exchange data with the shader that called
    /// TraceRay. When several ray types are used (e.g. primary and shadow rays), this value must be
    /// the largest possible payload size. Note that to optimize performance, this size must be kept
    /// as low as possible.
    void SetMaxPayloadSize(UINT sizeInBytes);

    /// When hitting geometry, a number of surface attributes can be generated by the intersector.
    /// Using the built-in triangle intersector the attributes are the barycentric coordinates, with a
    /// size 2*sizeof(float).
    void SetMaxAttributeSize(UINT sizeInBytes);

    /// Upon hitting a surface, a closest hit shader can issue a new TraceRay call. This parameter
    /// indicates the maximum level of recursion. Note that this depth should be kept as low as
    /// possible, typically 2, to allow hit shaders to trace shadow rays. Recursive ray tracing
    /// algorithms must be flattened to a loop in the ray generation program for best performance.
    void SetMaxRecursionDepth(UINT maxDepth);

    // jooh
    void SetGlobalRootSignature(ID3D12RootSignature* InGlobalRootSignature)
    {
        GlobalRootSignature = InGlobalRootSignature;
    }

    /// Compiles the raytracing state object
    void Generate(ComPtr<ID3D12StateObject>& rtStateObject);

private:
    /// Storage for DXIL libraries and their exported symbols
    struct Library
    {
        Library(IDxcBlob* dxil, const std::vector<std::wstring>& exportedSymbols);

        Library(const Library& source);

        IDxcBlob* m_dxil;
        const std::vector<std::wstring> m_exportedSymbols;

        std::vector<D3D12_EXPORT_DESC> m_exports;
        D3D12_DXIL_LIBRARY_DESC m_libDesc;
    };

    /// Storage for the hit groups, binding the hit group name with the underlying intersection, any
    /// hit and closest hit symbols
    struct HitGroup
    {
        HitGroup(std::wstring hitGroupName, std::wstring closestHitSymbol,
            std::wstring anyHitSymbol = L"", std::wstring intersectionSymbol = L"");

        HitGroup(const HitGroup& source);

        std::wstring m_hitGroupName;
        std::wstring m_closestHitSymbol;
        std::wstring m_anyHitSymbol;
        std::wstring m_intersectionSymbol;
        D3D12_HIT_GROUP_DESC m_desc = {};
    };

    std::vector<Library> m_libraries = {};
    std::vector<HitGroup> m_hitGroups = {};

    ID3D12RootSignature* GlobalRootSignature;

    UINT m_maxPayLoadSizeInBytes = 0;
    /// Attribute size, initialized to 2 for the barycentric coordinates used by the built-in triangle
    /// intersection shader
    UINT m_maxAttributeSizeInBytes = 2 * sizeof(float);
    /// Maximum recursion depth, initialized to 1 to at least allow tracing primary rays
    UINT m_maxRecursionDepth = 1;

    ID3D12Device5* m_device;
    ID3D12RootSignature* m_dummyLocalRootSignature;
};