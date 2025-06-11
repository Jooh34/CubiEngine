#include "Renderer/Renderer.h"
#include "Core/Input.h"
#include "Core/Editor.h"
#include "Graphics/Profiler.h"

#include "Renderer/PathTracing.h"

#define GI_METHOD_SSGI 1

FRenderer::FRenderer(FGraphicsDevice* GraphicsDevice, SDL_Window* Window, uint32_t Width, uint32_t Height)
    :Width(Width), Height(Height), GraphicsDevice(GraphicsDevice)
{
    Scene = std::make_unique<FScene>(GraphicsDevice, Width, Height);

    DeferredGPass = std::make_unique<FDeferredGPass>(GraphicsDevice, Width, Height);
    DebugPass = std::make_unique<FDebugPass>(GraphicsDevice, Width, Height);
    PostProcess = std::make_unique<FPostProcess>(GraphicsDevice, Width, Height);
    TemporalAA = std::make_unique<FTemporalAA>(GraphicsDevice, Width, Height);
    ShadowDepthPass = std::make_unique<FShadowDepthPass>(GraphicsDevice);
    ScreenSpaceGI = std::make_unique<FScreenSpaceGI>(GraphicsDevice, Width, Height);
    EyeAdaptationPass = std::make_unique<FEyeAdaptationPass>(GraphicsDevice, Width, Height);
    BloomPass = std::make_unique<FBloomPass>(GraphicsDevice, Width, Height);
    SSAOPass = std::make_unique<FSSAO>(GraphicsDevice, Width, Height);

    RaytracingDebugScenePass = std::make_unique<FRaytracingDebugScenePass>(GraphicsDevice, Scene.get(), Width, Height);
    RaytracingShadowPass = std::make_unique<FRaytracingShadowPass>(GraphicsDevice, Scene.get(), Width, Height);

	PathTracingPass = std::make_unique<FPathTracingPass>(GraphicsDevice, Scene.get(), Width, Height);

    Editor = std::make_unique<FEditor>(GraphicsDevice, Window, Width, Height);

    InitSizeDependantResource(GraphicsDevice, Width, Height);
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
    Editor->GameTick(DeltaTime, Input);
}

