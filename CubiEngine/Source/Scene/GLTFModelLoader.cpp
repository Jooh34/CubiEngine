#include "Scene/GLTFModelLoader.h"
#include "Scene/Scene.h"
#include "Core/FileSystem.h"
#include "Graphics/Resource.h"
#include "Graphics/D3D12DynamicRHI.h"
#include "Graphics/GraphicsContext.h"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "Math/CubiMath.h"

FGLTFModelLoader::FGLTFModelLoader(const FModelCreationDesc& ModelCreationDesc)
	:ModelName(ModelCreationDesc.ModelName), ModelCreationDesc(ModelCreationDesc)
{
    OverrideBaseColorValue = ModelCreationDesc.OverrideBaseColorValue;
    OverrideRoughnessValue = ModelCreationDesc.OverrideRoughnessValue;
	OverrideMetallicValue = ModelCreationDesc.OverrideMetallicValue;
	OverrideEmissiveValue = ModelCreationDesc.OverrideEmissiveValue;

    std::string FullPath = FFileSystem::GetAssetPath() + std::string(ModelCreationDesc.ModelPath);

    if (FullPath.find_last_of("/\\") != std::string::npos)
    {
        ModelDir = FullPath.substr(0, FullPath.find_last_of("/\\")) + "/";
    }

    std::string error{};
    std::string warning{};
    tinygltf::TinyGLTF GLTFContext{};
    tinygltf::Model GLTFModel{};

    bool bLoaded = false;
    if (GetExtension(FullPath) == "glb")
    {
        bLoaded = GLTFContext.LoadBinaryFromFile(&GLTFModel, &error, &warning, FullPath);
    }
    else
    {
        bLoaded = GLTFContext.LoadASCIIFromFile(&GLTFModel, &error, &warning, FullPath);
    }

    if (!warning.empty())
    {
        Log(std::format("glTF warning: {}", warning));
    }
    if (!bLoaded)
    {
        FatalError(error.empty() ? std::format("Failed to load glTF model: {}", FullPath) : error);
    }

    FTransform ModelTransform;
    ModelTransform.Set(ModelCreationDesc.Rotation, ModelCreationDesc.Scale, ModelCreationDesc.Translate);

    // RHI resource creation and the loader's output vectors are not independent;
    // preserve their dependency order instead of racing materials against samplers.
    LoadSamplers(GLTFModel);
    LoadMaterials(GLTFModel);

    if (GLTFModel.scenes.empty())
    {
        FatalError("glTF model contains no scenes.");
    }

    const int SceneIndex = GLTFModel.defaultScene >= 0 ? GLTFModel.defaultScene : 0;
    if (SceneIndex >= static_cast<int>(GLTFModel.scenes.size()))
    {
        FatalError("glTF default scene index is out of range.");
    }

    const tinygltf::Scene& Scene = GLTFModel.scenes[SceneIndex];
    for (const int NodeIndex : Scene.nodes)
    {
        if (NodeIndex < 0)
        {
            FatalError("glTF scene contains an invalid node index.");
        }
        LoadNode(static_cast<uint32_t>(NodeIndex), GLTFModel, ModelTransform);
    }

    FGraphicsContext* GraphicsContext = RHIGetCurrentGraphicsContext();
    GraphicsContext->Reset();

    for (const auto& Mesh : Meshes)
    {
        Mesh->GenerateRaytracingGeometry();
    }

    // sync gpu immediatly
    RHIGetDirectCommandQueue()->ExecuteContext(GraphicsContext);
    RHIGetDirectCommandQueue()->Flush();
}

void FGLTFModelLoader::LoadSamplers(const tinygltf::Model& GLTFModel)
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
            return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
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

    FSamplerCreationDesc DefaultDesc{};
    DefaultDesc.SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    DefaultDesc.SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    DefaultDesc.SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    DefaultDesc.SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    DefaultDesc.SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    DefaultDesc.SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    DefaultSampler = RHICreateSampler(DefaultDesc);

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

        Samplers[index++] = RHICreateSampler(Desc);
    }
}

