#pragma once

#include "Graphics/Context.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

class FGraphicsDevice;

class FComputeContext : public FContext
{
public:
    FComputeContext(FGraphicsDevice* const Device);
    void SetDescriptorHeaps() const;
    void Reset() override;

    void SetComputeRootSignatureAndPipeline(const FPipelineState& PipelineState) const;
    void Set32BitComputeConstants(const void* RenderResources) const;

    void Dispatch(const uint32_t ThreadGroupX, const uint32_t ThreadGroupY, const uint32_t ThreadGroupZ) const;

private:
    FGraphicsDevice* Device;

    static constexpr uint32_t NUMBER_32_BIT_CONSTANTS = 64;
};