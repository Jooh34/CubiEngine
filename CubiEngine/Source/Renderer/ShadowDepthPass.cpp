#include "Renderer/ShadowDepthPass.h"
#include "Graphics/GraphicsDevice.h"
#include "Scene/Scene.h"
#include "ShaderInterlop/RenderResources.hlsli"

FShadowDepthPass::FShadowDepthPass(FGraphicsDevice* const Device)
{
    FTextureCreationDesc Desc{
        .Usage = ETextureUsage::DepthStencil,
        .Width = GShadowDepthDimension,
        .Height = GShadowDepthDimension,
        .Format = DXGI_FORMAT_D32_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE,
        .Name = L"ShadowDepth Texture",
    };

    ShadowDepthTexture = Device->CreateTexture(Desc);

    FGraphicsPipelineStateCreationDesc PipelineStateDesc{
        .ShaderModule =
            {
                .vertexShaderPath = L"Shaders/RenderPass/ShadowDepthPass.hlsl",
                .pixelShaderPath = L"Shaders/RenderPass/ShadowDepthPass.hlsl",
            },
        .RtvFormats = {},
        .RtvCount = 0,
        .PipelineName = L"ShadowDepthPass Pipeline",
    };

    ShadowDepthPassPipelineState = Device->CreatePipelineState(PipelineStateDesc);

    ShadowBuffer = Device->CreateBuffer<interlop::ShadowBuffer>(FBufferCreationDesc{
        .Usage = EBufferUsage::ConstantBuffer,
        .Name = L"Shadow Buffer",
    });

    ShadowBufferData = {};
}

void FShadowDepthPass::Render(FGraphicsContext* GraphicsContext, FScene* Scene)
{
    uint32_t DIndex = 0u;
    XMVECTOR LightDirection = XMVector3Normalize(XMLoadFloat4(&Scene->Light.LightBufferData.lightPosition[DIndex]));

    float SceneRadius = 100.f;
    XMVECTOR CamPos = Scene->GetCamera().GetCameraPosition();
    float NearZ = 0.1f;
    float FarZ = SceneRadius * 2.f;

    XMMATRIX ViewProj = CalculateLightViewProjMatrix(LightDirection, CamPos, SceneRadius, NearZ, FarZ);
    ShadowBufferData.lightViewProjectionMatrix = ViewProj;
    ShadowBuffer.Update(&ShadowBufferData);

    GraphicsContext->SetGraphicsPipelineState(ShadowDepthPassPipelineState);
    GraphicsContext->SetRenderTargetDepthOnly(ShadowDepthTexture);
    GraphicsContext->SetViewport(D3D12_VIEWPORT{
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = static_cast<float>(ShadowDepthTexture.Width),
        .Height = static_cast<float>(ShadowDepthTexture.Height),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    });
    GraphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    GraphicsContext->ClearDepthStencilView(ShadowDepthTexture);

    interlop::ShadowDepthPassRenderResource RenderResources{
        .shadowBufferIndex = ShadowBuffer.CbvIndex,
    };

    Scene->RenderModels(GraphicsContext, RenderResources);
}

XMMATRIX FShadowDepthPass::CalculateLightViewProjMatrix(XMVECTOR LightDirection, XMVECTOR Center, float SceneRadius, float NearZ, float FarZ)
{
    float Width = SceneRadius * 2.0f;
    float Height = SceneRadius * 2.0f;
    
    XMVECTOR EyePos = XMVectorAdd(
        Center,
        XMVectorScale(LightDirection, -1.0f * SceneRadius)
    );
    XMVECTOR UpVector = Dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (Dx::XMVector3Equal(Dx::XMVector3Cross(UpVector, LightDirection), Dx::XMVectorZero())) {
        // 빛 방향이 Y축과 평행하면 Z축을 사용
        UpVector = Dx::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    }

    XMMATRIX ViewMatrix = XMMatrixLookAtLH(EyePos, Center, UpVector);
    XMMATRIX OrthoMatrix = Dx::XMMatrixOrthographicLH(Width, Height, NearZ, FarZ);
    return XMMatrixMultiply(ViewMatrix, OrthoMatrix);
}
