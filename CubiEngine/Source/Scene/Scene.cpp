#include "Scene/Scene.h"
#include "Graphics/GraphicsDevice.h"

FScene::FScene(FGraphicsDevice* Device, uint32_t Width, uint32_t Height)
    : Device(Device), Camera(Width, Height)
{
    SceneBuffer = Device->CreateBuffer<interlop::SceneBuffer>(FBufferCreationDesc{
       .Usage = EBufferUsage::ConstantBuffer,
       .Name = L"Scene Buffer",
    });

    LightBuffer = Device->CreateBuffer<interlop::LightBuffer>(FBufferCreationDesc{
        .Usage = EBufferUsage::ConstantBuffer,
        .Name = L"Light Buffer",
    });
    
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
    AddModel(Desc);

    float LightPosition[4] = { 1,-1,1,0 };
    float LightColor[4] = { 1,1,1,1 };
    AddLight(LightPosition, LightColor);

    // set environment map
    EnviromentMap = FCubeMap(Device, FCubeMapCreationDesc{
        .EquirectangularTexturePath = L"Assets/Textures/Environment.hdr",
        .Name = L"Environment Map"
    });
}

FScene::~FScene()
{
}

void FScene::Update(float DeltaTime, FInput* Input, uint32_t Width, uint32_t Height)
{
    Camera.Update(DeltaTime, Input, Width, Height);

    const interlop::SceneBuffer SceneBufferData = {
        .viewProjectionMatrix = Camera.GetViewProjMatrix(),
        .projectionMatrix = Camera.GetProjMatrix(),
        .inverseProjectionMatrix = XMMatrixInverse(nullptr, Camera.GetProjMatrix()),
        .viewMatrix = Camera.GetViewMatrix(),
        .inverseViewMatrix = XMMatrixInverse(nullptr, Camera.GetViewMatrix()),
    };

    SceneBuffer.Update(&SceneBufferData);
    
    const interlop::LightBuffer LightBufferData = Light.GetLightBufferWithViewUpdate(Camera.GetViewMatrix());
    LightBuffer.Update(&LightBufferData);
}

void FScene::AddModel(const FModelCreationDesc& Desc)
{
    const std::wstring Key{ Desc.ModelName };

    Models[Key] = std::make_unique<FModel>(Device, Desc);
}

void FScene::RenderModels(FGraphicsContext* const GraphicsContext,
    interlop::UnlitPassRenderResources& UnlitRenderResources)
{
    UnlitRenderResources.sceneBufferIndex = SceneBuffer.CbvIndex;
    for (const auto& [_, Model] : Models)
    {
        Model->Render(GraphicsContext, UnlitRenderResources);
    }
}

void FScene::RenderModels(FGraphicsContext* const GraphicsContext,
    interlop::DeferredGPassRenderResources& DeferredGRenderResources)
{
    DeferredGRenderResources.sceneBufferIndex = SceneBuffer.CbvIndex;
    for (const auto& [_, Model] : Models)
    {
        Model->Render(GraphicsContext, DeferredGRenderResources);
    }
}

void FScene::RenderEnvironmentMap(FGraphicsContext* const GraphicsContext,
    FTexture& Target, const FTexture& DepthBuffer)
{
    interlop::ScreenSpaceCubeMapRenderResources RenderResource = {
        .sceneBufferIndex = SceneBuffer.CbvIndex,
        .cubenmapTextureIndex = EnviromentMap->CubeMapTexture.SrvIndex,
    };

    if (EnviromentMap.has_value())
    {
        EnviromentMap->Render(GraphicsContext, RenderResource, Target, DepthBuffer);
    }
}
