#pragma once

#include "Graphics/Resource.h"
#include "WinPixEventRuntime/pix3.h"
#include "Graphics/Query.h"

class FGraphicsDevice;

class FDeviceChild
{
protected:
    FGraphicsDevice* Parent;

public:
    FDeviceChild(FGraphicsDevice* InParent = nullptr) : Parent(InParent) {}

    FORCEINLINE FGraphicsDevice* GetParentDevice() const
    {
        return Parent;
    }
};

class FGPUEventNode : public FDeviceChild
{
public:
    FGPUEventNode(const char* Name, FGPUEventNode* Parent, FGraphicsDevice* ParentDevice);
    void StartTiming();
    void StopTiming();
    double GetTiming(UINT64* ReadBackData);
    
    FGPUEventNode* Parent;
    std::vector<FGPUEventNode*> Children;

    FQueryLocation BeginQueryLocation;
    FQueryLocation EndQueryLocation;

    const char* Name;
};

struct FProfileData
{
    const char* Name;
    double Duration;
    bool bHasChildren;
};

class FGPUProfiler : public FDeviceChild
{
public:
    FGPUProfiler(FGraphicsDevice* GraphicsDevice);

    void BeginFrame();
    void EndFrame();
    void EndFrameAfterFence();
    void TraverseNode(FGPUEventNode* Node, UINT64* ReadBackData);
    void PushEvent(const char* Name, bool bFrameStart = false);
    void PopEvent(bool bFrameEnd = false);
    void ResolveQueryData();
    std::vector<FProfileData>& GetProfileData() { return ProfileData; }

private:
    FGPUEventNode* RootNode = nullptr;
    FGPUEventNode* CurrentEventNode = nullptr;

    uint32_t StackDepth;

    std::array<FGPUEventNode*, FRAMES_IN_FLIGHT> PendingNodes{};

    uint32_t FrameQueryStartIndex;
    uint32_t FrameQueryEndIndex;

    std::vector<FProfileData> ProfileData;
};

class GPUProfileScopedObject
{
    FGraphicsDevice* Device;

public:
    GPUProfileScopedObject(FGraphicsDevice* Device, const char* Name);
    ~GPUProfileScopedObject();
};

#if ENABLE_PIX_EVENT
    #define SCOPED_NAMED_EVENT(GraphicsContext, NAME)\
        PIXScopedEventObject Event_##NAME = PIXScopedEventObject(GraphicsContext->GetCommandList(), PIX_COLOR(255, 255, 255), #NAME);
#else
    #define SCOPED_NAMED_EVENT(GraphicsContext, NAME)
#endif

#define SCOPED_GPU_EVENT(Device, NAME)\
    GPUProfileScopedObject GPUProfileEvent_##NAME = GPUProfileScopedObject(Device, #NAME);

