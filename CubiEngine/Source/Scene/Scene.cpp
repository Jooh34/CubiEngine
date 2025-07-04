#include "Scene/Scene.h"
#include "Graphics/GraphicsDevice.h"
#include "Scene/GLTFModelLoader.h"
#include "Scene/FBXLoader.h"
#include "Scene/SceneLoader.h"
#include <thread>

FScene::FScene(FGraphicsDevice* Device, uint32_t Width, uint32_t Height)
    : Device(Device), Camera(Width, Height)
{
    std::chrono::high_resolution_clock Clock{};
    PrevTime = Clock.now();

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        SceneBuffer[i] = Device->CreateBuffer<interlop::SceneBuffer>(FBufferCreationDesc{
           .Usage = EBufferUsage::ConstantBuffer,
           .Name = L"Scene Buffer" + i,
        });

        LightBuffer[i] = Device->CreateBuffer<interlop::LightBuffer>(FBufferCreationDesc{
            .Usage = EBufferUsage::ConstantBuffer,
            .Name = L"Light Buffer" + i,
        });

        ShadowBuffer[i] = Device->CreateBuffer<interlop::ShadowBuffer>(FBufferCreationDesc{
            .Usage = EBufferUsage::ConstantBuffer,
            .Name = L"Shadow Buffer" + i,
        });

        DebugBuffer[i] = Device->CreateBuffer<interlop::DebugBuffer>(FBufferCreationDesc{
            .Usage = EBufferUsage::ConstantBuffer,
            .Name = L"Debug Buffer" + i,
        });
    }

    ESceneType Scene = ESceneType::Sponza;
    FSceneLoader::LoadScene(Scene, this, Device);

    WhiteFurnaceMap = std::make_unique<FCubeMap>(Device, FCubeMapCreationDesc{
        .EquirectangularTexturePath = L"Assets/Textures/WhiteFurnace.hdr",
        .Name = L"WhiteFurnace Map"
    });

    DebugVisualizeList.push_back(std::string("None"));
    DebugVisualizeList.push_back(std::string("Depth"));
    DebugVisualizeList.push_back(std::string("GBufferA"));
    DebugVisualizeList.push_back(std::string("GBufferB"));
    DebugVisualizeList.push_back(std::string("GBufferC"));
    DebugVisualizeList.push_back(std::string("HDRTexture"));
    DebugVisualizeList.push_back(std::string("TemporalHistory"));
    DebugVisualizeList.push_back(std::string("StochasticNormal"));
    DebugVisualizeList.push_back(std::string("ScreenSpaceGI"));
    DebugVisualizeList.push_back(std::string("DenoisedScreenSpaceGITexture"));
    DebugVisualizeList.push_back(std::string("QuarterTexture"));
    DebugVisualizeList.push_back(std::string("SSGIHistory"));
    DebugVisualizeList.push_back(std::string("SSGIHistroyNumFrameAccumulated"));
    DebugVisualizeList.push_back(std::string("DownSampledSceneTexture 1/2"));
    DebugVisualizeList.push_back(std::string("DownSampledSceneTexture 1/4"));
    DebugVisualizeList.push_back(std::string("DownSampledSceneTexture 1/8"));
    DebugVisualizeList.push_back(std::string("DownSampledSceneTexture 1/16"));
    DebugVisualizeList.push_back(std::string("BloomYTexture 1/4"));
    DebugVisualizeList.push_back(std::string("BloomYTexture 1/8"));
    DebugVisualizeList.push_back(std::string("BloomYTexture 1/16"));
    DebugVisualizeList.push_back(std::string("BloomResultTexture"));
    DebugVisualizeList.push_back(std::string("SSAO Texture"));
    DebugVisualizeList.push_back(std::string("RaytracingShadowTexture"));
}

FScene::~FScene()
{

}

