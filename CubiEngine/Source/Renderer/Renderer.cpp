#include "Renderer/Renderer.h"
#include "Core/Input.h"

FRenderer::FRenderer(FGraphicsDevice* GraphicsDevice, uint32_t Width, uint32_t Height)
    :Width(Width), Height(Height), GraphicsDevice(GraphicsDevice)
{
    Scene = std::make_unique<FScene>(GraphicsDevice, Width, Height);

    FTextureCreationDesc DepthTextureDesc = {
        .Usage = ETextureUsage::DepthStencil,
        .Width = Width,
        .Height = Height,
        .Format = DXGI_FORMAT_D32_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE,
        .Name = L"Depth Texture",
    };

    DepthTexture = GraphicsDevice->CreateTexture(DepthTextureDesc);

    DeferredGPass = std::make_unique<FDeferredGPass>(GraphicsDevice, Width, Height);
    DebugPass = std::make_unique<FDebugPass>(GraphicsDevice, Width, Height);
}

FRenderer::~FRenderer()
{
}

void FRenderer::Update(float DeltaTime, FInput* Input)
{
    Scene->Update(DeltaTime, Input);
}

void FRenderer::Render()
{
    GraphicsDevice->BeginFrame();

    FGraphicsContext* GraphicsContext = GraphicsDevice->GetCurrentGraphicsContext();
    FTexture& BackBuffer = GraphicsDevice->GetCurrentBackBuffer();

    GraphicsContext->AddResourceBarrier(BackBuffer.GetResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    GraphicsContext->ExecuteResourceBarriers();
    
    float RenderTargetClearValue[4] = { 0,0,0,1 };

    GraphicsContext->ClearRenderTargetView(BackBuffer, RenderTargetClearValue);
    GraphicsContext->ClearDepthStencilView(DepthTexture);
    
    GraphicsContext->SetGraphicsRootSignature();
    GraphicsContext->SetComputeRootSignature();

    //if (UnlitPass)
    //{
    //    UnlitPass->Render(Scene.get(), GraphicsContext, DepthTexture, Width, Height);

    //    // Copy to final image
    //    GraphicsContext->AddResourceBarrier(UnlitPass->UnlitTexture.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    //    GraphicsContext->AddResourceBarrier(BackBuffer.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
    //    GraphicsContext->ExecuteResourceBarriers();

    //    GraphicsContext->CopyResource(BackBuffer.GetResource(), UnlitPass->UnlitTexture.GetResource());

    //    GraphicsContext->AddResourceBarrier(UnlitPass->UnlitTexture.GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    //    GraphicsContext->AddResourceBarrier(BackBuffer.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
    //    GraphicsContext->ExecuteResourceBarriers();
    //}
    if (DeferredGPass)
    {
        DeferredGPass->Render(Scene.get(), GraphicsContext, DepthTexture, Width, Height);
        
        FTexture& TextureToDebug = DeferredGPass->GBuffer.GBufferB;

        // Copy to final image
        GraphicsContext->AddResourceBarrier(TextureToDebug.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
        GraphicsContext->AddResourceBarrier(BackBuffer.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
        GraphicsContext->ExecuteResourceBarriers();

        DebugPass->Copy(GraphicsContext, TextureToDebug, DebugPass->TextureForCopy, Width, Height);

        GraphicsContext->CopyResource(BackBuffer.GetResource(), DebugPass->TextureForCopy.GetResource());

        GraphicsContext->AddResourceBarrier(TextureToDebug.GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        GraphicsContext->AddResourceBarrier(BackBuffer.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
        GraphicsContext->ExecuteResourceBarriers();
    }

    GraphicsDevice->GetDirectCommandQueue()->ExecuteContext(GraphicsContext);
    GraphicsDevice->Present();
    GraphicsDevice->EndFrame();
}
