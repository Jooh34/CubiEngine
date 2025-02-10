#pragma once

#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"
#include "Renderer/DeferredGPass.h"
#include "Renderer/DebugPass.h"
#include "Renderer/PostProcess.h"
#include "Renderer/ShadowDepthPass.h"

class FInput;
class FEditor;
struct SDL_Window;

class FRenderer
{
public:
    FRenderer(FGraphicsDevice* GraphicsDevice, SDL_Window* Window, uint32_t Width, uint32_t Height);
    ~FRenderer();

    void GameTick(float DeltaTime, FInput* Input);
    void BeginFrame(FGraphicsContext* GraphicsContext, FTexture& BackBuffer, FTexture& DepthTexture);
    void Render();
    void OnWindowResized(uint32_t InWidth, uint32_t InHeight);
    FGraphicsDevice* GetGraphicsDevice() { return GraphicsDevice; }

private:
    FGraphicsDevice* GraphicsDevice;

    uint32_t Width{};
    uint32_t Height{};
    FTexture DepthTexture;

    std::unique_ptr<FScene> Scene;
    std::unique_ptr<FDeferredGPass> DeferredGPass;
    std::unique_ptr<FDebugPass> DebugPass;
    std::unique_ptr<FPostProcess> PostProcess;
    std::unique_ptr<FShadowDepthPass> ShadowDepthPass;

    std::unique_ptr<FEditor> Editor;
};