FSampler FGLTFModelLoader::ResolveSampler(const tinygltf::Texture& Texture) const
{
    if (Texture.sampler < 0)
    {
        return DefaultSampler;
    }
    if (Texture.sampler >= static_cast<int>(Samplers.size()))
    {
        FatalError("glTF texture sampler index is out of range.");
    }
    return Samplers[Texture.sampler];
}

void FGLTFModelLoader::LoadMaterials(const tinygltf::Model& GLTFModel)
{
    const auto GetTexture = [&](int TextureIndex) -> const tinygltf::Texture&
        {
            if (TextureIndex < 0 || TextureIndex >= static_cast<int>(GLTFModel.textures.size()))
            {
                FatalError("glTF material texture index is out of range.");
            }
            return GLTFModel.textures[TextureIndex];
        };

    const auto GetImage = [&](const tinygltf::Texture& Texture) -> const tinygltf::Image&
        {
            if (Texture.source < 0 || Texture.source >= static_cast<int>(GLTFModel.images.size()))
            {
                FatalError("glTF texture image index is out of range.");
            }
            return GLTFModel.images[Texture.source];
        };

    const auto CreateTexture = [&](const tinygltf::Image& Image, FTextureCreationDesc Desc)
        {
            int Width = Image.width;
            int Height = Image.height;
            const uint8_t* TextureData = nullptr;
            std::vector<uint8_t> ConvertedPixels;
            std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> LoadedPixels(nullptr, stbi_image_free);

            if (!Image.image.empty())
            {
                if (Image.bits != 8 || Image.pixel_type != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
                    Image.component < 1 || Image.component > 4)
                {
                    FatalError("Only 8-bit glTF images with one to four components are supported.");
                }
                const size_t RequiredImageBytes = static_cast<size_t>(Width) * Height * Image.component;
                if (Width <= 0 || Height <= 0 || Image.image.size() < RequiredImageBytes)
                {
                    FatalError("glTF image pixel data is truncated.");
                }

                if (Image.component == 4)
                {
                    TextureData = Image.image.data();
                }
                else
                {
                    ConvertedPixels.resize(static_cast<size_t>(Width) * Height * 4u);
                    for (size_t PixelIndex = 0; PixelIndex < static_cast<size_t>(Width) * Height; ++PixelIndex)
                    {
                        const uint8_t* Source = Image.image.data() + PixelIndex * Image.component;
                        uint8_t* Destination = ConvertedPixels.data() + PixelIndex * 4u;
                        if (Image.component == 1)
                        {
                            Destination[0] = Destination[1] = Destination[2] = Source[0];
                            Destination[3] = 255u;
                        }
                        else if (Image.component == 2)
                        {
                            Destination[0] = Destination[1] = Destination[2] = Source[0];
                            Destination[3] = Source[1];
                        }
                        else
                        {
                            Destination[0] = Source[0];
                            Destination[1] = Source[1];
                            Destination[2] = Source[2];
                            Destination[3] = 255u;
                        }
                    }
                    TextureData = ConvertedPixels.data();
                }
            }
            else
            {
                const std::string TexturePath = ModelDir + Image.uri;
                int Components{};
                LoadedPixels.reset(stbi_load(TexturePath.c_str(), &Width, &Height, &Components, 4));
                if (!LoadedPixels)
                {
                    FatalError(std::format("Failed to load texture from path: {}", TexturePath));
                }
                TextureData = LoadedPixels.get();
            }

            if (Width <= 0 || Height <= 0 || !TextureData)
            {
                FatalError("glTF image has invalid dimensions or pixel data.");
            }

            const uint32_t MaxMipLevels = static_cast<uint32_t>(std::floor(std::log2(max(Width, Height)))) + 1u;
            Desc.MipLevels = min(max(Desc.MipLevels, 1u), MaxMipLevels);

            Desc.Width = static_cast<uint32_t>(Width);
            Desc.Height = static_cast<uint32_t>(Height);
            Desc.Usage = ETextureUsage::TextureFromData;

            return RHICreateTexture(Desc, TextureData);
        };

    Materials.resize(GLTFModel.materials.size());
    size_t index = 0;
    
    uint32_t MipLevels = 6u;
    for (const tinygltf::Material& material : GLTFModel.materials)
    {
        bool bOverrideBaseColor = OverrideBaseColorValue.x >= 0.f;
		bool bOverrideRoughness = OverrideRoughnessValue >= 0.f;
		bool bOverrideMetallic = OverrideMetallicValue >= 0.f;
        bool bOverrideEmissive = OverrideEmissiveValue.x >= 0.f;

        std::shared_ptr<FPBRMaterial> PbrMaterial = std::make_shared<FPBRMaterial>();
        {
            // Albedo
            if (material.pbrMetallicRoughness.baseColorTexture.index >= 0 && !bOverrideBaseColor)
            {
                const tinygltf::Texture& albedoTexture = GetTexture(material.pbrMetallicRoughness.baseColorTexture.index);
                const tinygltf::Image& albedoImage = GetImage(albedoTexture);

                PbrMaterial->AlbedoTexture =
                    CreateTexture(albedoImage, FTextureCreationDesc{
                                                   .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                                   .MipLevels = MipLevels,
                                                   .Name = ModelName + L" albedo texture",
                        });
                PbrMaterial->AlbedoSampler = ResolveSampler(albedoTexture);
            }
        }
        {
            // MetalicRoughness
            if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0 && !bOverrideRoughness && !bOverrideMetallic)
            {

                const tinygltf::Texture& metalRoughnessTexture = GetTexture(material.pbrMetallicRoughness.metallicRoughnessTexture.index);
                const tinygltf::Image& metalRoughnessImage = GetImage(metalRoughnessTexture);

                PbrMaterial->MetalRoughnessTexture =
                    CreateTexture(metalRoughnessImage, FTextureCreationDesc{
                                                           .Usage = ETextureUsage::TextureFromData,
                                                           .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                           .MipLevels = MipLevels,
                                                           .Name = ModelName + L" metal roughness texture",
                    });
                PbrMaterial->MetalRoughnessSampler = ResolveSampler(metalRoughnessTexture);
            }
        }
        {
            // Normal
            if (material.normalTexture.index >= 0)
            {
                const tinygltf::Texture& normalTexture = GetTexture(material.normalTexture.index);
                const tinygltf::Image& normalImage = GetImage(normalTexture);

                PbrMaterial->NormalTexture =
                    CreateTexture(normalImage, FTextureCreationDesc{
                                               .Usage = ETextureUsage::TextureFromData,
                                               .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                               .MipLevels = MipLevels,
                                               .Name = ModelName + L" normal texture",
                    });

                PbrMaterial->NormalSampler = ResolveSampler(normalTexture);
            }
        }
        {
            // Occlusion
            if (material.occlusionTexture.index >= 0)
            {
                const tinygltf::Texture& aoTexture = GetTexture(material.occlusionTexture.index);
                const tinygltf::Image& aoImage = GetImage(aoTexture);

                PbrMaterial->AOTexture = CreateTexture(aoImage, FTextureCreationDesc{
                                                               .Usage = ETextureUsage::TextureFromData,
                                                               .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                               .MipLevels = MipLevels,
                                                               .Name = ModelName + L" occlusion texture",
                });
                PbrMaterial->AOSampler = ResolveSampler(aoTexture);
            }
        }
        {
            // Emissive
            if (material.emissiveTexture.index >= 0 && !bOverrideEmissive)
            {
                const tinygltf::Texture& emissiveTexture = GetTexture(material.emissiveTexture.index);
                const tinygltf::Image& emissiveImage = GetImage(emissiveTexture);

                PbrMaterial->EmissiveTexture = CreateTexture(emissiveImage, FTextureCreationDesc{
                                                     .Usage = ETextureUsage::TextureFromData,
                                                     .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                                     .MipLevels = MipLevels,
                                                     .Name = ModelName + L" emissive texture",
                        });
                PbrMaterial->EmissiveSampler = ResolveSampler(emissiveTexture);
            }
        }
        
        if (material.alphaMode.compare("BLEND") == 0)
        {
            PbrMaterial->AlphaMode = EAlphaMode::Blend;
        }
        else if (material.alphaMode.compare("MASK") == 0)
        {
            PbrMaterial->AlphaMode = EAlphaMode::Mask;
        }
        else
        {
            PbrMaterial->AlphaMode = EAlphaMode::Opaque;
        }
        PbrMaterial->AlphaCutoff = material.alphaCutoff;

        PbrMaterial->MaterialBuffer = RHICreateBuffer<interlop::MaterialBuffer>(FBufferCreationDesc{
            .Usage = EBufferUsage::ConstantBuffer,
            .Name = ModelName + L"_MaterialBuffer" + std::to_wstring(index),
        });

        PbrMaterial->MaterialBufferData = {
                .albedoColor = bOverrideBaseColor ? OverrideBaseColorValue :
                XMFLOAT3 {
                    (float)material.pbrMetallicRoughness.baseColorFactor[0],
                    (float)material.pbrMetallicRoughness.baseColorFactor[1],
                    (float)material.pbrMetallicRoughness.baseColorFactor[2],
                },
                .roughnessFactor = bOverrideRoughness ? OverrideRoughnessValue : (float)material.pbrMetallicRoughness.roughnessFactor,
                .metallicFactor = bOverrideMetallic ? OverrideMetallicValue : (float)material.pbrMetallicRoughness.metallicFactor,
                .emissiveColor = bOverrideEmissive ? OverrideEmissiveValue : XMFLOAT3{
                    static_cast<float>(material.emissiveFactor[0]),
                    static_cast<float>(material.emissiveFactor[1]),
                    static_cast<float>(material.emissiveFactor[2])},
                .refractionFactor = ModelCreationDesc.RefractionFactor,
                .IOR = ModelCreationDesc.IOR,
        };

        PbrMaterial->MaterialBuffer.Update(&PbrMaterial->MaterialBufferData);
        PbrMaterial->MaterialIndex = index;
        Materials[index++] = PbrMaterial;
    }

    DefaultMaterial = std::make_shared<FPBRMaterial>();
    DefaultMaterial->MaterialBuffer = RHICreateBuffer<interlop::MaterialBuffer>(FBufferCreationDesc{
        .Usage = EBufferUsage::ConstantBuffer,
        .Name = ModelName + L"_DefaultMaterialBuffer",
    });
    DefaultMaterial->MaterialBufferData = {
        .albedoColor = OverrideBaseColorValue.x >= 0.0f ? OverrideBaseColorValue : XMFLOAT3{ 1.0f, 1.0f, 1.0f },
        .roughnessFactor = OverrideRoughnessValue >= 0.0f ? OverrideRoughnessValue : 1.0f,
        .metallicFactor = OverrideMetallicValue >= 0.0f ? OverrideMetallicValue : 0.0f,
        .emissiveColor = OverrideEmissiveValue.x >= 0.0f ? OverrideEmissiveValue : XMFLOAT3{ 0.0f, 0.0f, 0.0f },
        .refractionFactor = ModelCreationDesc.RefractionFactor,
        .IOR = ModelCreationDesc.IOR,
    };
    DefaultMaterial->MaterialBuffer.Update(&DefaultMaterial->MaterialBufferData);
    DefaultMaterial->MaterialIndex = static_cast<uint32_t>(Materials.size());
    DefaultMaterial->AlphaMode = EAlphaMode::Opaque;
    DefaultMaterial->AlphaCutoff = 0.5;
    Materials.push_back(DefaultMaterial);
}

