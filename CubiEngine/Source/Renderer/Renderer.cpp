#include "Renderer/Renderer.h"
#include "Core/Input.h"
#include "Core/Editor.h"
#include "Graphics/Profiler.h"

#define GI_METHOD_SSGI 1

FRenderer::FRenderer(FGraphicsDevice* GraphicsDevice, SDL_Window* Window, uint32_t Width, uint32_t Height)
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
    TemporalAA = std::make_unique<FTemporalAA>(GraphicsDevice, Width, Height);
    ShadowDepthPass = std::make_unique<FShadowDepthPass>(GraphicsDevice);
    ScreenSpaceGI = std::make_unique<FScreenSpaceGI>(GraphicsDevice, Width, Height);

    Editor = std::make_unique<FEditor>(GraphicsDevice, Window, Width, Height);
}

FRenderer::~FRenderer()
{
    if (GraphicsDevice)
    {
        GraphicsDevice->FlushAllQueue();
    }
}

void FRenderer::GameTick(float DeltaTime, FInput* Input)
{
    Scene->GameTick(DeltaTime, Input, Width, Height);
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
    
    // ----- Shadow Depth pass -----
    if (ShadowDepthPass)
    {
        SCOPED_NAMED_EVENT(GraphicsContext, ShadowDepth);

        ShadowDepthPass->Render(GraphicsContext, Scene.get());
    }
    // ----- Shadow Depth pass -----

    // ----- Deferred GPass ----
    if (DeferredGPass)
    {
        SCOPED_NAMED_EVENT(GraphicsContext, DeferredGPass);
        DeferredGPass->Render(Scene.get(), GraphicsContext, DepthTexture, Width, Height);

        // Render Skybox
        {
            SCOPED_NAMED_EVENT(GraphicsContext, EnvironmentMap);
            Scene->RenderEnvironmentMap(GraphicsContext, DeferredGPass->HDRTexture, DepthTexture);
        }
    }
    // ----- Deferred GPass ----

    // ----- Deferred Lighting Pass -----
    if (DeferredGPass)
    {
        SCOPED_NAMED_EVENT(GraphicsContext, LightPass);

        DeferredGPass->RenderLightPass(Scene.get(), GraphicsContext, ShadowDepthPass.get(), DepthTexture, Width, Height);
    }
    // ----- Deferred Lighting Pass -----

    if (Scene->GIMethod == GI_METHOD_SSGI)
    {
        SCOPED_NAMED_EVENT(GraphicsContext, ScreenSpaceGI);
        ScreenSpaceGI->GenerateStochasticNormal(GraphicsContext, Scene.get(), &DeferredGPass->GBuffer.GBufferB, Width, Height);
        ScreenSpaceGI->RaycastDiffuse(GraphicsContext, Scene.get(), &DeferredGPass->HDRTexture, &DepthTexture, Width, Height);
        ScreenSpaceGI->Denoise(GraphicsContext, Scene.get(), Width, Height);
        ScreenSpaceGI->Resolve(GraphicsContext, Scene.get(), &DeferredGPass->GBuffer.VelocityTexture, Width, Height);
        ScreenSpaceGI->UpdateHistory(GraphicsContext, Scene.get(), Width, Height);
        ScreenSpaceGI->CompositionSSGI(GraphicsContext, Scene.get(), &DeferredGPass->HDRTexture, &DeferredGPass->GBuffer.GBufferA, Width, Height);
    }
    
    // ----- Post Process -----
    {
        SCOPED_NAMED_EVENT(GraphicsContext, PostProcess);

        FTexture* HDR = &DeferredGPass->HDRTexture;
        FTexture* LDR = &PostProcess->LDRTexture;
        
        if (Scene.get()->bUseTaa)
        {
            SCOPED_NAMED_EVENT(GraphicsContext, TemporalAA);

            TemporalAA->Resolve(GraphicsContext, Scene.get(), *HDR, DeferredGPass->GBuffer.VelocityTexture, Width, Height);
            HDR = &TemporalAA->ResolveTexture;

            TemporalAA->UpdateHistory(GraphicsContext, Scene.get(), Width, Height);
        }

        PostProcess->Tonemapping(GraphicsContext, Scene.get(), *HDR, Width, Height);

        // ----- Vis Debug -----
        {
            FTexture* SelectedTexture = GetDebugVisualizeTexture(Scene.get());
            if (SelectedTexture != nullptr)
            {
                PostProcess->DebugVisualize(GraphicsContext, *SelectedTexture, *LDR, Width, Height);
            }
        }
        // ----- Vis Debug -----

        GraphicsContext->AddResourceBarrier(*LDR, D3D12_RESOURCE_STATE_COPY_SOURCE);
        GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
        GraphicsContext->ExecuteResourceBarriers();
        GraphicsContext->CopyResource(BackBuffer.GetResource(), LDR->GetResource());

        // ----- Editor ------
        {
            SCOPED_NAMED_EVENT(GraphicsContext, Editor);

            GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
            GraphicsContext->ExecuteResourceBarriers();
            GraphicsContext->SetRenderTarget(BackBuffer);
            Editor->Render(GraphicsContext, Scene.get());
        }
        // -------------------

        GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_PRESENT);
        GraphicsContext->ExecuteResourceBarriers();
    }
    // ----- Post Process -----

    GraphicsDevice->GetDirectCommandQueue()->ExecuteContext(GraphicsContext);
    GraphicsDevice->Present();
    GraphicsDevice->EndFrame();

    GFrameCount++;
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
    TemporalAA->OnWindowResized(GraphicsDevice, InWidth, InHeight);
    ScreenSpaceGI->OnWindowResized(GraphicsDevice, InWidth, InHeight);
    Editor->OnWindowResized(Width, Height);
}

FTexture* FRenderer::GetDebugVisualizeTexture(FScene* Scene)
{
    std::string Name = Scene->DebugVisualizeList[Scene->DebugVisualizeIndex];

    if (Name.compare("Depth") == 0)
    {
        return &DepthTexture;
    }
    else if (Name.compare("GBufferA") == 0)
    {
        return &DeferredGPass->GBuffer.GBufferA;
    }
    else if (Name.compare("GBufferB") == 0)
    {
        return &DeferredGPass->GBuffer.GBufferB;
    }
    else if (Name.compare("GBufferC") == 0)
    {
        return &DeferredGPass->GBuffer.GBufferC;
    }
    else if (Name.compare("HDRTexture") == 0)
    {
        return &DeferredGPass->HDRTexture;
    }
    else if (Name.compare("TemporalHistory") == 0)
    {
        return &TemporalAA->HistoryTexture;
    }
    else if (Name.compare("StochasticNormal") == 0)
    {
        return &ScreenSpaceGI->StochasticNormalTexture;
    }
    else if (Name.compare("ScreenSpaceGI") == 0)
    {
        return &ScreenSpaceGI->ScreenSpaceGITexture;
    }
    else if (Name.compare("SSGIHistory") == 0)
    {
        return &ScreenSpaceGI->HistoryTexture;
    }
    else if (Name.compare("DenoisedScreenSpaceGITexture") == 0)
    {
        return &ScreenSpaceGI->DenoisedScreenSpaceGITexture;
    }
    else
    {
        return nullptr;
    }
}
