#include "Graphics/Context.h"

void FContext::Reset() const
{
    ThrowIfFailed(CommandAllocator->Reset());
    ThrowIfFailed(CommandList->Reset(CommandAllocator.Get(), nullptr));
}
