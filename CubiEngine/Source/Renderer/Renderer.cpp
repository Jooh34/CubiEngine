#include "Renderer/Renderer.h"
#include "Core/Input.h"
#include "Core/Editor.h"
#include "Graphics/Profiler.h"

#include "Renderer/PathTracing.h"

#define GI_METHOD_SSGI 1

#define SCOPED_GPU_EVENT(Device, NAME)\
    GPUProfileScopedObject GPUProfileEvent_##NAME = GPUProfileScopedObject(Device, #NAME);
#define INITIALIZE_RENDER_PASS(TYPE, VAR_NAME)\
    VAR_NAME = std::make_unique<TYPE>(GraphicsDevice, Width, Height); \
    VAR_NAME->Initialize(); \
    RenderPasses.push_back(VAR_NAME.get());
    

FRenderer::FRenderer(FGraphicsDevice* GraphicsDevice, SDL_Window* Window, uint32_t Width, uint32_t Height)
    :Width(Width), Height(Height), GraphicsDevice(GraphicsDevice)
{
    InitSizeDependantResource(GraphicsDevice, Width, Height);
    Scene = std::make_unique<FScene>(GraphicsDevice, Width, Height);
    Editor = std::make_unique<FEditor>(GraphicsDevice, Window, Width, Height);

    INITIALIZE_RENDER_PASS(FDeferredGPass, DeferredGPass);
    INITIALIZE_RENDER_PASS(FDebugPass, DebugPass);
    INITIALIZE_RENDER_PASS(FPostProcess, PostProcess);
    INITIALIZE_RENDER_PASS(FTemporalAAPass, TemporalAA);
    INITIALIZE_RENDER_PASS(FShadowDepthPass, ShadowDepthPass);
    INITIALIZE_RENDER_PASS(FScreenSpaceGIPass, ScreenSpaceGI);
    INITIALIZE_RENDER_PASS(FEyeAdaptationPass, EyeAdaptationPass);
    INITIALIZE_RENDER_PASS(FBloomPass, BloomPass);
    INITIALIZE_RENDER_PASS(FSSAOPass, SSAOPass);

    INITIALIZE_RENDER_PASS(FRaytracingDebugScenePass, RaytracingDebugScenePass);
    INITIALIZE_RENDER_PASS(FRaytracingShadowPass, RaytracingShadowPass);
    INITIALIZE_RENDER_PASS(FPathTracingPass, PathTracingPass);

    INITIALIZE_RENDER_PASS(FDenoisePass, DenoisePass);
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

void FRenderer::BeginFrame(FGraphicsContext* GraphicsContext, FTexture* BackBuffer)
{
    GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
    if (DeferredGPass)
    {
        GraphicsContext->AddResourceBarrier(SceneTexture.GBufferA.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        GraphicsContext->AddResourceBarrier(SceneTexture.GBufferB.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        GraphicsContext->AddResourceBarrier(SceneTexture.GBufferC.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        GraphicsContext->AddResourceBarrier(SceneTexture.VelocityTexture.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        GraphicsContext->AddResourceBarrier(SceneTexture.HDRTexture.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    GraphicsContext->AddResourceBarrier(SceneTexture.DepthTexture.get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
    GraphicsContext->ExecuteResourceBarriers();

    float RenderTargetClearValue[4] = { 0,0,0,1 };
    GraphicsContext->ClearRenderTargetView(BackBuffer, RenderTargetClearValue);
    GraphicsContext->ClearDepthStencilView(SceneTexture.DepthTexture.get());

    GraphicsContext->SetGraphicsRootSignature();
    GraphicsContext->SetComputeRootSignature();
}

void FRenderer::CopyHistoricalTexture(FGraphicsContext* GraphicsContext)
{
    SCOPED_NAMED_EVENT(GraphicsContext, CopyHistoricalTexture);

    // Copy Prev Depth Texture
    GraphicsContext->AddResourceBarrier(SceneTexture.DepthTexture.get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
    GraphicsContext->AddResourceBarrier(SceneTexture.PrevDepthTexture.get(), D3D12_RESOURCE_STATE_COPY_DEST);
    GraphicsContext->ExecuteResourceBarriers();
    GraphicsContext->CopyResource(SceneTexture.PrevDepthTexture->GetResource(), SceneTexture.DepthTexture->GetResource());
}

void FRenderer::Render()
{
    GraphicsDevice->BeginFrame();
    GraphicsDevice->GetGPUProfiler().BeginFrame();

    FGraphicsContext* GraphicsContext = GraphicsDevice->GetCurrentGraphicsContext();
    FTexture* BackBuffer = GraphicsDevice->GetCurrentBackBuffer();

    BeginFrame(GraphicsContext, BackBuffer);

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
    FTexture* BackBuffer = GraphicsDevice->GetCurrentBackBuffer();

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

        FTexture* SSAOTexture = Scene->bUseSSAO ? SSAOPass->SSAOTexture.get() : nullptr;
        DeferredGPass->RenderLightPass(Scene.get(), GraphicsContext,
            ShadowDepthPass.get(), SceneTexture, SSAOTexture, RaytracingShadowPass->GetRaytracingShadowTexture()
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

        FTexture* HDR = SceneTexture.HDRTexture.get();
        FTexture* LDR = SceneTexture.LDRTexture.get();

        if (Scene.get()->bUseTaa)
        {
            SCOPED_NAMED_EVENT(GraphicsContext, TemporalAA);
            SCOPED_GPU_EVENT(GraphicsDevice, TemporalAA);

            TemporalAA->Resolve(GraphicsContext, Scene.get(), SceneTexture);
            HDR = TemporalAA->ResolveTexture.get();

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
                Scene->bUseBloom ? BloomPass->BloomResultTexture.get() : nullptr);
        }

        // PostProcess->Tonemapping(GraphicsContext, Scene.get(), *HDR, Width, Height);

        // ----- Vis Debug -----
        {
            SCOPED_NAMED_EVENT(GraphicsContext, DebugVisualize);
            SCOPED_GPU_EVENT(GraphicsDevice, DebugVisualize);
            FTexture* SelectedTexture = Scene->SelectedDebugTexture;
            if (SelectedTexture != nullptr)
            {
                PostProcess->DebugVisualize(GraphicsContext, Scene.get(), SelectedTexture, LDR, Width, Height);
            }
        }
        // ----- Vis Debug -----

        GraphicsContext->AddResourceBarrier(LDR, D3D12_RESOURCE_STATE_COPY_SOURCE);
        GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
        GraphicsContext->ExecuteResourceBarriers();
        GraphicsContext->CopyResource(BackBuffer->GetResource(), LDR->GetResource());
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

    {
        SCOPED_NAMED_EVENT(GraphicsContext, PostProcess);
        SCOPED_GPU_EVENT(GraphicsDevice, PostProcess);

        FTexture* HDR = RaytracingDebugScenePass->GetRaytracingDebugSceneTexture();
        FTexture* LDR = SceneTexture.LDRTexture.get();

        {
            SCOPED_NAMED_EVENT(GraphicsContext, ToneMapping);
            SCOPED_GPU_EVENT(GraphicsDevice, ToneMapping);
            PostProcess->Tonemapping(GraphicsContext, Scene.get(), HDR, LDR, Width, Height);
        }

        FTexture* BackBuffer = GraphicsDevice->GetCurrentBackBuffer();

        GraphicsContext->AddResourceBarrier(LDR, D3D12_RESOURCE_STATE_COPY_SOURCE);
        GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
        GraphicsContext->ExecuteResourceBarriers();
        GraphicsContext->CopyResource(BackBuffer->GetResource(), LDR->GetResource());
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

    {
        SCOPED_NAMED_EVENT(GraphicsContext, PostProcess);
        SCOPED_GPU_EVENT(GraphicsDevice, PostProcess);

        FTexture* HDR = PathTracingPass->GetPathTracingSceneTexture();
        if (Scene->bEnablePathTracingDenoiser)
        {
			HDR = DenoisePass->AddPass(GraphicsContext, Scene.get(), HDR);
        }

        FTexture* LDR = SceneTexture.LDRTexture.get();
        {
            SCOPED_NAMED_EVENT(GraphicsContext, ToneMapping);
            SCOPED_GPU_EVENT(GraphicsDevice, ToneMapping);
            PostProcess->Tonemapping(GraphicsContext, Scene.get(), HDR, LDR, Width, Height);
        }

        FTexture* BackBuffer = GraphicsDevice->GetCurrentBackBuffer();

        GraphicsContext->AddResourceBarrier(LDR, D3D12_RESOURCE_STATE_COPY_SOURCE);
        GraphicsContext->AddResourceBarrier(BackBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
        GraphicsContext->ExecuteResourceBarriers();
        GraphicsContext->CopyResource(BackBuffer->GetResource(), LDR->GetResource());
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
    Width = InWidth;
    Height = InHeight;
    InitSizeDependantResource(GraphicsDevice, InWidth, InHeight);
    GraphicsDevice->OnWindowResized(InWidth, InHeight);

	for (auto& RenderPass : RenderPasses)
	{
		RenderPass->OnWindowResized(GraphicsDevice, InWidth, InHeight);
	}
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