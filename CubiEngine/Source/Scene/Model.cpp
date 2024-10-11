#include "Scene/Model.h"
#include "Core/FileSystem.h"
#include "Graphics/Resource.h"
#include "Graphics/GraphicsDevice.h"
#include "ShaderInterlop/ConstantBuffers.hlsli"

void FTransform::Update()
{
    const XMVECTOR scalingVector = XMLoadFloat3(&Scale);
    const XMVECTOR rotationVector = XMLoadFloat3(&Rotation);
    const XMVECTOR translationVector = XMLoadFloat3(&Translate);

    const XMMATRIX modelMatrix = Dx::XMMatrixScalingFromVector(scalingVector) *
        Dx::XMMatrixRotationRollPitchYawFromVector(rotationVector) *
        Dx::XMMatrixTranslationFromVector(translationVector);

    const interlop::TransformBuffer transformBufferData = {
        .modelMatrix = modelMatrix,
        .inverseModelMatrix = Dx::XMMatrixInverse(nullptr, modelMatrix),
    };

    TransformBuffer.Update(&transformBufferData);

}
FModel::FModel(const FGraphicsDevice* const GraphicsDevice, const FModelCreationDesc& ModelCreationDesc)
    :ModelName(ModelCreationDesc.ModelName)
{
    Transform.TransformBuffer =
        GraphicsDevice->CreateBuffer<interlop::TransformBuffer>(FBufferCreationDesc{
            .Usage = EBufferUsage::ConstantBuffer,
            .Name = ModelName + L" Transform Buffer",
        });

    Transform.Scale = ModelCreationDesc.Scale;
    Transform.Rotation = ModelCreationDesc.Rotation;
    Transform.Translate = ModelCreationDesc.Translate;

    Transform.Update();

    std::string FullPath = FFileSystem::GetAssetPath() + ModelCreationDesc.ModelPath.data();
    
    if (FullPath.find_last_of("/") != std::string::npos)
    {
        ModelDir = FullPath.substr(0, FullPath.find_last_of("/")) + "/";
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

    const tinygltf::Scene& scene = GLTFModel.scenes[GLTFModel.defaultScene];
    for (const int& nodeIndex : scene.nodes)
    {
        LoadNode(GraphicsDevice, ModelCreationDesc, nodeIndex, GLTFModel);
    }
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
            Desc.Usage = ETextureUsage::TextureFromData;

            FTexture texture = GraphicsDevice->CreateTexture(Desc, (std::byte*)data);

            return texture;
        };

    Materials.resize(GLTFModel.materials.size());
    size_t index = 0;

    for (const tinygltf::Material& material : GLTFModel.materials)
    {
        FPBRMaterial PbrMaterial{};
        {
            // Albedo
            if (material.pbrMetallicRoughness.baseColorTexture.index >= 0)
            {
                const tinygltf::Texture& albedoTexture =
                    GLTFModel.textures[material.pbrMetallicRoughness.baseColorTexture.index];
                const tinygltf::Image& albedoImage = GLTFModel.images[albedoTexture.source];

                PbrMaterial.AlbedoTexture =
                    CreateTexture(albedoImage, FTextureCreationDesc{
                                                   .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                                   .MipLevels = 1u, // Todo:Mips
                                                   .Name = ModelName + L" albedo texture",
                        });
                PbrMaterial.AlbedoSampler = Samplers[albedoTexture.sampler];
            }
        }
        {
            // MetalicRoughness
            if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
            {

                const tinygltf::Texture& metalRoughnessTexture =
                    GLTFModel.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index];
                const tinygltf::Image& metalRoughnessImage = GLTFModel.images[metalRoughnessTexture.source];

                PbrMaterial.MetalRoughnessTexture =
                    CreateTexture(metalRoughnessImage, FTextureCreationDesc{
                                                           .Usage = ETextureUsage::TextureFromData,
                                                           .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                           .MipLevels = 1u,
                                                           .Name = ModelName + L" metal roughness texture",
                    });
                PbrMaterial.MetalRoughnessSampler = Samplers[metalRoughnessTexture.sampler];
            }
        }
        {
            // Normal
            if (material.normalTexture.index >= 0)
            {
                const tinygltf::Texture& normalTexture = GLTFModel.textures[material.normalTexture.index];
                const tinygltf::Image& normalImage = GLTFModel.images[normalTexture.source];

                PbrMaterial.NormalTexture =
                    CreateTexture(normalImage, FTextureCreationDesc{
                                               .Usage = ETextureUsage::TextureFromData,
                                               .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                               .MipLevels = 1u,
                                               .Name = ModelName + L" normal texture",
                    });

                PbrMaterial.NormalSampler = Samplers[normalTexture.sampler];
            }
        }
        {
            // Occlusion
            if (material.occlusionTexture.index >= 0)
            {
                const tinygltf::Texture& aoTexture = GLTFModel.textures[material.occlusionTexture.index];
                const tinygltf::Image& aoImage = GLTFModel.images[aoTexture.source];

                PbrMaterial.AOTexture = CreateTexture(aoImage, FTextureCreationDesc{
                                                               .Usage = ETextureUsage::TextureFromData,
                                                               .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                               .MipLevels = 1u,
                                                               .Name = ModelName + L" occlusion texture",
                });
                PbrMaterial.AOSampler = Samplers[aoTexture.sampler];
            }
        }
        {
            // Emissive
            if (material.emissiveTexture.index >= 0)
            {
                const tinygltf::Texture& emissiveTexture = GLTFModel.textures[material.emissiveTexture.index];
                const tinygltf::Image& emissiveImage = GLTFModel.images[emissiveTexture.source];

                PbrMaterial.EmissiveTexture = CreateTexture(emissiveImage, FTextureCreationDesc{
                                                     .Usage = ETextureUsage::TextureFromData,
                                                     .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                                     .MipLevels = 1u,
                                                     .Name = ModelName + L" emissive texture",
                        });
                PbrMaterial.EmissiveSampler = Samplers[emissiveTexture.sampler];
            }
        }

        PbrMaterial.MaterialBuffer = GraphicsDevice->CreateBuffer<interlop::MaterialBuffer>(FBufferCreationDesc{
            .Usage = EBufferUsage::ConstantBuffer,
            .Name = ModelName + L"_MaterialBuffer" + std::to_wstring(index),
        });

        PbrMaterial.MaterialBufferData = {
                .albedoColor = XMFLOAT3 {
                    material.pbrMetallicRoughness.baseColorFactor[0],
                    material.pbrMetallicRoughness.baseColorFactor[1],
                    material.pbrMetallicRoughness.baseColorFactor[2],
                },
                .roughnessFactor = 1.0f,
                .metallicFactor = 1.0f,
                .emissiveFactor = 0.0f,
        };

        PbrMaterial.MaterialBuffer.Update(&PbrMaterial.MaterialBufferData);

        PbrMaterial.MaterialIndex = index;
        Materials[index++] = std::move(PbrMaterial);
    }
}

