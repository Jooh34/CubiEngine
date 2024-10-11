#include "Scene/Scene.h"
#include "Graphics/GraphicsDevice.h"

FScene::FScene(FGraphicsDevice* Device, uint32_t Width, uint32_t Height)
    : Device(Device), Camera(Width, Height)
{
    SceneBuffer = Device->CreateBuffer<interlop::SceneBuffer>(FBufferCreationDesc{
       .Usage = EBufferUsage::ConstantBuffer,
       .Name = "Scene Buffer",
    });

    FModelCreationDesc Desc{
        .ModelPath = "Models/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf",
        .ModelName = "MetalRoughSpheres",
    };
    AddModel(Desc);
}

FScene::~FScene()
{
}

void FScene::Update()
{
    const interlop::SceneBuffer SceneBufferData = {
        .viewProjectionMatrix = Camera.GetViewProjMatrix(),
        .projectionMatrix = Camera.GetProjMatrix(),
        .inverseProjectionMatrix = XMMatrixInverse(nullptr, Camera.GetProjMatrix()),
        .viewMatrix = Camera.GetViewMatrix(),
        .inverseViewMatrix = XMMatrixInverse(nullptr, Camera.GetViewMatrix()),
    };

    SceneBuffer.Update(&SceneBufferData);
}

void FScene::AddModel(const FModelCreationDesc& Desc)
{
    const std::string Key{ Desc.ModelName };

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
