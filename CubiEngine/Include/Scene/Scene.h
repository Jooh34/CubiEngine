#pragma once

#include "Scene/Model.h"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Scene/Camera.h"
#include "Scene/Light.h"
#include "Renderer/CubeMap.h"

class FGraphicsContext;
class FCamera;
class FInput;

class FScene
{
public:
    FScene(FGraphicsDevice* Device, uint32_t Width, uint32_t Height);
    ~FScene();

    void GameTick(float DeltaTime, FInput* Input, uint32_t Width, uint32_t Height);
    void HandleMaxTickRate();

    void UpdateBuffers();
    void AddModel(const FModelCreationDesc& Desc);
    void AddLight(float Position[4], float Color[4], float Intensity = 1.f) { Light.AddLight(Position, Color, Intensity); }

    void RenderModels(FGraphicsContext* const GraphicsContext,
        interlop::UnlitPassRenderResources& UnlitRenderResources);
    void RenderModels(FGraphicsContext* const GraphicsContext,
        interlop::DeferredGPassRenderResources& DeferredGRenderResources);
    void RenderModels(FGraphicsContext* const GraphicsContext,
        interlop::ShadowDepthPassRenderResource& ShadowDepthPassRenderResource);

    void RenderLightsDeferred(FGraphicsContext* const GraphicsContext,
        interlop::DeferredGPassCubeRenderResources);

    FBuffer& GetSceneBuffer() { return SceneBuffer[GFrameCount % FRAMES_IN_FLIGHT]; }
    FBuffer& GetLightBuffer() { return LightBuffer[GFrameCount % FRAMES_IN_FLIGHT]; }
    FBuffer& GetShadowBuffer() { return ShadowBuffer[GFrameCount % FRAMES_IN_FLIGHT]; }
    FBuffer& GetDebugBuffer() { return DebugBuffer[GFrameCount % FRAMES_IN_FLIGHT]; }
    FCamera& GetCamera() { return Camera; }
    FCubeMap* GetEnvironmentMap() { return (WhiteFurnaceMethod == 0 || WhiteFurnaceMethod == 3) ? EnviromentMap.get() : WhiteFurnaceMap.get(); }
    void RenderEnvironmentMap(FGraphicsContext* const GraphicsContext,
        FTexture& Target, const FTexture& DepthBuffer);

    FLight Light;
    float CPUFrameMsTime = 0;

    // UI control
    std::vector<std::string> DebugVisualizeList;
    int DebugVisualizeIndex = 0;
    int WhiteFurnaceMethod = 0;
    int GIMethod = 0;

    bool bToneMapping = true;
    bool bGammaCorrection = true;

    bool bUseEnvmap = true;
    bool bUseEnergyCompensation = true;
    bool bCSMDebug = false;

    int DiffuseMethod = 0;
    
    // Debug
    int MaxFPS = 60;
    bool bUseTaa = true;

    // SSGI
    float SSGIIntensity = 1.f;
    float SSGIRayLength = 2000.f;
    int SSGINumSteps = 64;
    int StochasticNormalSamplingMethod = 1; // Cosine weighted importance sampling
    float CompareToleranceScale = 10.f;

private:
    uint32_t Width;
    uint32_t Height;

    static constexpr uint32_t FRAMES_IN_FLIGHT = 3u;

    std::unordered_map<std::wstring, std::unique_ptr<FModel>> Models{};
    FGraphicsDevice* Device;

    FCamera Camera;
    std::array<FBuffer, FRAMES_IN_FLIGHT> SceneBuffer;
    std::array<FBuffer, FRAMES_IN_FLIGHT> LightBuffer;
    std::array<FBuffer, FRAMES_IN_FLIGHT> ShadowBuffer;
    std::array<FBuffer, FRAMES_IN_FLIGHT> DebugBuffer;
    std::unique_ptr<FCubeMap> EnviromentMap{};
    std::unique_ptr<FCubeMap> WhiteFurnaceMap{};

    std::chrono::high_resolution_clock::time_point PrevTime;
};