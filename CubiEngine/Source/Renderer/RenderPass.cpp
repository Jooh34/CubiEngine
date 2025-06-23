#include "Renderer/RenderPass.h"

FRenderPass::FRenderPass(const FGraphicsDevice* const Device, uint32_t Width, uint32_t Height)
	: GraphicsDevice(Device), Width(Width), Height(Height)
{
}

void FRenderPass::Initialize()
{
	InitSizeDependantResource(GraphicsDevice, Width, Height);
}

void FRenderPass::OnWindowResized(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight)
{
	Width = InWidth;
	Height = InHeight;
	InitSizeDependantResource(Device, InWidth, InHeight);
}
