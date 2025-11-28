#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsContext;
class FScene;

class FUnlitPass : public FRenderPass
{
public:
    FUnlitPass(const uint32_t Width, const uint32_t Height);
	void InitSizeDependantResource(uint32_t InWidth, uint32_t InHeight) override;

    void Render(FScene* const Scene, FGraphicsContext* const GraphicsContext,
        const FTexture* DepthBuffer, uint32_t Width, uint32_t Height);

    std::unique_ptr<FTexture> UnlitTexture;
    FPipelineState UnlitPipelineState;
};