#include "Graphics/Query.h"
#include "Graphics/GraphicsDevice.h"

FQueryHeap::FQueryHeap(FGraphicsDevice* GraphicsDevice, D3D12_QUERY_TYPE QueryType, D3D12_QUERY_HEAP_TYPE HeapType)
    : GraphicsDevice(GraphicsDevice)
    , QueryType(QueryType)
    , HeapType(HeapType)
    , NumQueries(MaxHeapSize / GetResultSize())
{
    D3D12_QUERY_HEAP_DESC QueryHeapDesc = {};
    QueryHeapDesc.Count = NumQueries;  // 시작과 끝 두 개의 timestamp
    QueryHeapDesc.Type = HeapType;
    QueryHeapDesc.NodeMask = 0;

    GraphicsDevice->GetDevice()->CreateQueryHeap(&QueryHeapDesc, IID_PPV_ARGS(&QueryHeap));

    // Query 결과를 저장할 Readback Buffer 생성
    D3D12_RESOURCE_DESC readbackBufferDesc = {};
    readbackBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    readbackBufferDesc.Width = GetResultSize() * NumQueries;
    readbackBufferDesc.Height = 1;
    readbackBufferDesc.DepthOrArraySize = 1;
    readbackBufferDesc.MipLevels = 1;
    readbackBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    readbackBufferDesc.SampleDesc.Count = 1;
    readbackBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    readbackBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    
    CD3DX12_HEAP_PROPERTIES HeapProperies = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    ThrowIfFailed(
        GraphicsDevice->GetDevice()->CreateCommittedResource(
            &HeapProperies,
            D3D12_HEAP_FLAG_NONE,
            &readbackBufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&QueryReadbackBuffer)
        )
    );
}
