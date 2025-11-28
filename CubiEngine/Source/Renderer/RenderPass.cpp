#include "Renderer/RenderPass.h"

FRenderPass::FRenderPass(uint32_t Width, uint32_t Height)
	: Width(Width), Height(Height)
{
}

void FRenderPass::Initialize()
{
	InitSizeDependantResource(Width, Height);
}

void FRenderPass::OnWindowResized(uint32_t InWidth, uint32_t InHeight)
{
	Width = InWidth;
	Height = InHeight;
	InitSizeDependantResource(InWidth, InHeight);
}
