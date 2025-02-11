#include "Scene/Scene.h"
#include "Graphics/GraphicsDevice.h"

FScene::FScene(FGraphicsDevice* Device, uint32_t Width, uint32_t Height)
    : Device(Device), Camera(Width, Height)
{
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

        DebugBuffer[i] = Device->CreateBuffer<interlop::DebugBuffer>(FBufferCreationDesc{
            .Usage = EBufferUsage::ConstantBuffer,
            .Name = L"Debug Buffer" + i,
        });
    }
    
    int Scene = 0;
    FModelCreationDesc Desc;
    if (Scene == 0)
    {
        Desc = {
            .ModelPath = "Models/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf",
            .ModelName = L"MetalRoughSpheres",
        };
    }
    else if (Scene == 1)
    {
        Desc = {
            .ModelPath = "Models/Sponza/sponza.glb",
            .ModelName = L"Sponza",
        };
    }
    else if (Scene == 2)
    {
        Desc = {
            .ModelPath = "Models/FlightHelmet/glTF/FlightHelmet.gltf",
            .ModelName = L"FlightHelmet",
        };
    }

    AddModel(Desc);

    float LightPosition[4] = { 1,-1,0,0 };
    float LightColor[4] = { 1,1,1,1 };
    AddLight(LightPosition, LightColor);

    // set environment map
    EnviromentMap = std::make_unique<FCubeMap>(Device, FCubeMapCreationDesc{
        .EquirectangularTexturePath = L"Assets/Textures/pisa.hdr",
        .Name = L"Environment Map"
    });

    WhiteFurnaceMap = std::make_unique<FCubeMap>(Device, FCubeMapCreationDesc{
        .EquirectangularTexturePath = L"Assets/Textures/WhiteFurnace.hdr",
        .Name = L"WhiteFurnace Map"
    });
}

FScene::~FScene()
{
}

void FScene::GameTick(float DeltaTime, FInput* Input, uint32_t Width, uint32_t Height)
{
    this->Width = Width;
    this->Height = Height;
    Camera.Update(DeltaTime, Input, Width, Height);

    UpdateBuffers();
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
        .nearZ = Camera.NearZ,
        .farZ = Camera.FarZ,
        .width = Width,
        .height = Height,
        .frameCount = GFrameCount,
    };

    GetSceneBuffer().Update(&SceneBufferData);

    const interlop::LightBuffer LightBufferData = Light.GetLightBufferWithViewUpdate(this, Camera.GetViewMatrix());
    GetLightBuffer().Update(&LightBufferData);

    const interlop::DebugBuffer DebugBufferData = {
        .bUseTaa = bUseTaa ? 1u : 0u,
    };
    GetDebugBuffer().Update(&DebugBufferData);
}

void FScene::AddModel(const FModelCreationDesc& Desc)
{
    const std::wstring Key{ Desc.ModelName };

    Models[Key] = std::make_unique<FModel>(Device, Desc);
}

void FScene::RenderModels(FGraphicsContext* const GraphicsContext,
    interlop::UnlitPassRenderResources& UnlitRenderResources)
{
    UnlitRenderResources.sceneBufferIndex = GetSceneBuffer().CbvIndex;
    for (const auto& [_, Model] : Models)
    {
        Model->Render(GraphicsContext, UnlitRenderResources);
    }
}

void FScene::RenderModels(FGraphicsContext* const GraphicsContext,
    interlop::DeferredGPassRenderResources& DeferredGRenderResources)
{
    DeferredGRenderResources.sceneBufferIndex = GetSceneBuffer().CbvIndex;
    for (const auto& [_, Model] : Models)
    {
        Model->Render(GraphicsContext, this, DeferredGRenderResources);
    }
}

void FScene::RenderModels(FGraphicsContext* const GraphicsContext, interlop::ShadowDepthPassRenderResource& ShadowDepthPassRenderResource)
{
    for (const auto& [_, Model] : Models)
    {
        Model->Render(GraphicsContext, ShadowDepthPassRenderResource);
    }
}

void FScene::RenderEnvironmentMap(FGraphicsContext* const GraphicsContext,
    FTexture& Target, const FTexture& DepthBuffer)
{
    interlop::ScreenSpaceCubeMapRenderResources RenderResource = {
        .sceneBufferIndex = GetSceneBuffer().CbvIndex,
        .cubenmapTextureIndex = GetEnvironmentMap()->CubeMapTexture.SrvIndex,
    };

    if (GetEnvironmentMap())
    {
        GetEnvironmentMap()->Render(GraphicsContext, RenderResource, Target, DepthBuffer);
    }
}
