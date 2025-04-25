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

#include <vector>
#include <string>

class ShaderBindingTableGenerator
{
public:
    /// Add a ray generation program by name, with its list of data pointers or values according to
    /// the layout of its root signature
    void AddRayGenerationProgram(const std::wstring& entryPoint, const std::vector<void*>& inputData);

    /// Add a miss program by name, with its list of data pointers or values according to
    /// the layout of its root signature
    void AddMissProgram(const std::wstring& entryPoint, const std::vector<void*>& inputData);

    /// Add a hit group by name, with its list of data pointers or values according to
    /// the layout of its root signature
    void AddHitGroup(const std::wstring& entryPoint, const std::vector<void*>& inputData);

    /// Compute the size of the SBT based on the set of programs and hit groups it contains
    uint32_t ComputeSBTSize();

    /// Build the SBT and store it into sbtBuffer, which has to be pre-allocated on the upload heap.
    /// Access to the raytracing pipeline object is required to fetch program identifiers using their
    /// names
    void Generate(ID3D12Resource* sbtBuffer,
        ID3D12StateObjectProperties* raytracingPipeline);

    /// Reset the sets of programs and hit groups
    void Reset();

    /// The following getters are used to simplify the call to DispatchRays where the offsets of the
    /// shader programs must be exactly following the SBT layout

    /// Get the size in bytes of the SBT section dedicated to ray generation programs
    UINT GetRayGenSectionSize() const;
    /// Get the size in bytes of one ray generation program entry in the SBT
    UINT GetRayGenEntrySize() const;

    /// Get the size in bytes of the SBT section dedicated to miss programs
    UINT GetMissSectionSize() const;
    /// Get the size in bytes of one miss program entry in the SBT
    UINT GetMissEntrySize();

    /// Get the size in bytes of the SBT section dedicated to hit groups
    UINT GetHitGroupSectionSize() const;
    /// Get the size in bytes of hit group entry in the SBT
    UINT GetHitGroupEntrySize() const;

private:
    /// Wrapper for SBT entries, each consisting of the name of the program and a list of values,
    /// which can be either pointers or raw 32-bit constants
    struct SBTEntry
    {
        SBTEntry(std::wstring entryPoint, std::vector<void*> inputData);

        const std::wstring m_entryPoint;
        const std::vector<void*> m_inputData;
    };

    /// For each entry, copy the shader identifier followed by its resource pointers and/or root
    /// constants in outputData, with a stride in bytes of entrySize, and returns the size in bytes
    /// actually written to outputData.
    uint32_t CopyShaderData(ID3D12StateObjectProperties* raytracingPipeline,
        uint8_t* outputData, const std::vector<SBTEntry>& shaders,
        uint32_t entrySize);

    /// Compute the size of the SBT entries for a set of entries, which is determined by the maximum
    /// number of parameters of their root signature
    uint32_t GetEntrySize(const std::vector<SBTEntry>& entries);

    std::vector<SBTEntry> m_rayGen;
    std::vector<SBTEntry> m_miss;
    std::vector<SBTEntry> m_hitGroup;

    /// For each category, the size of an entry in the SBT depends on the maximum number of resources
    /// used by the shaders in that category.The helper computes those values automatically in
    /// GetEntrySize()
    uint32_t m_rayGenEntrySize;
    uint32_t m_missEntrySize;
    uint32_t m_hitGroupEntrySize;
};