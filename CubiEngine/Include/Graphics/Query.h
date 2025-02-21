#pragma once

class FGraphicsDevice;
class FQueryHeap;

struct FQueryLocation
{
    // The heap in which the result is contained.
    class FQueryHeap* Heap = nullptr;

    // The index of the query within the heap.
    uint32_t Index = 0;

    D3D12_QUERY_TYPE Type = D3D12_QUERY_TYPE::D3D12_QUERY_TYPE_TIMESTAMP;

    FQueryLocation() = default;
    FQueryLocation(FQueryHeap* Heap, uint32_t Index, D3D12_QUERY_TYPE Type)
        : Heap(Heap)
        , Index(Index)
        , Type(Type)
    {}

    operator bool() const { return Heap != nullptr; }
};
class FQueryHeap
{
    // All query heaps are allocated to fill a single 64KB page
    static constexpr uint32_t MaxHeapSize = 65536;

public:
    FQueryHeap(FGraphicsDevice* Device, D3D12_QUERY_TYPE QueryType, D3D12_QUERY_HEAP_TYPE HeapType);

    // The byte size of a result for a single query
    uint32_t GetResultSize() const
    {
        switch (QueryType)
        {
            case D3D12_QUERY_TYPE_TIMESTAMP:
            case D3D12_QUERY_TYPE_OCCLUSION:
                return sizeof(UINT64);

            case D3D12_QUERY_TYPE_PIPELINE_STATISTICS:
                return sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);

            default:
                return sizeof(UINT64);
        }
    }
    
    void Maintain()
    {
        // it's ok because we allocated maximum NumQueries
        if (CurrentQueryIndex > 0.8f * NumQueries)
        {
            CurrentQueryIndex = 0;
        }
    }

    uint32_t AllocateQuery()
    {
        uint32_t Ret = CurrentQueryIndex;
        CurrentQueryIndex++;
        return Ret;
    }

    ID3D12QueryHeap* GetD3D12QueryHeap() { return QueryHeap.Get(); }
    ID3D12Resource* GetQueryReadbackBuffer() { return QueryReadbackBuffer.Get(); }
    void MapReadbackBuffer(void** Data) {
        QueryReadbackBuffer->Map(0, nullptr, Data);
    }
    void UnmapReadbackBuffer() {
        QueryReadbackBuffer->Unmap(0, nullptr);
    }

private:
    FGraphicsDevice* const GraphicsDevice;
    D3D12_QUERY_TYPE const QueryType;
    D3D12_QUERY_HEAP_TYPE const HeapType;
    uint32_t const NumQueries;

    wrl::ComPtr<ID3D12QueryHeap> QueryHeap;
    wrl::ComPtr<ID3D12Resource> QueryReadbackBuffer;
    uint32_t CurrentQueryIndex = 0;
};