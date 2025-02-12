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

DXGI_FORMAT FTexture::ConvertToLinearFormat(const DXGI_FORMAT Format)
{
    switch (Format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: {
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        }
        break;

        case DXGI_FORMAT_BC1_UNORM_SRGB: {
            return DXGI_FORMAT_BC1_UNORM;
        }
                                       break;

        case DXGI_FORMAT_BC2_UNORM_SRGB: {
            return DXGI_FORMAT_BC2_UNORM;
        }
                                       break;

        case DXGI_FORMAT_BC3_UNORM_SRGB: {
            return DXGI_FORMAT_BC3_UNORM;
        }
                                       break;

        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: {
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        }
                                            break;

        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: {
            return DXGI_FORMAT_B8G8R8X8_UNORM;
        }
                                            break;

        case DXGI_FORMAT_BC7_UNORM_SRGB: {
            return DXGI_FORMAT_BC7_UNORM;
        }
                                       break;

        default: {
        return Format;
        }
           break;
    }
}

bool FTexture::IsPowerOfTwo()
{
    bool a = (Width > 0) && (Width & (Width - 1)) == 0;
    bool b = (Height > 0) && (Height & (Height - 1)) == 0;
    return a && b;
}

bool FTexture::IsSRGB(DXGI_FORMAT Format)
{
    if (Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
        || Format == DXGI_FORMAT_BC1_UNORM_SRGB
        || Format == DXGI_FORMAT_BC2_UNORM_SRGB
        || Format == DXGI_FORMAT_BC3_UNORM_SRGB
        || Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
        || Format == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB
        || Format == DXGI_FORMAT_BC7_UNORM_SRGB
        )
    {
        return true;
    }
    return false;
}

bool FTexture::IsDepthFormat()
{
    if (Format == DXGI_FORMAT_D32_FLOAT)
    {
        return true;
    }

    return false;
}
