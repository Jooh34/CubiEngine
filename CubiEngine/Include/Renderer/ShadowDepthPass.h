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

    XMMATRIX CalculateLightViewProjMatrix(XMVECTOR LightDirection, XMVECTOR Center, float SceneRadius, float NearZ, float FarZ);
    XMMATRIX GetViewProjectionMatrix() { return ViewProjectionMatrix; }

private:
    FTexture ShadowDepthTexture;
    FPipelineState ShadowDepthPassPipelineState;

    XMMATRIX ViewProjectionMatrix{};
};