namespace
{
    struct FAccessorView
    {
        const uint8_t* Data{};
        size_t Count{};
        size_t Stride{};
        int ComponentType{};
        int ComponentCount{};
        bool bNormalized{};
    };

    FAccessorView MakeAccessorView(const tinygltf::Model& Model, int AccessorIndex)
    {
        if (AccessorIndex < 0 || AccessorIndex >= static_cast<int>(Model.accessors.size()))
        {
            FatalError("glTF accessor index is out of range.");
        }

        const tinygltf::Accessor& Accessor = Model.accessors[AccessorIndex];
        if (Accessor.sparse.isSparse)
        {
            FatalError("Sparse glTF accessors are not supported.");
        }
        if (Accessor.bufferView < 0 || Accessor.bufferView >= static_cast<int>(Model.bufferViews.size()))
        {
            FatalError("glTF accessor has an invalid buffer view.");
        }

        const tinygltf::BufferView& BufferView = Model.bufferViews[Accessor.bufferView];
        if (BufferView.buffer < 0 || BufferView.buffer >= static_cast<int>(Model.buffers.size()))
        {
            FatalError("glTF buffer view has an invalid buffer index.");
        }

        const tinygltf::Buffer& Buffer = Model.buffers[BufferView.buffer];
        const int Stride = Accessor.ByteStride(BufferView);
        const int ComponentSize = tinygltf::GetComponentSizeInBytes(static_cast<uint32_t>(Accessor.componentType));
        const int ComponentCount = tinygltf::GetNumComponentsInType(static_cast<uint32_t>(Accessor.type));
        if (Stride <= 0 || ComponentSize <= 0 || ComponentCount <= 0)
        {
            FatalError("glTF accessor has an unsupported component layout.");
        }

        const size_t Offset = BufferView.byteOffset + Accessor.byteOffset;
        const size_t ElementSize = static_cast<size_t>(ComponentSize) * ComponentCount;
        const size_t RequiredSize = Accessor.count == 0u
            ? Offset
            : Offset + (Accessor.count - 1u) * static_cast<size_t>(Stride) + ElementSize;
        if (RequiredSize > Buffer.data.size())
        {
            FatalError("glTF accessor reads beyond its buffer.");
        }

        return {
            .Data = Buffer.data.data() + Offset,
            .Count = Accessor.count,
            .Stride = static_cast<size_t>(Stride),
            .ComponentType = Accessor.componentType,
            .ComponentCount = ComponentCount,
            .bNormalized = Accessor.normalized,
        };
    }