void FRenderer::BeginFrame(FGraphicsContext* GraphicsContext, FTexture& BackBuffer)
{
    GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
    if (DeferredGPass)
    {
        GraphicsContext->AddResourceBarrier(SceneTexture.GBufferA, D3D12_RESOURCE_STATE_RENDER_TARGET);
        GraphicsContext->AddResourceBarrier(SceneTexture.GBufferB, D3D12_RESOURCE_STATE_RENDER_TARGET);
        GraphicsContext->AddResourceBarrier(SceneTexture.GBufferC, D3D12_RESOURCE_STATE_RENDER_TARGET);
        GraphicsContext->AddResourceBarrier(SceneTexture.VelocityTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
        GraphicsContext->AddResourceBarrier(SceneTexture.HDRTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    GraphicsContext->AddResourceBarrier(SceneTexture.DepthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    GraphicsContext->ExecuteResourceBarriers();

    float RenderTargetClearValue[4] = { 0,0,0,1 };
    GraphicsContext->ClearRenderTargetView(BackBuffer, RenderTargetClearValue);
    GraphicsContext->ClearDepthStencilView(SceneTexture.DepthTexture);

    GraphicsContext->SetGraphicsRootSignature();
    GraphicsContext->SetComputeRootSignature();
}

void FRenderer::CopyHistoricalTexture(FGraphicsContext* GraphicsContext)
{
    SCOPED_NAMED_EVENT(GraphicsContext, CopyHistoricalTexture);

    // Copy Prev Depth Texture
    GraphicsContext->AddResourceBarrier(SceneTexture.DepthTexture, D3D12_RESOURCE_STATE_COPY_SOURCE);
    GraphicsContext->AddResourceBarrier(SceneTexture.PrevDepthTexture, D3D12_RESOURCE_STATE_COPY_DEST);
    GraphicsContext->ExecuteResourceBarriers();
    GraphicsContext->CopyResource(SceneTexture.PrevDepthTexture.GetResource(), SceneTexture.DepthTexture.GetResource());
}

void FRenderer::Render()
{
    GraphicsDevice->BeginFrame();
    GraphicsDevice->GetGPUProfiler().BeginFrame();

    FGraphicsContext* GraphicsContext = GraphicsDevice->GetCurrentGraphicsContext();
    FTexture& BackBuffer = GraphicsDevice->GetCurrentBackBuffer();

    // Resource Transition + BackBuffer Clear
    BeginFrame(GraphicsContext, BackBuffer);

    // Generate Raytracing Scene
    {
        SCOPED_NAMED_EVENT(GraphicsContext, GenerateRaytracingScene);
        SCOPED_GPU_EVENT(GraphicsDevice, GenerateRaytracingScene);
        Scene->GenerateRaytracingScene(GraphicsContext);
    }

    if (Scene->GetRenderingMode() == ERenderingMode::Rasterize)
    {
        RenderDeferredShading(GraphicsContext);
    }
	else if (Scene->GetRenderingMode() == ERenderingMode::DebugRaytracing)
    {
        RenderDebugRaytracingScene(GraphicsContext);
    }
    else
    {
		// Path Tracing Mode
        RenderPathTracingScene(GraphicsContext);
    }

    // ----- Editor ------
    {
        SCOPED_NAMED_EVENT(GraphicsContext, Editor);
        SCOPED_GPU_EVENT(GraphicsDevice, Editor);

        GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
        GraphicsContext->ExecuteResourceBarriers();
        GraphicsContext->SetRenderTarget(BackBuffer);
        Editor->Render(GraphicsContext, Scene.get());
    }
    // -------------------

    GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_PRESENT);
    GraphicsContext->ExecuteResourceBarriers();

    GraphicsDevice->GetGPUProfiler().EndFrame();

    GraphicsDevice->GetDirectCommandQueue()->ExecuteContext(GraphicsContext);
    GraphicsDevice->Present();

    GraphicsDevice->EndFrame();
    GraphicsDevice->GetGPUProfiler().EndFrameAfterFence();

    GFrameCount++;
}

void FRenderer::RenderDeferredShading(FGraphicsContext* GraphicsContext)
{
    FTexture& BackBuffer = GraphicsDevice->GetCurrentBackBuffer();

    // ----- Deferred GPass ----
    if (DeferredGPass)
    {
        SCOPED_NAMED_EVENT(GraphicsContext, DeferredGPass);
        SCOPED_GPU_EVENT(GraphicsDevice, DeferredGPass);
        DeferredGPass->Render(Scene.get(), GraphicsContext, SceneTexture);

        // Render Skybox
        {
            SCOPED_NAMED_EVENT(GraphicsContext, EnvironmentMap);
            SCOPED_GPU_EVENT(GraphicsDevice, EnvironmentMap);
            Scene->RenderEnvironmentMap(GraphicsContext, SceneTexture);
        }
    }
    // ----- Deferred GPass ----

    // ----- Shadow pass -----
    RenderShadow(GraphicsContext, SceneTexture);
    // ----- Shadow Depth pass -----

    // ----- Screen Space Ambient Occlusion -----
    if (Scene->bUseSSAO)
    {
        SCOPED_NAMED_EVENT(GraphicsContext, SSAO);
        SCOPED_GPU_EVENT(GraphicsDevice, SSAO);

        SSAOPass->AddSSAOPass(GraphicsContext, Scene.get(), SceneTexture);
    }
    // ----- Screen Space Ambient Occlusion -----

    // ----- Deferred Lighting Pass -----
    if (DeferredGPass)
    {
        SCOPED_NAMED_EVENT(GraphicsContext, LightPass);
        SCOPED_GPU_EVENT(GraphicsDevice, LightPass);

        FTexture* SSAOTexture = Scene->bUseSSAO ? &SSAOPass->SSAOTexture : nullptr;
        DeferredGPass->RenderLightPass(Scene.get(), GraphicsContext,
            ShadowDepthPass.get(), SceneTexture, SSAOTexture, &RaytracingShadowPass->GetRaytracingShadowTexture()
        );
    }
    // ----- Deferred Lighting Pass -----

    if (Scene->GIMethod == GI_METHOD_SSGI)
    {
        SCOPED_NAMED_EVENT(GraphicsContext, ScreenSpaceGI);
        SCOPED_GPU_EVENT(GraphicsDevice, ScreenSpaceGI);

        ScreenSpaceGI->AddPass(GraphicsContext, Scene.get(), SceneTexture);
    }
    
    // ----- Post Process -----
    {
        SCOPED_NAMED_EVENT(GraphicsContext, PostProcess);
        SCOPED_GPU_EVENT(GraphicsDevice, PostProcess);

        FTexture* HDR = &SceneTexture.HDRTexture;
        FTexture* LDR = &SceneTexture.LDRTexture;

        if (Scene.get()->bUseTaa)
        {
            SCOPED_NAMED_EVENT(GraphicsContext, TemporalAA);
            SCOPED_GPU_EVENT(GraphicsDevice, TemporalAA);

            TemporalAA->Resolve(GraphicsContext, Scene.get(), SceneTexture);
            HDR = &TemporalAA->ResolveTexture;

            TemporalAA->UpdateHistory(GraphicsContext, Scene.get());
        }

        if (Scene.get()->bUseEyeAdaptation)
        {
            SCOPED_NAMED_EVENT(GraphicsContext, EyeAdaptation);
            SCOPED_GPU_EVENT(GraphicsDevice, EyeAdaptation);

            EyeAdaptationPass->GenerateHistogram(GraphicsContext, Scene.get(), HDR);
            EyeAdaptationPass->CalculateAverageLuminance(GraphicsContext, Scene.get(), Width, Height);
        }

        if (Scene->bUseBloom)
        {
            SCOPED_NAMED_EVENT(GraphicsContext, Bloom);
            SCOPED_GPU_EVENT(GraphicsDevice, Bloom);
            BloomPass->AddBloomPass(GraphicsContext, Scene.get(), HDR);
        }

        {
            SCOPED_NAMED_EVENT(GraphicsContext, ToneMapping);
            SCOPED_GPU_EVENT(GraphicsDevice, ToneMapping);
            EyeAdaptationPass->ToneMapping(GraphicsContext, Scene.get(), HDR, LDR,
                Scene->bUseBloom ? &BloomPass->BloomResultTexture : nullptr);
        }

        // PostProcess->Tonemapping(GraphicsContext, Scene.get(), *HDR, Width, Height);

        // ----- Vis Debug -----
        {
            SCOPED_NAMED_EVENT(GraphicsContext, DebugVisualize);
            SCOPED_GPU_EVENT(GraphicsDevice, DebugVisualize);
            FTexture* SelectedTexture = GetDebugVisualizeTexture(Scene.get());
            if (SelectedTexture != nullptr)
            {
                PostProcess->DebugVisualize(GraphicsContext, Scene.get(), *SelectedTexture, *LDR, Width, Height);
            }
        }
        // ----- Vis Debug -----

        GraphicsContext->AddResourceBarrier(*LDR, D3D12_RESOURCE_STATE_COPY_SOURCE);
        GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
        GraphicsContext->ExecuteResourceBarriers();
        GraphicsContext->CopyResource(BackBuffer.GetResource(), LDR->GetResource());
    }

    CopyHistoricalTexture(GraphicsContext);
    // ----- Post Process -----
}

void FRenderer::RenderDebugRaytracingScene(FGraphicsContext* GraphicsContext)
{
    if (RaytracingDebugScenePass)
    {
        SCOPED_NAMED_EVENT(GraphicsContext, RaytracingDebugScene);
        SCOPED_GPU_EVENT(GraphicsDevice, RaytracingDebugScene);

        RaytracingDebugScenePass->AddPass(GraphicsContext, Scene.get());
    }

    // ----- Post Process -----
    {
        SCOPED_NAMED_EVENT(GraphicsContext, PostProcess);
        SCOPED_GPU_EVENT(GraphicsDevice, PostProcess);

        FTexture* HDR = &RaytracingDebugScenePass->GetRaytracingDebugSceneTexture();
        FTexture* LDR = &SceneTexture.LDRTexture;

        {
            SCOPED_NAMED_EVENT(GraphicsContext, ToneMapping);
            SCOPED_GPU_EVENT(GraphicsDevice, ToneMapping);
            PostProcess->Tonemapping(GraphicsContext, Scene.get(), *HDR, *LDR, Width, Height);
        }

        FTexture& BackBuffer = GraphicsDevice->GetCurrentBackBuffer();

        GraphicsContext->AddResourceBarrier(*LDR, D3D12_RESOURCE_STATE_COPY_SOURCE);
        GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
        GraphicsContext->ExecuteResourceBarriers();
        GraphicsContext->CopyResource(BackBuffer.GetResource(), LDR->GetResource());
    }
}

void FRenderer::RenderPathTracingScene(FGraphicsContext* GraphicsContext)
{
    if (PathTracingPass)
    {
        SCOPED_NAMED_EVENT(GraphicsContext, PathTracing);
        SCOPED_GPU_EVENT(GraphicsDevice, PathTracing);

        PathTracingPass->AddPass(GraphicsContext, Scene.get());
    }

    // ----- Post Process -----
    {
        SCOPED_NAMED_EVENT(GraphicsContext, PostProcess);
        SCOPED_GPU_EVENT(GraphicsDevice, PostProcess);

        FTexture* HDR = &PathTracingPass->GetPathTracingSceneTexture();
        FTexture* LDR = &SceneTexture.LDRTexture;

        {
            SCOPED_NAMED_EVENT(GraphicsContext, ToneMapping);
            SCOPED_GPU_EVENT(GraphicsDevice, ToneMapping);
            PostProcess->Tonemapping(GraphicsContext, Scene.get(), *HDR, *LDR, Width, Height);
        }

        FTexture& BackBuffer = GraphicsDevice->GetCurrentBackBuffer();

        GraphicsContext->AddResourceBarrier(*LDR, D3D12_RESOURCE_STATE_COPY_SOURCE);
        GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
        GraphicsContext->ExecuteResourceBarriers();
        GraphicsContext->CopyResource(BackBuffer.GetResource(), LDR->GetResource());
    }
}

void FRenderer::RenderShadow(FGraphicsContext* GraphicsContext, FSceneTexture& SceneTexture)
{
    if (Scene->ShadowMethod == (int)EShadowMethod::ShadowMap)
    {
        if (ShadowDepthPass)
        {
            SCOPED_NAMED_EVENT(GraphicsContext, ShadowDepth);
            SCOPED_GPU_EVENT(GraphicsDevice, ShadowDepth)
            ShadowDepthPass->Render(GraphicsContext, Scene.get());
        }
    }
    else if (Scene->ShadowMethod == (int)EShadowMethod::Raytracing)
    {
        if (RaytracingShadowPass)
        {
            SCOPED_NAMED_EVENT(GraphicsContext, RaytracingShadow);
            SCOPED_GPU_EVENT(GraphicsDevice, RaytracingShadow);
            RaytracingShadowPass->AddPass(GraphicsContext, Scene.get(), SceneTexture);
        }
    }
}

void FRenderer::OnWindowResized(uint32_t InWidth, uint32_t InHeight)
{
    GraphicsDevice->OnWindowResized(InWidth, InHeight);

    Width = InWidth;
    Height = InHeight;

    DeferredGPass->OnWindowResized(GraphicsDevice, InWidth, InHeight);
    DebugPass->OnWindowResized(GraphicsDevice, InWidth, InHeight);
    PostProcess->OnWindowResized(GraphicsDevice, InWidth, InHeight);
    TemporalAA->OnWindowResized(GraphicsDevice, InWidth, InHeight);
    ScreenSpaceGI->OnWindowResized(GraphicsDevice, InWidth, InHeight);
    BloomPass->OnWindowResized(GraphicsDevice, InWidth, InHeight);
    SSAOPass->OnWindowResized(GraphicsDevice, InWidth, InHeight);
    Editor->OnWindowResized(Width, Height);
    RaytracingDebugScenePass->OnWindowResized(GraphicsDevice, InWidth, InHeight);

    InitSizeDependantResource(GraphicsDevice, InWidth, InHeight);
}

void FRenderer::InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    InitializeSceneTexture(Device, InWidth, InHeight);
}

void FRenderer::InitializeSceneTexture(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
    FTextureCreationDesc DepthTextureDesc = {
        .Usage = ETextureUsage::DepthStencil,
        .Width = Width,
        .Height = Height,
        .Format = DXGI_FORMAT_D32_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE,
        .Name = L"Depth Texture",
    };
    FTextureCreationDesc PrevDepthTextureDesc = {
        .Usage = ETextureUsage::DepthStencil,
        .Width = Width,
        .Height = Height,
        .Format = DXGI_FORMAT_D32_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE,
        .Name = L"PrevDepth Texture",
    };
    FTextureCreationDesc LDRTextureDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R10G10B10A2_UNORM,
        .InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .Name = L"LDR Texture",
    };

    // TODO : re-use RTV descriptor handle
    FTextureCreationDesc GBufferADesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"GBuffer A",
    };
    FTextureCreationDesc GBufferBDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"GBuffer B",
    };
    FTextureCreationDesc GBufferCDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"GBuffer C",
    };
    FTextureCreationDesc HDRTextureDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"HDR",
    };
    FTextureCreationDesc VelocityTextureDesc{
        .Usage = ETextureUsage::RenderTarget,
        .Width = InWidth,
        .Height = InHeight,
        .Format = DXGI_FORMAT_R16G16_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        .Name = L"Velocity",
    };


    SceneTexture.DepthTexture = Device->CreateTexture(DepthTextureDesc);
    SceneTexture.PrevDepthTexture = Device->CreateTexture(PrevDepthTextureDesc);

    SceneTexture.GBufferA = Device->CreateTexture(GBufferADesc);
    SceneTexture.GBufferB = Device->CreateTexture(GBufferBDesc);
    SceneTexture.GBufferC = Device->CreateTexture(GBufferCDesc);
    SceneTexture.VelocityTexture = Device->CreateTexture(VelocityTextureDesc);

    SceneTexture.LDRTexture = Device->CreateTexture(LDRTextureDesc);
    SceneTexture.HDRTexture = Device->CreateTexture(HDRTextureDesc);

    SceneTexture.Size = { InWidth, InHeight };
}

