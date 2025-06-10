#pragma once

#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"
#include "Renderer/DeferredGPass.h"
#include "Renderer/DebugPass.h"
#include "Renderer/PostProcess.h"
#include "Renderer/TemporalAA.h"
#include "Renderer/ShadowDepthPass.h"
#include "Renderer/ScreenSpaceGI.h"
#include "Renderer/EyeAdaptationPass.h"
#include "Renderer/BloomPass.h"
#include "Renderer/SSAO.h"
#include "Renderer/RaytracingDebugScenePass.h"
#include "Renderer/RaytracingShadowPass.h"
#include "Renderer/PathTracing.h"

class FInput;
class FEditor;
struct SDL_Window;

class FRenderer
{
public:
    FRenderer(FGraphicsDevice* GraphicsDevice, SDL_Window* Window, uint32_t Width, uint32_t Height);
    ~FRenderer();

    void GameTick(float DeltaTime, FInput* Input);
    void BeginFrame(FGraphicsContext* GraphicsContext, FTexture& BackBuffer);
    void CopyHistoricalTexture(FGraphicsContext* GraphicsContext);
    void Render();
    void RenderDeferredShading(FGraphicsContext* GraphicsContext);
    void RenderDebugRaytracingScene(FGraphicsContext* GraphicsContext);
    void RenderPathTracingScene(FGraphicsContext* GraphicsContext);

    void RenderShadow(FGraphicsContext* GraphicsContext, FSceneTexture& SceneTexture);

    void OnWindowResized(uint32_t InWidth, uint32_t InHeight);
    void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);
    void InitializeSceneTexture(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight);

    FGraphicsDevice* GetGraphicsDevice() { return GraphicsDevice; }

private:
    FSceneTexture SceneTexture;

    FTexture* GetDebugVisualizeTexture(FScene* Scene);

    FGraphicsDevice* GraphicsDevice;

    uint32_t Width{};
    uint32_t Height{};

    std::unique_ptr<FScene> Scene;
    std::unique_ptr<FDeferredGPass> DeferredGPass;
    std::unique_ptr<FDebugPass> DebugPass;
    std::unique_ptr<FTemporalAA> TemporalAA;
    std::unique_ptr<FPostProcess> PostProcess;

    std::unique_ptr<FShadowDepthPass> ShadowDepthPass;
    std::unique_ptr<FRaytracingShadowPass> RaytracingShadowPass;

    std::unique_ptr<FScreenSpaceGI> ScreenSpaceGI;
    std::unique_ptr<FEyeAdaptationPass> EyeAdaptationPass;
    std::unique_ptr<FBloomPass> BloomPass;
    std::unique_ptr<FSSAO> SSAOPass;

    std::unique_ptr<FRaytracingDebugScenePass> RaytracingDebugScenePass;
    std::unique_ptr<FPathTracingPass> PathTracingPass;

    std::unique_ptr<FEditor> Editor;
};