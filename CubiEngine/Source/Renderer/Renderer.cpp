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
    PostProcess = std::make_unique<FPostProcess>(GraphicsDevice, Width, Height);
}

FRenderer::~FRenderer()
{
}

void FRenderer::Update(float DeltaTime, FInput* Input)
{
    Scene->Update(DeltaTime, Input, Width, Height);
}

void FRenderer::BeginFrame(FGraphicsContext* GraphicsContext, FTexture& BackBuffer, FTexture& DepthTexture)
{
    GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
    if (DeferredGPass)
    {
        FGBuffer& GBuffer = DeferredGPass->GBuffer;
        GraphicsContext->AddResourceBarrier(GBuffer.GBufferA, D3D12_RESOURCE_STATE_RENDER_TARGET);
        GraphicsContext->AddResourceBarrier(GBuffer.GBufferB, D3D12_RESOURCE_STATE_RENDER_TARGET);
        GraphicsContext->AddResourceBarrier(GBuffer.GBufferC, D3D12_RESOURCE_STATE_RENDER_TARGET);
        GraphicsContext->AddResourceBarrier(DeferredGPass->HDRTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    GraphicsContext->AddResourceBarrier(DepthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    GraphicsContext->ExecuteResourceBarriers();

    float RenderTargetClearValue[4] = { 0,0,0,1 };
    GraphicsContext->ClearRenderTargetView(BackBuffer, RenderTargetClearValue);
    GraphicsContext->ClearDepthStencilView(DepthTexture);

    GraphicsContext->SetGraphicsRootSignature();
    GraphicsContext->SetComputeRootSignature();
}

void FRenderer::Render()
{
    GraphicsDevice->BeginFrame();

    FGraphicsContext* GraphicsContext = GraphicsDevice->GetCurrentGraphicsContext();
    FTexture& BackBuffer = GraphicsDevice->GetCurrentBackBuffer();

    // Resource Transition + BackBuffer Clear
    BeginFrame(GraphicsContext, BackBuffer, DepthTexture);
    
    // ----- Deferred GPass ----
    if (DeferredGPass)
    {
        DeferredGPass->Render(Scene.get(), GraphicsContext, DepthTexture, Width, Height);
    }
    // ----- Deferred GPass ----
    
    // ----- Deferred Lighting Pass -----
    if (DeferredGPass)
    {
        DeferredGPass->RenderLightPass(Scene.get(), GraphicsContext, DepthTexture, Width, Height);
    }
    // ----- Deferred Lighting Pass -----

    // ----- Debug Mode -----
    bool bDebugTexture = false;
    if (bDebugTexture)
    {
        //FTexture& TextureToDebug = DeferredGPass->GBuffer.GBufferB;

        //// Copy to final image
        //GraphicsContext->ExecuteResourceBarriers();

        //DebugPass->Copy(GraphicsContext, TextureToDebug, DebugPass->TextureForCopy, Width, Height);

        //GraphicsContext->AddResourceBarrier(DebugPass->TextureForCopy, D3D12_RESOURCE_STATE_COPY_SOURCE);
        //GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
        //GraphicsContext->CopyResource(BackBuffer.GetResource(), DebugPass->TextureForCopy.GetResource());

        //GraphicsContext->AddResourceBarrier(TextureToDebug, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    // ----- Debug Mode -----

    // ----- Post Process -----
    {
        FTexture& HDR = DeferredGPass->HDRTexture;
        FTexture& LDR = PostProcess->LDRTexture;
        
        PostProcess->Tonemapping(GraphicsContext, HDR, Width, Height);

        GraphicsContext->AddResourceBarrier(LDR, D3D12_RESOURCE_STATE_COPY_SOURCE);
        GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
        GraphicsContext->ExecuteResourceBarriers();
        GraphicsContext->CopyResource(BackBuffer.GetResource(), LDR.GetResource());

        GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_PRESENT);
        GraphicsContext->ExecuteResourceBarriers();
    }
    // ----- Post Process -----

    GraphicsDevice->GetDirectCommandQueue()->ExecuteContext(GraphicsContext);
    GraphicsDevice->Present();
    GraphicsDevice->EndFrame();
}

void FRenderer::OnWindowResized(uint32_t InWidth, uint32_t InHeight)
{
    GraphicsDevice->OnWindowResized(InWidth, InHeight);

    Width = InWidth;
    Height = InHeight;

    FTextureCreationDesc DepthTextureDesc = {
        .Usage = ETextureUsage::DepthStencil,
        .Width = Width,
        .Height = Height,
        .Format = DXGI_FORMAT_D32_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE,
        .Name = L"Depth Texture",
    };

    DepthTexture = GraphicsDevice->CreateTexture(DepthTextureDesc);
    
    DeferredGPass->OnWindowResized(GraphicsDevice, InWidth, InHeight);
    DebugPass->OnWindowResized(GraphicsDevice, InWidth, InHeight);
    PostProcess->OnWindowResized(GraphicsDevice, InWidth, InHeight);
}