void FScene::GameTick(float DeltaTime, FInput* Input, uint32_t Width, uint32_t Height)
{
    this->Width = Width;
    this->Height = Height;
    
    HandleMaxTickRate();

	bool bRasterizeMode = (RenderingMode == (int)ERenderingMode::Rasterize);
    bool bApplyJitter = bUseTaa && bRasterizeMode;

    Camera.Update(DeltaTime, Input, Width, Height, bApplyJitter, CSMExponentialFactor);

    UpdateBuffers();

    if (bLightDanceDebug)
    {
        Light.LightBufferData.lightPosition[0].z = sinf(bLightDanceSpeed * GFrameCount / 100.f);

        if (Light.LightBufferData.numLight > 1)
        {
            Light.LightBufferData.lightPosition[1].x = 500.f * sinf(bLightDanceSpeed * GFrameCount / 100.f);
            Light.LightBufferData.lightPosition[1].z = 500.f * cosf(bLightDanceSpeed * GFrameCount / 100.f);
        }
    }
}

void FScene::HandleMaxTickRate()
{
    std::chrono::high_resolution_clock Clock{};
    std::chrono::high_resolution_clock::time_point CurTime = Clock.now();

    const float DeltaTime = std::chrono::duration<double, std::milli>(CurTime - PrevTime).count();
    
    float MsMax = (1000.f / MaxFPS);
    if (DeltaTime < MsMax)
    {
        float MilliSleep = (MsMax - DeltaTime);

        auto WakeUpTime = PrevTime + std::chrono::duration<double, std::milli>(MsMax-0.5f);
        std::this_thread::sleep_until(WakeUpTime);
    }

    CurTime = Clock.now();
    CPUFrameMsTime = std::chrono::duration<double, std::milli>(CurTime - PrevTime).count();

    PrevTime = CurTime;
}

void FScene::UpdateBuffers()
{
    const interlop::SceneBuffer SceneBufferData = {
        .viewProjectionMatrix = Camera.GetViewProjMatrix(),
        .projectionMatrix = Camera.GetProjMatrix(),
        .inverseProjectionMatrix = XMMatrixInverse(nullptr, Camera.GetProjMatrix()),
        .viewMatrix = Camera.GetViewMatrix(),
        .inverseViewMatrix = XMMatrixInverse(nullptr, Camera.GetViewMatrix()),
        .prevViewProjMatrix = Camera.GetPrevViewProjMatrix(),
        .clipToPrevClip = Camera.GetClipToPrevClip(),
        .invDeviceZToWorldZTransform = Camera.CreateInvDeviceZToWorldZTransform(Camera.GetProjMatrix()),
        .nearZ = Camera.NearZ,
        .farZ = Camera.FarZ,
        .width = Width,
        .height = Height,
		.cameraPosition = Camera.GetCameraPositionF3(),
        .frameCount = GFrameCount,
    };

    GetSceneBuffer().Update(&SceneBufferData);

    const interlop::LightBuffer LightBufferData = Light.GetLightBufferWithViewUpdate(this, Camera.GetViewMatrix());
    GetLightBuffer().Update(&LightBufferData);

    interlop::ShadowBuffer ShadowBufferData = Light.GetShadowBuffer(this);
    ShadowBufferData.shadowBias = ShadowBias;
    GetShadowBuffer().Update(&ShadowBufferData);
    
    const interlop::DebugBuffer DebugBufferData = {
        .bUseTaa = bUseTaa ? 1u : 0u,
        .ShadowMethod = (uint32_t)ShadowMethod,
        .bEnableDiffuse = (uint32_t)bEnableDiffuse,
        .bEnableSpecular = (uint32_t)bEnableSpecular
    };

    GetDebugBuffer().Update(&DebugBufferData);
}

void FScene::AddModel(const FModelCreationDesc& Desc)
{
    std::string_view Extension = GetExtension(Desc.ModelPath);
    if (Extension == "glb" || Extension == "gltf")
    {
		FGLTFModelLoader Model = FGLTFModelLoader(Device, Desc);
		Meshes.insert(
			Meshes.end(),
			std::make_move_iterator(Model.Meshes.begin()),
			std::make_move_iterator(Model.Meshes.end())
		);
    }
	else if (Extension == "fbx")
    {
		FFBXLoader Model = FFBXLoader(Device, Desc);
        Meshes.insert(
			Meshes.end(),
			std::make_move_iterator(Model.Meshes.begin()),
			std::make_move_iterator(Model.Meshes.end())
		);
	}
    else
    {
		throw std::runtime_error("Model format not supported");
	}
}

