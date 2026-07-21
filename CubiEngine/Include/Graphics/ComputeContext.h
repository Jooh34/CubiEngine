#pragma once

#include "Graphics/Context.h"
#include "Graphics/Resource.h"
#include "Graphics/PipelineState.h"

#include <type_traits>


class FComputeContext : public FContext
{
public:
    FComputeContext();
    void SetDescriptorHeaps() const;
    void Reset() override;

    void SetComputeRootSignatureAndPipeline(const FPipelineState& PipelineState) const;
    template<typename T>
    void Set32BitComputeConstants(const T* RenderResources) const
    {
        static_assert(std::is_trivially_copyable_v<T>, "Root constants must be trivially copyable.");
        static_assert(sizeof(T) % sizeof(uint32_t) == 0u, "Root constants must be DWORD aligned.");
        constexpr UINT Num32BitValues = static_cast<UINT>(sizeof(T) / sizeof(uint32_t));
        static_assert(Num32BitValues <= NUMBER_32_BIT_CONSTANTS, "Root constants exceed the root signature limit.");
        D3D12CommandList->SetComputeRoot32BitConstants(0u, Num32BitValues, RenderResources, 0u);
    }

    void Dispatch(const uint32_t ThreadGroupX, const uint32_t ThreadGroupY, const uint32_t ThreadGroupZ) const;

private:
    static constexpr uint32_t NUMBER_32_BIT_CONSTANTS = 64;
};
