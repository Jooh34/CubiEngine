#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FUnlitPass : public FRenderPass
{
public:
    FUnlitPass(FGraphicsDevice* Device, const uint32_t Width, const uint32_t Height);
	void InitSizeDependantResource(const FGraphicsDevice* Device, uint32_t InWidth, uint32_t InHeight) override;

    void Render(FScene* const Scene, FGraphicsContext* const GraphicsContext,
        const FTexture& DepthBuffer, uint32_t Width, uint32_t Height);

    FTexture UnlitTexture;
    FPipelineState UnlitPipelineState;
};