#include "Graphics/Profiler.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/D3D12DynamicRHI.h"
#include "Graphics/GraphicsContext.h"

FGPUEventNode::FGPUEventNode(const char* Name, FGPUEventNode* Parent)
    :Name(Name), Parent(Parent)
{
}

void FGPUEventNode::StartTiming()
{
    FGraphicsContext* Context =  RHIGetCurrentGraphicsContext();
    
    BeginQueryLocation = RHIAllocateQuery(D3D12_QUERY_TYPE_TIMESTAMP);
    Context->EndQuery(BeginQueryLocation.Heap, D3D12_QUERY_TYPE_TIMESTAMP, BeginQueryLocation.Index);
}

void FGPUEventNode::StopTiming()
{
    FGraphicsContext* Context = RHIGetCurrentGraphicsContext();

    EndQueryLocation = RHIAllocateQuery(D3D12_QUERY_TYPE_TIMESTAMP);
    Context->EndQuery(EndQueryLocation.Heap, D3D12_QUERY_TYPE_TIMESTAMP, EndQueryLocation.Index);
}

double FGPUEventNode::GetTiming(UINT64* ReadBackData)
{
    double TimingMs = 0;

    UINT64 GpuFrequency = 0;
    FCommandQueue* CommandQueue = RHIGetDirectCommandQueue();
    CommandQueue->GetTimestampFrequency(&GpuFrequency); // Hz (ticks per second)

    uint32_t Timing = 0;
    if (BeginQueryLocation && EndQueryLocation)
    {
        UINT64 TimestampStart = ReadBackData[BeginQueryLocation.Index];
        UINT64 TimestampEnd = ReadBackData[EndQueryLocation.Index];
        Timing = (TimestampEnd - TimestampStart);

        TimingMs = (double)(TimestampEnd - TimestampStart) / GpuFrequency * 1000.0;
    }

    return TimingMs;
}

FGPUProfiler::FGPUProfiler()
    :StackDepth(0)
{
}

void FGPUProfiler::BeginFrame()
{
    PushEvent("GPU Frame", true);
}

void FGPUProfiler::EndFrame()
{
    PopEvent(true);
    ResolveQueryData();
}

void FGPUProfiler::EndFrameAfterFence()
{
    uint32_t Index = GFrameCount % FRAMES_IN_FLIGHT;

    if (PendingNodes[Index])
    {
        UINT64* ReadBackData = nullptr;
        D3D12_RANGE Range;
        Range.Begin = PendingNodes[Index]->BeginQueryLocation.Index;
        Range.End = PendingNodes[Index]->EndQueryLocation.Index;
        RHIGetTimestampQueryHeap()->MapReadbackBuffer(reinterpret_cast<void**>(&ReadBackData));

        // traverse and clear.
        ProfileData.clear();
        TraverseNode(PendingNodes[Index], ReadBackData);

        RHIGetTimestampQueryHeap()->UnmapReadbackBuffer();
    }

    PendingNodes[Index] = RootNode;
}

void FGPUProfiler::TraverseNode(FGPUEventNode* Node, UINT64* ReadBackData)
{
    bool bHasChildren = Node->Children.size() > 0;

    FProfileData Data{
        .Name = Node->Name,
        .Duration = Node->GetTiming(ReadBackData),
        .bHasChildren = bHasChildren,
    };
    ProfileData.push_back(Data);

    for (FGPUEventNode* Child : Node->Children)
    {
        TraverseNode(Child, ReadBackData);
    }
    
    if (bHasChildren)
    {
        // Notify Pop
        FProfileData PopData{
            .Name = nullptr,
            .Duration = 0,
            .bHasChildren = true,
        };
        ProfileData.push_back(PopData);
    }
    
    delete Node;
    Node = nullptr;
}

void FGPUProfiler::PushEvent(const char* Name, bool bFrameStart)
{
    StackDepth++;
    if (CurrentEventNode)
    {
        FGPUEventNode* Node = new FGPUEventNode(Name, CurrentEventNode);
        CurrentEventNode->Children.push_back(Node);
        CurrentEventNode = CurrentEventNode->Children[CurrentEventNode->Children.size() - 1];
    }
    else
    {
        // Add a new root node to the tree
        RootNode = new FGPUEventNode(Name, nullptr);
        CurrentEventNode = RootNode;
    }

    CurrentEventNode->StartTiming();

    if (bFrameStart)
    {
        FrameQueryStartIndex = CurrentEventNode->BeginQueryLocation.Index;
    }
}


void FGPUProfiler::PopEvent(bool bFrameEnd)
{
    assert(StackDepth >= 1);
    StackDepth--;

    assert(CurrentEventNode);

    // Stop timing the current node and move one level up the tree
    CurrentEventNode->StopTiming();
    if (bFrameEnd)
    {
        FrameQueryEndIndex = CurrentEventNode->EndQueryLocation.Index;
    }

    CurrentEventNode = CurrentEventNode->Parent;
}

void FGPUProfiler::ResolveQueryData()
{
    FGraphicsContext* Context = RHIGetCurrentGraphicsContext();

    uint32_t NumFrameQuery = FrameQueryEndIndex - FrameQueryStartIndex + 1;
    UINT64 destinationOffset = FrameQueryStartIndex * sizeof(UINT64);  // Readback 버퍼 내 오프셋

    Context->ResolveQueryData(
        RHIGetTimestampQueryHeap(),
        D3D12_QUERY_TYPE_TIMESTAMP,
        FrameQueryStartIndex, NumFrameQuery,
        destinationOffset
    );
}

GPUProfileScopedObject::GPUProfileScopedObject(const char* Name)
{
    RHIGetGPUProfiler().PushEvent(Name);
}

GPUProfileScopedObject::~GPUProfileScopedObject()
{
    RHIGetGPUProfiler().PopEvent();
}