FTexture* FRenderer::GetDebugVisualizeTexture(FScene* Scene)
{
    std::string Name = Scene->DebugVisualizeList[Scene->DebugVisualizeIndex];

    if (Name.compare("Depth") == 0)
    {
        return &SceneTexture.DepthTexture;
    }
    else if (Name.compare("GBufferA") == 0)
    {
        return &SceneTexture.GBufferA;
    }
    else if (Name.compare("GBufferB") == 0)
    {
        return &SceneTexture.GBufferB;
    }
    else if (Name.compare("GBufferC") == 0)
    {
        return &SceneTexture.GBufferC;
    }
    else if (Name.compare("HDRTexture") == 0)
    {
        return &SceneTexture.HDRTexture;
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
    else if (Name.compare("SSGIHistroyNumFrameAccumulated") == 0)
    {
        return &ScreenSpaceGI->HistroyNumFrameAccumulated;
    }
    else if (Name.compare("QuarterTexture") == 0)
    {
        return &ScreenSpaceGI->QuarterTexture;
    }
    else if (Name.compare("DenoisedScreenSpaceGITexture") == 0)
    {
        return &ScreenSpaceGI->DenoisedScreenSpaceGITexture;
    }
    else if (Name.compare("DownSampledSceneTexture 1/2") == 0)
    {
        return &BloomPass->DownSampledSceneTextures[0];
    }
    else if (Name.compare("DownSampledSceneTexture 1/4") == 0)
    {
        return &BloomPass->DownSampledSceneTextures[1];
    }
    else if (Name.compare("DownSampledSceneTexture 1/8") == 0)
    {
        return &BloomPass->DownSampledSceneTextures[2];
    }
    else if (Name.compare("DownSampledSceneTexture 1/16") == 0)
    {
        return &BloomPass->DownSampledSceneTextures[3];
    }
    else if (Name.compare("BloomYTexture 1/4") == 0)
    {
        return &BloomPass->BloomYTextures[1];
    }
    else if (Name.compare("BloomYTexture 1/8") == 0)
    {
        return &BloomPass->BloomYTextures[2];
    }
    else if (Name.compare("BloomYTexture 1/16") == 0)
    {
        return &BloomPass->BloomYTextures[3];
    }
    else if (Name.compare("BloomResultTexture") == 0)
    {
        return &BloomPass->BloomResultTexture;
    }
    else if (Name.compare("SSAO Texture") == 0)
    {
        return &SSAOPass->SSAOTexture;
    }
    else if (Name.compare("RaytracingShadowTexture") == 0)
    {
        return &RaytracingShadowPass->GetRaytracingShadowTexture();
    }
    else
    {
        return nullptr;
    }
}