void FModel::LoadNode(const FGraphicsDevice* const GraphicsDevice, const FModelCreationDesc& ModelCreationDesc,
    const uint32_t NodeIndex, const tinygltf::Model& GLTFModel)
{
    const tinygltf::Node& node = GLTFModel.nodes[NodeIndex];
    if (node.mesh < 0)
    {
        for (const int& child : node.children)
        {
            LoadNode(GraphicsDevice, ModelCreationDesc, child, GLTFModel);
        }
        return;
    }

    const tinygltf::Mesh& nodeMesh = GLTFModel.meshes[node.mesh];
    for (const size_t i : std::views::iota(0u, nodeMesh.primitives.size()))
    {
        tinygltf::Primitive primitive = nodeMesh.primitives[i];

        FMesh Mesh{};
        const std::wstring MeshName = ModelName + L"Mesh " + std::to_wstring(NodeIndex);

        std::vector<XMFLOAT3> Positions{};
        std::vector<XMFLOAT2> TextureCoords{};
        std::vector<XMFLOAT3> Normals{};
        std::vector<uint16_t> Indices{};

        const tinygltf::Model& model = GLTFModel;

        // Position
        size_t accessorNum = 0;
        const uint8_t* positions;
        int positionStride;
        {
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes["POSITION"]];
            const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

            positionStride = accessor.ByteStride(bufferView);
            positions = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);

            accessorNum = accessor.count;
        }

        // TextureCoord
        const uint8_t* texcoords;
        int texcoordStride;
        {
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
            const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

            texcoordStride = accessor.ByteStride(bufferView);
            texcoords = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
        }

        // Normal
        const uint8_t* normals;
        int normalStride;
        {
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes["NORMAL"]];
            const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

            normalStride = accessor.ByteStride(bufferView);
            normals = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
        }

        // Index Buffer
        const uint8_t* indices;
        int indexStride;
        tinygltf::Accessor indexAccessor{};
        {
            indexAccessor = model.accessors[primitive.indices];
            const tinygltf::BufferView& bufferView = model.bufferViews[indexAccessor.bufferView];
            const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

            indexStride = indexAccessor.ByteStride(bufferView);
            indices = reinterpret_cast<const uint8_t*>(&buffer.data[indexAccessor.byteOffset + bufferView.byteOffset]);
        }

        {
            {
                // Fill in the vertices array.
                for (size_t i : std::views::iota(0u, accessorNum))
                {
                    const XMFLOAT3 position = {
                        (reinterpret_cast<float const*>(positions + (i * positionStride)))[0],
                        (reinterpret_cast<float const*>(positions + (i * positionStride)))[1],
                        (reinterpret_cast<float const*>(positions + (i * positionStride)))[2],
                    };

                    const XMFLOAT2 textureCoord = {
                        (reinterpret_cast<float const*>(texcoords + (i * texcoordStride)))[0],
                        (reinterpret_cast<float const*>(texcoords + (i * texcoordStride)))[1],
                    };

                    const XMFLOAT3 normal = {
                        (reinterpret_cast<float const*>(normals + (i * normalStride)))[0],
                        (reinterpret_cast<float const*>(normals + (i * normalStride)))[1],
                        (reinterpret_cast<float const*>(normals + (i * normalStride)))[2],
                    };

                    Positions.emplace_back(position);
                    TextureCoords.emplace_back(textureCoord);
                    Normals.emplace_back(normal);
                }
            }
        }
        
        {
            size_t indexCount = indexAccessor.count;
            Indices.resize(indexCount);
            // Fill indices array.
            for (const size_t i : std::views::iota(0u, indexCount)) {
                Indices[i] = (reinterpret_cast<uint16_t const*>(indices + (i * indexStride))[0]);
            }
        }

        Mesh.PositionBuffer = GraphicsDevice->CreateBuffer<XMFLOAT3>(
            FBufferCreationDesc{
                .Usage = EBufferUsage::StructuredBuffer,
                .Name = MeshName + L" position buffer",
            },
            Positions);

        Mesh.TextureCoordsBuffer = GraphicsDevice->CreateBuffer<XMFLOAT2>(
            FBufferCreationDesc{
                .Usage = EBufferUsage::StructuredBuffer,
                .Name = MeshName + L" texture coord buffer",
            },
            TextureCoords);

        Mesh.NormalBuffer = GraphicsDevice->CreateBuffer<XMFLOAT3>(
            FBufferCreationDesc{
                .Usage = EBufferUsage::StructuredBuffer,
                .Name = MeshName + L" normal buffer",
            },
            Normals);

        Mesh.IndexBuffer = GraphicsDevice->CreateBuffer<uint16_t>(
            FBufferCreationDesc{
                .Usage = EBufferUsage::StructuredBuffer,
                .Name = MeshName + L" index buffer",
            },
            Indices);

        Mesh.IndicesCount = static_cast<uint32_t>(Indices.size());
        Mesh.MaterialIndex = primitive.material;

        Meshes.push_back(Mesh);
    }
    for (const int& ChildIndex : node.children)
    {
        LoadNode(GraphicsDevice, ModelCreationDesc, ChildIndex, GLTFModel);
    }
}

void FModel::Render(const FGraphicsContext* const GraphicsContext,
    interlop::UnlitPassRenderResources& UnlitRenderResources)
{
    for (const FMesh& Mesh : Meshes)
    {
        GraphicsContext->SetIndexBuffer(Mesh.IndexBuffer);

        UnlitRenderResources.albedoTextureIndex = Materials[Mesh.MaterialIndex].AlbedoTexture.SrvIndex;
        UnlitRenderResources.albedoTextureSamplerIndex =
            Materials[Mesh.MaterialIndex].AlbedoSampler.SamplerIndex;

        UnlitRenderResources.materialBufferIndex = Materials[Mesh.MaterialIndex].MaterialBuffer.CbvIndex;

        UnlitRenderResources.positionBufferIndex = Mesh.PositionBuffer.SrvIndex;
        UnlitRenderResources.textureCoordBufferIndex = Mesh.TextureCoordsBuffer.SrvIndex;
        UnlitRenderResources.transformBufferIndex = Transform.TransformBuffer.CbvIndex;
        
        GraphicsContext->SetGraphicsRoot32BitConstants(&UnlitRenderResources);
        GraphicsContext->DrawIndexedInstanced(Mesh.IndicesCount);
    }
}