void FScene::AddMesh(FMesh* Mesh)
{
    Meshes.emplace_back(Mesh);
}

void FScene::RenderModels(FGraphicsContext* const GraphicsContext,
    interlop::UnlitPassRenderResources& UnlitRenderResources)
{
    UnlitRenderResources.sceneBufferIndex = GetSceneBuffer().CbvIndex;
    for (const auto& Mesh : Meshes)
    {
        Mesh->Render(GraphicsContext, UnlitRenderResources);
    }
}

void FScene::RenderModels(FGraphicsContext* const GraphicsContext,
    interlop::DeferredGPassRenderResources& DeferredGRenderResources)
{
    DeferredGRenderResources.sceneBufferIndex = GetSceneBuffer().CbvIndex;
    for (const auto& Mesh : Meshes)
    {
        Mesh->Render(GraphicsContext, this, DeferredGRenderResources);
    }
}

void FScene::RenderModels(FGraphicsContext* const GraphicsContext, interlop::ShadowDepthPassRenderResource& ShadowDepthPassRenderResource)
{
	for (const auto& Mesh : Meshes)
    {
        Mesh->Render(GraphicsContext, ShadowDepthPassRenderResource);
    }
}

void FScene::RenderLightsDeferred(FGraphicsContext* const GraphicsContext,
    interlop::DeferredGPassCubeRenderResources RenderResource)
{
    RenderResource.debugBufferIndex = GetDebugBuffer().CbvIndex;
    RenderResource.sceneBufferIndex = GetSceneBuffer().CbvIndex;

    for (uint32_t i = 1; i < Light.LightBufferData.numLight; i++)
    {
        const XMVECTOR translationVector = XMLoadFloat4(&Light.LightBufferData.lightPosition[i]);
        float scale = 1.f;
        const XMVECTOR scalingVector = { scale, scale, scale, 1.f };

        const XMMATRIX modelMatrix = Dx::XMMatrixTranslationFromVector(translationVector)
            * Dx::XMMatrixScalingFromVector(scalingVector); // no rotation

        RenderResource.modelMatrix = modelMatrix;
        RenderResource.inverseModelMatrix = XMMatrixInverse(nullptr, modelMatrix);
        RenderResource.lightColor = Light.LightBufferData.lightColor[i];
        RenderResource.intensityDistance = Light.LightBufferData.intensityDistance[i];

        GraphicsContext->SetGraphicsRoot32BitConstants(&RenderResource);
        GraphicsContext->DrawInstanced(36, 1, 0, 0);
    }
}

void FScene::RenderEnvironmentMap(FGraphicsContext* const GraphicsContext, FSceneTexture& SceneTexture)
{
    interlop::ScreenSpaceCubeMapRenderResources RenderResource = {
        .sceneBufferIndex = GetSceneBuffer().CbvIndex,
        .cubenmapTextureIndex = GetEnvironmentMap()->CubeMapTexture.SrvIndex,
    };

    if (GetEnvironmentMap())
    {
        GetEnvironmentMap()->Render(GraphicsContext, RenderResource, SceneTexture.HDRTexture, SceneTexture.DepthTexture);
    }
}

void FScene::GenerateRaytracingScene(FGraphicsContext* const GraphicsContext)
{
    std::vector<FRaytracingGeometryContext> RaytracingGeometryContextList;

    for (const auto& Mesh : Meshes)
    {
        Mesh->GatherRaytracingGeometry(RaytracingGeometryContextList);
    }

    if (RaytracingGeometryContextList.size() == 0)
    {
        return;
    }

    RaytracingScene.GenerateRaytracingScene(Device, GraphicsContext, RaytracingGeometryContextList);
}
