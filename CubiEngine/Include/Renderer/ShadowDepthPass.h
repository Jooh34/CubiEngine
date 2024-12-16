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
    FBuffer& GetShadowBuffer() { return ShadowBuffer; }

    XMMATRIX CalculateLightViewProjMatrix(XMVECTOR LightDirection, XMVECTOR Center, float SceneRadius, float NearZ, float FarZ);

private:
    FTexture ShadowDepthTexture;
    FPipelineState ShadowDepthPassPipelineState;

    FBuffer ShadowBuffer;
    interlop::ShadowBuffer ShadowBufferData;
};