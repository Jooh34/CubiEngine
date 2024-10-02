#include "Renderer/Renderer.h"

FRenderer::FRenderer(HWND Handle, int Width, int Height)
{
    GraphicsDevice = std::make_unique<FGraphicsDevice>(
        Width, Height, DXGI_FORMAT_R10G10B10A2_UNORM, Handle);
}

FRenderer::~FRenderer()
{
}

void FRenderer::Render()
{
    std::vector<ID3D12CommandList*> CmdList;

    GraphicsDevice->GetDirectCommandQueue()->Execute(CmdList);
    GraphicsDevice->Prensent();
    GraphicsDevice->EndFrame();
}
