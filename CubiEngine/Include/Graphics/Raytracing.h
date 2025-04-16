#pragma once

class FGraphicsDevice;
class FGraphicsContext;

typedef std::pair<ID3D12Resource*, Dx::XMMATRIX> BLASMatrixPairType;

class FRaytracingGeometry
{
public:
    FRaytracingGeometry() = delete;
    FRaytracingGeometry(const FGraphicsDevice* const GraphicsDevice,
        std::pair<ComPtr<ID3D12Resource>, uint32_t>& VertexBuffer,
        std::pair<ComPtr<ID3D12Resource>, uint32_t>& IndexBuffer
    );

    ID3D12Resource* GetBLAS() { return result.Get(); }

private:
    ComPtr<ID3D12Resource> scratch;
    ComPtr<ID3D12Resource> result;
};

class FRaytracingScene
{
public:
    void GenerateRaytracingScene(
        const FGraphicsDevice* const GraphicsDevice,
        FGraphicsContext* const GraphicsContext,
        std::vector<BLASMatrixPairType>& instances
    );
    
    ComPtr<ID3D12Resource> pScratch;
    ComPtr<ID3D12Resource> pResult;
    ComPtr<ID3D12Resource> pInstanceDesc;
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

class BLASGenerator
{
public:
    void AddVertexBuffer(ID3D12Resource* vertexBuffer, /// Buffer containing the vertex coordinates,
        /// possibly interleaved with other vertex data
        UINT64 vertexOffsetInBytes,   /// Offset of the first vertex in the vertex
        /// buffer
        uint32_t vertexCount,         /// Number of vertices to consider
        /// in the buffer
        UINT vertexSizeInBytes,       /// Size of a vertex including
        /// all its other data,
        /// used to stride in the buffer
        ID3D12Resource* indexBuffer,  /// Buffer containing the vertex indices
        /// describing the triangles
        UINT64 indexOffsetInBytes,    /// Offset of the first index in
        /// the index buffer
        uint32_t indexCount,          /// Number of indices to consider in the buffer
        ID3D12Resource* transformBuffer, /// Buffer containing a 4x4 transform
        /// matrix in GPU memory, to be applied
        /// to the vertices. This buffer cannot
        /// be nullptr
        UINT64 transformOffsetInBytes,   /// Offset of the transform matrix in the
        /// transform buffer
        bool isOpaque = true /// If true, the geometry is considered opaque,
        /// optimizing the search for a closest hit
    );

    /// Compute the size of the scratch space required to build the acceleration structure, as well as
  /// the size of the resulting structure. The allocation of the buffers is then left to the
  /// application
    void ComputeASBufferSizes(
        ID3D12Device5* device, /// Device on which the build will be performed
        bool allowUpdate,           /// If true, the resulting acceleration structure will
        /// allow iterative updates
        UINT64* scratchSizeInBytes, /// Required scratch memory on the GPU to
        /// build the acceleration structure
        UINT64* resultSizeInBytes   /// Required GPU memory to store the
        /// acceleration structure
    );

    void Generate(
        ID3D12GraphicsCommandList4* commandList, /// Command list on which the build will be enqueued
        ID3D12Resource* scratchBuffer, /// Scratch buffer used by the builder to
        /// store temporary data
        ID3D12Resource* resultBuffer,  /// Result buffer storing the acceleration structure
        bool updateOnly = false,       /// If true, simply refit the existing acceleration structure
        ID3D12Resource* previousResult = nullptr /// Optional previous acceleration structure, used
        /// if an iterative update is requested
    );

private:
    /// Vertex buffer descriptors used to generate the AS
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_vertexBuffers = {};

    /// Amount of temporary memory required by the builder
    UINT64 m_scratchSizeInBytes = 0;

    /// Amount of memory required to store the AS
    UINT64 m_resultSizeInBytes = 0;

    /// Flags for the builder, specifying whether to allow iterative updates, or
    /// when to perform an update
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS m_flags;
};

/// Helper class to generate top-level acceleration structures for raytracing
class TopLevelASGenerator
{
public:
    /// Add an instance to the top-level acceleration structure. The instance is
    /// represented by a bottom-level AS, a transform, an instance ID and the
    /// index of the hit group indicating which shaders are executed upon hitting
    /// any geometry within the instance
    void
        AddInstance(ID3D12Resource* bottomLevelAS, /// Bottom-level acceleration structure containing the
            /// actual geometric data of the instance
            const DirectX::XMMATRIX& transform, /// Transform matrix to apply to the instance,
            /// allowing the same bottom-level AS to be used
            /// at several world-space positions
            UINT instanceID,   /// Instance ID, which can be used in the shaders to
            /// identify this specific instance
            UINT hitGroupIndex /// Hit group index, corresponding the the index of the
            /// hit group in the Shader Binding Table that will be
            /// invocated upon hitting the geometry
        );

    /// Compute the size of the scratch space required to build the acceleration
    /// structure, as well as the size of the resulting structure. The allocation
    /// of the buffers is then left to the application
    void ComputeASBufferSizes(
        ID3D12Device5* device, /// Device on which the build will be performed
        bool allowUpdate,              /// If true, the resulting acceleration structure will
        /// allow iterative updates
        UINT64* scratchSizeInBytes,    /// Required scratch memory on the GPU to
        /// build the acceleration structure
        UINT64* resultSizeInBytes,     /// Required GPU memory to store the
        /// acceleration structure
        UINT64* descriptorsSizeInBytes /// Required GPU memory to store instance
        /// descriptors, containing the matrices,
        /// indices etc.
    );

    /// Enqueue the construction of the acceleration structure on a command list,
    /// using application-provided buffers and possibly a pointer to the previous
    /// acceleration structure in case of iterative updates. Note that the update
    /// can be done in place: the result and previousResult pointers can be the
    /// same.
    void Generate(
        ID3D12GraphicsCommandList4* commandList, /// Command list on which the build will be enqueued
        ID3D12Resource* scratchBuffer,     /// Scratch buffer used by the builder to
        /// store temporary data
        ID3D12Resource* resultBuffer,      /// Result buffer storing the acceleration structure
        ID3D12Resource* descriptorsBuffer, /// Auxiliary result buffer containing the instance
        /// descriptors, has to be in upload heap
        bool updateOnly = false, /// If true, simply refit the existing acceleration structure
        ID3D12Resource* previousResult = nullptr /// Optional previous acceleration structure, used
        /// if an iterative update is requested
    );

private:
    /// Helper struct storing the instance data
    struct Instance
    {
        Instance(ID3D12Resource* blAS, const DirectX::XMMATRIX& tr, UINT iID, UINT hgId);
        /// Bottom-level AS
        ID3D12Resource* bottomLevelAS;
        /// Transform matrix
        const DirectX::XMMATRIX& transform;
        /// Instance ID visible in the shader
        UINT instanceID;
        /// Hit group index used to fetch the shaders from the SBT
        UINT hitGroupIndex;
    };

    /// Construction flags, indicating whether the AS supports iterative updates
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS m_flags;
    /// Instances contained in the top-level AS
    std::vector<Instance> m_instances;

    /// Size of the temporary memory used by the TLAS builder
    UINT64 m_scratchSizeInBytes;
    /// Size of the buffer containing the instance descriptors
    UINT64 m_instanceDescsSizeInBytes;
    /// Size of the buffer containing the TLAS
    UINT64 m_resultSizeInBytes;
};