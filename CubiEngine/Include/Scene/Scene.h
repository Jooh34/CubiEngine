#pragma once

#include "ShaderInterlop/RenderResources.hlsli"
#include "Scene/Camera.h"
#include "Scene/Light.h"
#include "Renderer/CubeMap.h"
#include "Graphics/Raytracing.h"
#include "Scene/Mesh.h"

class FGraphicsContext;
class FCamera;
class FInput;

enum class ERenderingMode
{
    Rasterize,
    DebugRaytracing,
    PathTracing
};

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
    void RenderEnvironmentMap(FGraphicsContext* const GraphicsContext, FSceneTexture& SceneTexture);

    void GenerateRaytracingScene(FGraphicsContext* const GraphicsContext);

    FRaytracingScene& GetRaytracingScene() { return RaytracingScene; }

    FLight Light;
    float CPUFrameMsTime = 0;

    inline ERenderingMode GetRenderingMode() const { return (ERenderingMode)RenderingMode; };

    // UI control
    // Debug
    std::vector<std::string> DebugVisualizeList;
    int DebugVisualizeIndex = 0;
    float VisualizeDebugMin = 0.f;
    float VisualizeDebugMax = 1.f;

    int WhiteFurnaceMethod = 0;

    bool bUseEnergyCompensation = true;

    int DiffuseMethod = 0;
    
    int MaxFPS = 60;

    int RenderingMode = 0;
	int PathTracingSamplePerPixel = 1;

    bool bOverrideBaseColor = false;
    float OverrideBaseColorValue[3] = { 1.f, 1.f, 1.f };
    float OverrideRoughnessValue = -1.f;
    float OverrideMetallicValue = -1.f;
    
    // Light
    bool bLightDanceDebug = false;
    float bLightDanceSpeed = 0.3f;

    bool bUseEnvmap = false;
	float EnvmapIntensity = 0.5f;

    // Shadow
    int ShadowMethod = 1;
    bool bUseVSM = false;
    bool bCSMDebug = false;
    float CSMExponentialFactor = 0.8f;
    float ShadowBias = 1e-4f;
    
    // SSAO
    bool bUseSSAO = true;
    int SSAOKernelSize = 64;
    float SSAOKernelRadius = 4.f;
    float SSAODepthBias = 1e-6f;
    bool SSAOUseRangeCheck = true;

    // SSGI
    int GIMethod = 1;

    float SSGIIntensity = 5.f;
    float SSGIRayLength = 1000.f;
    int SSGINumSteps = 64;
    int SSGINumSamples = 1;
    int StochasticNormalSamplingMethod = 1;
    float CompareToleranceScale = 2.f;
    
    int SSGIGaussianKernelSize = 32;
    float SSGIGaussianStdDev = 12.f;

    int MaxHistoryFrame = 10;

    // PostProcess
    int ToneMappingMethod = 3;
    bool bUseTaa = true;
    bool bGammaCorrection = true;

    bool bUseBloom = true;
    float BloomTint[4] = { 0.1f, 0.040f, 0.020f, 0.010f };

    // auto exposure
    bool bUseEyeAdaptation = true;
    float HistogramLogMin = -2.f;
    float HistogramLogMax = 2.f;
    float TimeCoeff = 0.5f;

    std::unique_ptr<FCubeMap> EnviromentMap{};

private:
    uint32_t Width;
    uint32_t Height;

    static constexpr uint32_t FRAMES_IN_FLIGHT = 3u;
    
	std::vector<FMesh> Meshes{};
    FGraphicsDevice* Device;

    FCamera Camera;
    std::array<FBuffer, FRAMES_IN_FLIGHT> SceneBuffer;
    std::array<FBuffer, FRAMES_IN_FLIGHT> LightBuffer;
    std::array<FBuffer, FRAMES_IN_FLIGHT> ShadowBuffer;
    std::array<FBuffer, FRAMES_IN_FLIGHT> DebugBuffer;
    std::unique_ptr<FCubeMap> WhiteFurnaceMap{};

    std::chrono::high_resolution_clock::time_point PrevTime;

    FRaytracingScene RaytracingScene;
};