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

    FTexture& GetShadowDepthTexture() { return ShadowDepthTexture; }

    XMMATRIX GetViewProjectionMatrix(int Index) { return ViewProjectionMatrix[Index]; }

private:
    FTexture ShadowDepthTexture;
    FPipelineState ShadowDepthPassPipelineState;

    XMMATRIX ViewProjectionMatrix[4];
};