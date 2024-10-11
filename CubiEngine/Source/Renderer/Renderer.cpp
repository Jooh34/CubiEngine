#include "Renderer/Renderer.h"

FRenderer::FRenderer(HWND Handle, uint32_t Width, uint32_t Height)
    :Width(Width), Height(Height)
{
    GraphicsDevice = std::make_unique<FGraphicsDevice>(
        Width, Height, DXGI_FORMAT_R10G10B10A2_UNORM, Handle);

    Scene = std::make_unique<FScene>(GraphicsDevice.get(), Width, Height);

    UnlitPass = std::make_unique<FUnlitPass>(GraphicsDevice.get(), Width, Height);

    FTextureCreationDesc DepthTextureDesc = {
        .Usage = ETextureUsage::DepthStencil,
        .Width = Width,
        .Height = Height,
        .Format = DXGI_FORMAT_D32_FLOAT,
        .InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE,
        .Name = L"Depth Texture",
    };

    DepthTexture = GraphicsDevice->CreateTexture(DepthTextureDesc);
}

FRenderer::~FRenderer()
{
}

void FRenderer::Update()
{
    Scene->Update();
}

void FRenderer::Render()
{
    Update();
    GraphicsDevice->BeginFrame();

    FGraphicsContext* GraphicsContext = GraphicsDevice->GetCurrentGraphicsContext();
    FTexture& BackBuffer = GraphicsDevice->GetCurrentBackBuffer();

    GraphicsContext->AddResourceBarrier(BackBuffer.GetResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    GraphicsContext->ExecuteResourceBarriers();

    //RenderTargetColorTest[1] += 0.01f;
    //if (RenderTargetColorTest[1] > 1.f) RenderTargetColorTest[1] -= 1.f;

    GraphicsContext->ClearRenderTargetView(BackBuffer, RenderTargetColorTest);
    GraphicsContext->ClearDepthStencilView(DepthTexture);
    
    GraphicsContext->SetGraphicsRootSignature();

    if (UnlitPass)
    {
        UnlitPass->Render(Scene.get(), GraphicsContext, DepthTexture, Width, Height);

        // Copy to final image
        GraphicsContext->AddResourceBarrier(UnlitPass->UnlitTexture.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
        GraphicsContext->AddResourceBarrier(BackBuffer.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
        GraphicsContext->ExecuteResourceBarriers();

        GraphicsContext->CopyResource(BackBuffer.GetResource(), UnlitPass->UnlitTexture.GetResource());

        GraphicsContext->AddResourceBarrier(UnlitPass->UnlitTexture.GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        GraphicsContext->AddResourceBarrier(BackBuffer.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
        GraphicsContext->ExecuteResourceBarriers();
    }

    //GraphicsContext->ExecuteResourceBarriers();

    GraphicsDevice->GetDirectCommandQueue()->ExecuteContext(GraphicsContext);
    GraphicsDevice->Present();
    GraphicsDevice->EndFrame();
}