    template<typename T>
    T ReadUnaligned(const uint8_t* Data)
    {
        T Value{};
        std::memcpy(&Value, Data, sizeof(T));
        return Value;
    }

    float ReadFloatComponent(const FAccessorView& View, size_t ElementIndex, int ComponentIndex)
    {
        if (ElementIndex >= View.Count || ComponentIndex < 0 || ComponentIndex >= View.ComponentCount)
        {
            FatalError("glTF accessor component is out of range.");
        }

        const size_t ComponentSize = tinygltf::GetComponentSizeInBytes(static_cast<uint32_t>(View.ComponentType));
        const uint8_t* Data = View.Data + ElementIndex * View.Stride + ComponentIndex * ComponentSize;
        switch (View.ComponentType)
        {
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            return ReadUnaligned<float>(Data);
        case TINYGLTF_COMPONENT_TYPE_BYTE:
        {
            const int8_t Value = ReadUnaligned<int8_t>(Data);
            return View.bNormalized ? max(static_cast<float>(Value) / 127.0f, -1.0f) : static_cast<float>(Value);
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        {
            const uint8_t Value = ReadUnaligned<uint8_t>(Data);
            return View.bNormalized ? static_cast<float>(Value) / 255.0f : static_cast<float>(Value);
        }
        case TINYGLTF_COMPONENT_TYPE_SHORT:
        {
            const int16_t Value = ReadUnaligned<int16_t>(Data);
            return View.bNormalized ? max(static_cast<float>(Value) / 32767.0f, -1.0f) : static_cast<float>(Value);
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        {
            const uint16_t Value = ReadUnaligned<uint16_t>(Data);
            return View.bNormalized ? static_cast<float>(Value) / 65535.0f : static_cast<float>(Value);
        }
        default:
            FatalError("Unsupported glTF vertex component type.");
        }
        return 0.0f;
    }

    uint32_t ReadIndex(const FAccessorView& View, size_t ElementIndex)
    {
        if (ElementIndex >= View.Count || View.ComponentCount != 1)
        {
            FatalError("glTF index accessor is invalid.");
        }

        const uint8_t* Data = View.Data + ElementIndex * View.Stride;
        switch (View.ComponentType)
        {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return ReadUnaligned<uint8_t>(Data);
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return ReadUnaligned<uint16_t>(Data);
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            return ReadUnaligned<uint32_t>(Data);
        default:
            FatalError("Unsupported glTF index component type.");
        }
        return 0u;
    }

    void GenerateVertexNormals(std::vector<XMFLOAT3>& Normals, const std::vector<XMFLOAT3>& Positions,
        const std::vector<UINT>& Indices)
    {
        Normals.assign(Positions.size(), XMFLOAT3{ 0.0f, 0.0f, 0.0f });
        for (size_t Index = 0; Index < Indices.size(); Index += 3u)
        {
            const UINT I0 = Indices[Index];
            const UINT I1 = Indices[Index + 1u];
            const UINT I2 = Indices[Index + 2u];
            const XMVECTOR Edge1 = XMVectorSubtract(XMLoadFloat3(&Positions[I1]), XMLoadFloat3(&Positions[I0]));
            const XMVECTOR Edge2 = XMVectorSubtract(XMLoadFloat3(&Positions[I2]), XMLoadFloat3(&Positions[I0]));
            const XMVECTOR FaceNormal = XMVector3Cross(Edge1, Edge2);

            for (const UINT VertexIndex : { I0, I1, I2 })
            {
                XMVECTOR Normal = XMVectorAdd(XMLoadFloat3(&Normals[VertexIndex]), FaceNormal);
                XMStoreFloat3(&Normals[VertexIndex], Normal);
            }
        }

        for (XMFLOAT3& Normal : Normals)
        {
            XMVECTOR NormalVector = XMLoadFloat3(&Normal);
            if (XMVectorGetX(Dx::XMVector3LengthSq(NormalVector)) <= 1e-12f)
            {
                Normal = { 0.0f, 1.0f, 0.0f };
            }
            else
            {
                XMStoreFloat3(&Normal, XMVector3Normalize(NormalVector));
            }
        }
    }
}

void FGLTFModelLoader::LoadNode(uint32_t NodeIndex, const tinygltf::Model& GLTFModel, const FTransform& ParentTransform)
{
    if (NodeIndex >= GLTFModel.nodes.size())
    {
        FatalError("glTF node index is out of range.");
    }
    const tinygltf::Node& Node = GLTFModel.nodes[NodeIndex];

    FTransform LocalTransform;
    if (!Node.matrix.empty())
    {
        if (Node.matrix.size() != 16u)
        {
            FatalError("glTF node matrix must contain 16 values.");
        }
        // glTF stores column-major matrices for column vectors. Feeding each
        // consecutive column as a DirectX row yields the row-vector equivalent.
        LocalTransform.SetMatrix(XMMATRIX(
            static_cast<float>(Node.matrix[0]), static_cast<float>(Node.matrix[1]), static_cast<float>(Node.matrix[2]), static_cast<float>(Node.matrix[3]),
            static_cast<float>(Node.matrix[4]), static_cast<float>(Node.matrix[5]), static_cast<float>(Node.matrix[6]), static_cast<float>(Node.matrix[7]),
            static_cast<float>(Node.matrix[8]), static_cast<float>(Node.matrix[9]), static_cast<float>(Node.matrix[10]), static_cast<float>(Node.matrix[11]),
            static_cast<float>(Node.matrix[12]), static_cast<float>(Node.matrix[13]), static_cast<float>(Node.matrix[14]), static_cast<float>(Node.matrix[15])));
    }
    else
    {
        if ((!Node.scale.empty() && Node.scale.size() != 3u) ||
            (!Node.translation.empty() && Node.translation.size() != 3u) ||
            (!Node.rotation.empty() && Node.rotation.size() != 4u))
        {
            FatalError("glTF node contains malformed TRS data.");
        }

        const XMFLOAT3 Scale = Node.scale.size() == 3u
            ? XMFLOAT3{ static_cast<float>(Node.scale[0]), static_cast<float>(Node.scale[1]), static_cast<float>(Node.scale[2]) }
            : XMFLOAT3{ 1.0f, 1.0f, 1.0f };
        const XMFLOAT3 Translation = Node.translation.size() == 3u
            ? XMFLOAT3{ static_cast<float>(Node.translation[0]), static_cast<float>(Node.translation[1]), static_cast<float>(Node.translation[2]) }
            : XMFLOAT3{ 0.0f, 0.0f, 0.0f };
        const XMVECTOR Rotation = Node.rotation.size() == 4u
            ? XMVectorSet(static_cast<float>(Node.rotation[0]), static_cast<float>(Node.rotation[1]),
                static_cast<float>(Node.rotation[2]), static_cast<float>(Node.rotation[3]))
            : XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

        LocalTransform.SetMatrix(
            Dx::XMMatrixScalingFromVector(XMLoadFloat3(&Scale)) *
            Dx::XMMatrixRotationQuaternion(Rotation) *
            Dx::XMMatrixTranslationFromVector(XMLoadFloat3(&Translation)));
    }

    // CubiEngine uses row vectors: local transforms precede their parent.
    const FTransform NodeTransform = LocalTransform.Multiply(ParentTransform);

    if (Node.mesh >= 0)
    {
        if (Node.mesh >= static_cast<int>(GLTFModel.meshes.size()))
        {
            FatalError("glTF node mesh index is out of range.");
        }

        const tinygltf::Mesh& NodeMesh = GLTFModel.meshes[Node.mesh];
        for (size_t PrimitiveIndex = 0; PrimitiveIndex < NodeMesh.primitives.size(); ++PrimitiveIndex)
        {
            const tinygltf::Primitive& Primitive = NodeMesh.primitives[PrimitiveIndex];
            if (Primitive.mode != -1 && Primitive.mode != TINYGLTF_MODE_TRIANGLES)
            {
                Log("Skipping non-triangle glTF primitive.");
                continue;
            }

            const auto PositionAttribute = Primitive.attributes.find("POSITION");
            if (PositionAttribute == Primitive.attributes.end())
            {
                FatalError("glTF mesh primitive has no POSITION attribute.");
            }

            const FAccessorView PositionView = MakeAccessorView(GLTFModel, PositionAttribute->second);
            if (PositionView.ComponentCount < 3)
            {
                FatalError("glTF POSITION accessor must have three components.");
            }

            std::vector<XMFLOAT3> Positions(PositionView.Count);
            for (size_t VertexIndex = 0; VertexIndex < PositionView.Count; ++VertexIndex)
            {
                Positions[VertexIndex] = {
                    ReadFloatComponent(PositionView, VertexIndex, 0),
                    ReadFloatComponent(PositionView, VertexIndex, 1),
                    ReadFloatComponent(PositionView, VertexIndex, 2),
                };
            }

            std::vector<UINT> Indices;
            if (Primitive.indices >= 0)
            {
                const FAccessorView IndexView = MakeAccessorView(GLTFModel, Primitive.indices);
                Indices.resize(IndexView.Count);
                for (size_t Index = 0; Index < IndexView.Count; ++Index)
                {
                    Indices[Index] = ReadIndex(IndexView, Index);
                }
            }
            else
            {
                Indices.resize(Positions.size());
                for (size_t Index = 0; Index < Positions.size(); ++Index)
                {
                    Indices[Index] = static_cast<UINT>(Index);
                }
            }

            if (Indices.empty() || Indices.size() % 3u != 0u)
            {
                FatalError("glTF triangle primitive has an invalid index count.");
            }
            for (const UINT Index : Indices)
            {
                if (Index >= Positions.size())
                {
                    FatalError("glTF primitive index exceeds its vertex count.");
                }
            }

            std::vector<XMFLOAT2> TextureCoords(Positions.size(), XMFLOAT2{ 0.0f, 0.0f });
            const auto TexcoordAttribute = Primitive.attributes.find("TEXCOORD_0");
            const bool bHasTextureCoords = TexcoordAttribute != Primitive.attributes.end();
            if (bHasTextureCoords)
            {
                const FAccessorView TexcoordView = MakeAccessorView(GLTFModel, TexcoordAttribute->second);
                if (TexcoordView.Count != Positions.size() || TexcoordView.ComponentCount < 2)
                {
                    FatalError("glTF TEXCOORD_0 accessor does not match POSITION.");
                }
                for (size_t VertexIndex = 0; VertexIndex < TexcoordView.Count; ++VertexIndex)
                {
                    TextureCoords[VertexIndex] = {
                        ReadFloatComponent(TexcoordView, VertexIndex, 0),
                        ReadFloatComponent(TexcoordView, VertexIndex, 1),
                    };
                }
            }

            std::vector<XMFLOAT3> Normals;
            const auto NormalAttribute = Primitive.attributes.find("NORMAL");
            if (NormalAttribute != Primitive.attributes.end())
            {
                const FAccessorView NormalView = MakeAccessorView(GLTFModel, NormalAttribute->second);
                if (NormalView.Count != Positions.size() || NormalView.ComponentCount < 3)
                {
                    FatalError("glTF NORMAL accessor does not match POSITION.");
                }
                Normals.resize(NormalView.Count);
                for (size_t VertexIndex = 0; VertexIndex < NormalView.Count; ++VertexIndex)
                {
                    Normals[VertexIndex] = {
                        ReadFloatComponent(NormalView, VertexIndex, 0),
                        ReadFloatComponent(NormalView, VertexIndex, 1),
                        ReadFloatComponent(NormalView, VertexIndex, 2),
                    };
                    XMVECTOR Normal = XMLoadFloat3(&Normals[VertexIndex]);
                    if (XMVectorGetX(Dx::XMVector3LengthSq(Normal)) > 1e-12f)
                    {
                        XMStoreFloat3(&Normals[VertexIndex], XMVector3Normalize(Normal));
                    }
                }
            }
            else
            {
                GenerateVertexNormals(Normals, Positions, Indices);
            }

            std::shared_ptr<FPBRMaterial> Material = DefaultMaterial;
            if (Primitive.material >= 0)
            {
                if (Primitive.material >= static_cast<int>(GLTFModel.materials.size()))
                {
                    FatalError("glTF primitive material index is out of range.");
                }
                Material = Materials[Primitive.material];
            }

            std::vector<XMFLOAT3> Tangents;
            if (Material->NormalTexture && bHasTextureCoords)
            {
                GenerateTangentVectorList(Tangents, Positions, TextureCoords, Indices);
            }
            else
            {
                GenerateSimpleTangentVectorList(Tangents, Positions, Normals, Indices);
            }

            for (size_t VertexIndex = 0; VertexIndex < Tangents.size(); ++VertexIndex)
            {
                XMVECTOR Tangent = XMLoadFloat3(&Tangents[VertexIndex]);
                if (XMVectorGetX(Dx::XMVector3LengthSq(Tangent)) <= 1e-12f)
                {
                    GenerateSimpleTangentVector(Normals[VertexIndex], &Tangents[VertexIndex]);
                }
                else
                {
                    XMStoreFloat3(&Tangents[VertexIndex], XMVector3Normalize(Tangent));
                }
            }

            std::unique_ptr<FMesh> Mesh = std::make_unique<FMesh>();
            const std::wstring MeshName = ModelName + L" Mesh " + std::to_wstring(NodeIndex) + L":" + std::to_wstring(PrimitiveIndex);
            Mesh->PositionBuffer = RHICreateBuffer<XMFLOAT3>({ .Usage = EBufferUsage::StructuredBuffer, .Name = MeshName + L" position buffer" }, Positions);
            Mesh->TextureCoordsBuffer = RHICreateBuffer<XMFLOAT2>({ .Usage = EBufferUsage::StructuredBuffer, .Name = MeshName + L" texture coord buffer" }, TextureCoords);
            Mesh->NormalBuffer = RHICreateBuffer<XMFLOAT3>({ .Usage = EBufferUsage::StructuredBuffer, .Name = MeshName + L" normal buffer" }, Normals);
            Mesh->TangentBuffer = RHICreateBuffer<XMFLOAT3>({ .Usage = EBufferUsage::StructuredBuffer, .Name = MeshName + L" tangent buffer" }, Tangents);
            Mesh->IndexBuffer = RHICreateBuffer<UINT>({ .Usage = EBufferUsage::StructuredBuffer, .Name = MeshName + L" index buffer" }, Indices);
            Mesh->IndicesCount = static_cast<uint32_t>(Indices.size());
            Mesh->Material = std::move(Material);
            Mesh->Transform = NodeTransform;
            Meshes.push_back(std::move(Mesh));
        }
    }

    for (const int ChildIndex : Node.children)
    {
        if (ChildIndex < 0)
        {
            FatalError("glTF node contains an invalid child index.");
        }
        LoadNode(static_cast<uint32_t>(ChildIndex), GLTFModel, NodeTransform);
    }
}
