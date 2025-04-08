#pragma once

#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"
#include "ShaderInterlop/ConstantBuffers.hlsli"

class FGraphicsDevice;
class FGraphicsContext;
class FScene;

class FShadowDepthPass
{
public:
    FShadowDepthPass(FGraphicsDevice* const Device);
    
    void Render(FGraphicsContext* GraphicsContext, FScene* Scene);
    void AddVSMPass(FGraphicsContext* GraphicsContext, FScene* Scene);

    FTexture& GetShadowDepthTexture() { return ShadowDepthTexture; }
    FTexture& GetMomentTexture() { return MomentTexture; }
    XMMATRIX GetViewProjectionMatrix(int Index) { return ViewProjectionMatrix[Index]; }

private:
    FTexture ShadowDepthTexture;
    FTexture MomentTexture;

    FPipelineState ShadowDepthPassPipelineState;
    FPipelineState MomentPassPipelineState;

    XMMATRIX ViewProjectionMatrix[4];
};