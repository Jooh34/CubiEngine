#include "Resource.h"

void FAllocation::Update(const void* Data, const size_t Size)
{
    if (!Data || !MappedPointer.has_value())
    {
        FatalError("Trying to update resource that is not placed in CPU visible memory, or the data is null");
    }

    memcpy(MappedPointer.value(), Data, Size);
}

void FAllocation::Reset()
{
    Resource.Reset();
    DmaAllocation.Reset();
}

void FBuffer::Update(const void* data)
{
    Allocation.Update(data, SizeInBytes);
}

ID3D12Resource* FTexture::GetResource() const
{
    return Allocation.Resource.Get();
}
