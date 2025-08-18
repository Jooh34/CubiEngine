#pragma once

#include "Renderer/RenderPass.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"
#include "ShaderInterlop/ConstantBuffers.hlsli"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FShadowDepthPass : public FRenderPass
{
public:
    FShadowDepthPass(FGraphicsDevice* const Device, uint32_t Width, uint32_t Height);
    
	void InitSizeDependantResource(const FGraphicsDevice* const Device, uint32_t Width, uint32_t Height) override;

    void Render(FGraphicsContext* GraphicsContext, FScene* Scene);
    void AddVSMPassCS(FGraphicsContext* GraphicsContext, FScene* Scene);

    FTexture* GetShadowDepthTexture() { return ShadowDepthTexture.get(); }
    FTexture* GetMomentTexture() { return MomentTexture.get(); }
    XMMATRIX GetViewProjectionMatrix(int Index) { return ViewProjectionMatrix[Index]; }

private:
    std::unique_ptr<FTexture> ShadowDepthTexture;
    std::unique_ptr<FTexture> MomentTexture;

    FPipelineState ShadowDepthPassPipelineState;
    FPipelineState VSMShadowDepthPassPipelineState;
    FPipelineState MomentPassPipelineState;

    XMMATRIX ViewProjectionMatrix[4];
};