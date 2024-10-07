#include "Scene/Model.h"
#include "Core/FileSystem.h"
#include "Graphics/Resource.h"
#include "Graphics/GraphicsDevice.h"

FModel::FModel(const FGraphicsDevice* const GraphicsDevice, const FModelCreationDesc& ModelCreationDesc)
    :Name(ModelCreationDesc.ModelName)
{
    std::string FullPath = FFileSystem::GetAssetPath() + ModelCreationDesc.ModelPath.data();
    
    if (FullPath.find_last_of("/\\") != std::string::npos)
    {
        ModelDir = FullPath.substr(0, FullPath.find_first_of("/\\")) + "/";
    }


    std::string error{};
    std::string warning{};
    tinygltf::TinyGLTF GLTFContext{};
    tinygltf::Model GLTFModel{};
    

    if (FullPath.find(".glb") != std::string::npos)
    {
        if (!GLTFContext.LoadBinaryFromFile(&GLTFModel, &error, &warning, FullPath))
        {
            if (!error.empty()) FatalError(error);
            if (!warning.empty()) FatalError(warning);
        }
    }
    else
    {
        if (!GLTFContext.LoadASCIIFromFile(&GLTFModel, &error, &warning, FullPath))
        {
            if (!error.empty()) FatalError(error);
            if (!warning.empty()) FatalError(warning);
        }
    }
    
    LoadSamplers(GraphicsDevice, GLTFModel);
    LoadMaterials(GraphicsDevice, GLTFModel);
}

void FModel::LoadSamplers(const FGraphicsDevice* const GraphicsDevice, const tinygltf::Model& GLTFModel)
{
    Samplers.resize(GLTFModel.samplers.size());
    
    size_t index = 0;

    auto ConvertFilter = [](int minFilter, int magFilter) {
        switch (minFilter)
        {
        case TINYGLTF_TEXTURE_FILTER_NEAREST: {
            if (magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST) return D3D12_FILTER_MIN_MAG_MIP_POINT;
            else return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        }
        case TINYGLTF_TEXTURE_FILTER_LINEAR: {
            if (magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST) return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
            else return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        }

        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST: {
            if (magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST) return D3D12_FILTER_MIN_MAG_MIP_POINT;
            else return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        }
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST: {
            if (magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST) return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
            else return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        }
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR: {
            if (magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST) return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            else return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        }
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR: {
            if (magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST) return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            else return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        }

        default: {
            return D3D12_FILTER_ANISOTROPIC;
        }
        }
    };

    auto ConvertTextureAddressMode = [](int Wrap) {
        switch (Wrap)
        {
        case TINYGLTF_TEXTURE_WRAP_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        default: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        }
    };

    for (const tinygltf::Sampler sampler : GLTFModel.samplers)
    {
        FSamplerCreationDesc Desc{};
        Desc.SamplerDesc.Filter = ConvertFilter(sampler.minFilter, sampler.magFilter);
        Desc.SamplerDesc.AddressU = ConvertTextureAddressMode(sampler.wrapS);
        Desc.SamplerDesc.AddressV = ConvertTextureAddressMode(sampler.wrapT);
        Desc.SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        Desc.SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        Desc.SamplerDesc.MinLOD = 0.0f;
        Desc.SamplerDesc.MipLODBias = 0.0f;
        Desc.SamplerDesc.MaxAnisotropy = 16;
        Desc.SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

        Samplers[index++] = GraphicsDevice->CreateSampler(Desc);
    }
}

void FModel::LoadMaterials(const FGraphicsDevice* const GraphicsDevice, const tinygltf::Model& GLTFModel)
{
    const auto CreateTexture = [&](const tinygltf::Image& image, FTextureCreationDesc Desc)
        {
            const std::string TexturePath = ModelDir + image.uri;
            
            int Width, Height;
            const unsigned char* data = stbi_load(TexturePath.c_str(), &Width, &Height, nullptr, 4);
            if (!data)
            {
                FatalError(std::format("Failed to load texture from path : {}", TexturePath));
            }

            // determine max mip levels possible.

            Desc.MipLevels = static_cast<uint32_t>(std::floor(std::log2(max(Width, Height))) + 1);

            Desc.Width = static_cast<uint32_t>(Width);
            Desc.Height = static_cast<uint32_t>(Height);

            FTexture texture = GraphicsDevice->CreateTexture(Desc, (std::byte*)data);

            return texture;
        };

    Materials.resize(GLTFModel.materials.size());
}
