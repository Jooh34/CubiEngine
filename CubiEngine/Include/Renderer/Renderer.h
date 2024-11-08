#pragma once

#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"
#include "Renderer/DeferredGPass.h"
#include "Renderer/DebugPass.h"

class FInput;
class FRenderer
{
public:
    FRenderer(FGraphicsDevice* GraphicsDevice, uint32_t Width, uint32_t Height);
    ~FRenderer();

    void Update(float DeltaTime, FInput* Input);
    void BeginFrame(FGraphicsContext* GraphicsContext, FTexture& BackBuffer, FTexture& DepthTexture);
    void Render();
    FGraphicsDevice* GetGraphicsDevice() { return GraphicsDevice; }

private:
    FGraphicsDevice* GraphicsDevice;

    uint32_t Width{};
    uint32_t Height{};
    FTexture DepthTexture;

    std::unique_ptr<FScene> Scene;
    std::unique_ptr<FDeferredGPass> DeferredGPass;
    std::unique_ptr<FDebugPass> DebugPass;
};