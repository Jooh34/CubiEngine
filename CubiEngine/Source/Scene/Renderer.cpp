#include "Scene/Renderer.h"

FRenderer::FRenderer(HWND Handle, int Width, int Height)
{
    GraphicsDevice = std::make_unique<FGraphicsDevice>(
        Width, Height, DXGI_FORMAT_R10G10B10A2_UNORM, Handle);

    Scene = std::make_unique<FScene>(GraphicsDevice.get());
}

FRenderer::~FRenderer()
{
}

void FRenderer::Render()
{
    GraphicsDevice->BeginFrame();

    FGraphicsContext* GraphicsContext = GraphicsDevice->GetCurrentGraphicsContext();
    FTexture& BackBuffer = GraphicsDevice->GetCurrentBackBuffer();

    GraphicsContext->AddResourceBarrier(BackBuffer.Allocation.Resource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    GraphicsContext->ExecuteResourceBarriers();

    RenderTargetColorTest[1] += 0.01f;
    if (RenderTargetColorTest[1] > 1.f) RenderTargetColorTest[1] -= 1.f;

    GraphicsContext->ClearRenderTargetView(BackBuffer, RenderTargetColorTest);

    GraphicsContext->AddResourceBarrier(BackBuffer.Allocation.Resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    GraphicsContext->ExecuteResourceBarriers();

    GraphicsDevice->GetDirectCommandQueue()->ExecuteContext(GraphicsContext);
    GraphicsDevice->Present();
    GraphicsDevice->EndFrame();
}
