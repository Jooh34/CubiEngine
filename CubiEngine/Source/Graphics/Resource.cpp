#include "Graphics/Resource.h"
#include "Graphics/GraphicsDevice.h"

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

bool FTexture::IsCompressedFormat(DXGI_FORMAT Format)
{
    if (Format == DXGI_FORMAT_BC1_UNORM
        || Format == DXGI_FORMAT_BC1_UNORM_SRGB
		|| Format == DXGI_FORMAT_BC2_UNORM
		|| Format == DXGI_FORMAT_BC3_UNORM
		|| Format == DXGI_FORMAT_BC5_UNORM
	)
    {
        return true;
    }

	return false;
}

uint32_t FTexture::GetBytesPerPixel(DXGI_FORMAT Format)
{
    if (Format == DXGI_FORMAT_R32G32B32A32_FLOAT)
	{
		return 16u;
	}
	else if (Format == DXGI_FORMAT_R32G32B32_FLOAT)
	{
		return 12u;
	}
	else if (Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
	{
		return 8u;
	}
	else
	{
		return 4u;
	}
}

bool FTexture::IsUAVAllowed(ETextureUsage Usage, DXGI_FORMAT Format)
{
    if (Usage == ETextureUsage::UAVTexture
        || Usage == ETextureUsage::TextureFromPath
        || Usage == ETextureUsage::TextureFromData
        || Usage == ETextureUsage::HDRTextureFromPath
        || Usage == ETextureUsage::CubeMap
        // for mipmap generation
        || Usage == ETextureUsage::RenderTarget
    )
    {
        if (IsCompressedFormat(Format)) return false;
        else return true;
    }
    else
	{
		return false;
	}
}

FTexture::~FTexture()
{
	std::string TextureName = wStringToString(DebugName);
	FGraphicsDevice::RemoveDebugTexture(TextureName, this);
}
