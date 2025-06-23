#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FEyeAdaptationPass : public FRenderPass
{
public:
    FEyeAdaptationPass(FGraphicsDevice* Device, const uint32_t Width, const uint32_t Height);
	void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t InWidth, uint32_t InHeight) override;

    void GenerateHistogram(FGraphicsContext* GraphicsContext, FScene* Scene, FTexture* HDR);
    void CalculateAverageLuminance(FGraphicsContext* GraphicsContext, FScene* Scene, uint32_t Width, uint32_t Height);
    void ToneMapping(FGraphicsContext* GraphicsContext, FScene* Scene, FTexture* HDRTexture, FTexture* LDRTexture, FTexture* BloomTexture);

private:
    FPipelineState GenerateHistogramPipelineState;
    FPipelineState CalcuateAverageLuminancePipelineState;
    FPipelineState EyeAdaptationTonemappingPipelineState;

    FBuffer HistogramBuffer;
    FBuffer AverageLuminanceBuffer;
};