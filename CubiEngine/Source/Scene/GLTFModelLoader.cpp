#include "Scene/GLTFModelLoader.h"
#include "Scene/Scene.h"
#include "Core/FileSystem.h"
#include "Graphics/Resource.h"
#include "Graphics/GraphicsDevice.h"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "Math/CubiMath.h"

FGLTFModelLoader::FGLTFModelLoader(const FGraphicsDevice* const GraphicsDevice, const FModelCreationDesc& ModelCreationDesc)
	:ModelName(ModelCreationDesc.ModelName), ModelCreationDesc(ModelCreationDesc)
{
    OverrideBaseColorValue = ModelCreationDesc.OverrideBaseColorValue;
    OverrideRoughnessValue = ModelCreationDesc.OverrideRoughnessValue;
	OverrideMetallicValue = ModelCreationDesc.OverrideMetallicValue;
	OverrideEmissiveValue = ModelCreationDesc.OverrideEmissiveValue;

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

    FTransform ModelTransform;
    ModelTransform.Set(ModelCreationDesc.Rotation, ModelCreationDesc.Scale, ModelCreationDesc.Translate);

    {
        const std::jthread loadSamplerThread([&]() { LoadSamplers(GraphicsDevice, GLTFModel); });
        const std::jthread loadMaterialThread([&]() { LoadMaterials(GraphicsDevice, GLTFModel); });
    }
    {
        const tinygltf::Scene& scene = GLTFModel.scenes[GLTFModel.defaultScene];
        const std::jthread loadMeshThread([&]() {
            for (const int& nodeIndex : scene.nodes)
            {
                LoadNode(GraphicsDevice, ModelCreationDesc, nodeIndex, GLTFModel, ModelTransform);
            }
        });
    }

    FGraphicsContext* GraphicsContext = GraphicsDevice->GetCurrentGraphicsContext();
    GraphicsContext->Reset();

    for (const auto& Mesh : Meshes)
    {
        Mesh->GenerateRaytracingGeometry(GraphicsDevice);
    }

    // sync gpu immediatly
    GraphicsDevice->GetDirectCommandQueue()->ExecuteContext(GraphicsContext);
    GraphicsDevice->GetDirectCommandQueue()->Flush();
}

void FGLTFModelLoader::LoadSamplers(const FGraphicsDevice* const GraphicsDevice, const tinygltf::Model& GLTFModel)
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

void FGLTFModelLoader::LoadMaterials(const FGraphicsDevice* const GraphicsDevice, const tinygltf::Model& GLTFModel)
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
        
            uint32_t MaxMipLevel = static_cast<uint32_t>(std::floor(std::log2(max(Width, Height))) + 1);
            Desc.MipLevels = max(MaxMipLevel - 5, 1);

            Desc.Width = static_cast<uint32_t>(Width);
            Desc.Height = static_cast<uint32_t>(Height);
            Desc.Usage = ETextureUsage::TextureFromData;

            return GraphicsDevice->CreateTexture(Desc, (std::byte*)data);
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
                const tinygltf::Texture& albedoTexture =
                    GLTFModel.textures[material.pbrMetallicRoughness.baseColorTexture.index];
                const tinygltf::Image& albedoImage = GLTFModel.images[albedoTexture.source];

                PbrMaterial->AlbedoTexture =
                    CreateTexture(albedoImage, FTextureCreationDesc{
                                                   .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                                   .MipLevels = MipLevels,
                                                   .Name = ModelName + L" albedo texture",
                        });
                PbrMaterial->AlbedoSampler = Samplers[albedoTexture.sampler];
            }
        }
        {
            // MetalicRoughness
            if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0 && !bOverrideRoughness && !bOverrideMetallic)
            {

                const tinygltf::Texture& metalRoughnessTexture =
                    GLTFModel.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index];
                const tinygltf::Image& metalRoughnessImage = GLTFModel.images[metalRoughnessTexture.source];

                PbrMaterial->MetalRoughnessTexture =
                    CreateTexture(metalRoughnessImage, FTextureCreationDesc{
                                                           .Usage = ETextureUsage::TextureFromData,
                                                           .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                           .MipLevels = MipLevels,
                                                           .Name = ModelName + L" metal roughness texture",
                    });
                PbrMaterial->MetalRoughnessSampler = Samplers[metalRoughnessTexture.sampler];
            }
        }
        {
            // Normal
            if (material.normalTexture.index >= 0)
            {
                const tinygltf::Texture& normalTexture = GLTFModel.textures[material.normalTexture.index];
                const tinygltf::Image& normalImage = GLTFModel.images[normalTexture.source];

                PbrMaterial->NormalTexture =
                    CreateTexture(normalImage, FTextureCreationDesc{
                                               .Usage = ETextureUsage::TextureFromData,
                                               .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                               .MipLevels = MipLevels,
                                               .Name = ModelName + L" normal texture",
                    });

                PbrMaterial->NormalSampler = Samplers[normalTexture.sampler];
            }
        }
        {
            // Occlusion
            if (material.occlusionTexture.index >= 0)
            {
                const tinygltf::Texture& aoTexture = GLTFModel.textures[material.occlusionTexture.index];
                const tinygltf::Image& aoImage = GLTFModel.images[aoTexture.source];

                PbrMaterial->AOTexture = CreateTexture(aoImage, FTextureCreationDesc{
                                                               .Usage = ETextureUsage::TextureFromData,
                                                               .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                               .MipLevels = MipLevels,
                                                               .Name = ModelName + L" occlusion texture",
                });
                PbrMaterial->AOSampler = Samplers[aoTexture.sampler];
            }
        }
        {
            // Emissive
            if (material.emissiveTexture.index >= 0 && !bOverrideEmissive)
            {
                const tinygltf::Texture& emissiveTexture = GLTFModel.textures[material.emissiveTexture.index];
                const tinygltf::Image& emissiveImage = GLTFModel.images[emissiveTexture.source];

                PbrMaterial->EmissiveTexture = CreateTexture(emissiveImage, FTextureCreationDesc{
                                                     .Usage = ETextureUsage::TextureFromData,
                                                     .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                                     .MipLevels = MipLevels,
                                                     .Name = ModelName + L" emissive texture",
                        });
                PbrMaterial->EmissiveSampler = Samplers[emissiveTexture.sampler];
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

        PbrMaterial->MaterialBuffer = GraphicsDevice->CreateBuffer<interlop::MaterialBuffer>(FBufferCreationDesc{
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
                .emissiveColor = bOverrideEmissive ? OverrideEmissiveValue : XMFLOAT3{0.f, 0.f, 0.f},
                .refractionFactor = ModelCreationDesc.RefractionFactor,
                .IOR = ModelCreationDesc.IOR,
        };

        PbrMaterial->MaterialBuffer.Update(&PbrMaterial->MaterialBufferData);
        PbrMaterial->MaterialIndex = index;
        Materials[index++] = PbrMaterial;
    }
}

void FGLTFModelLoader::LoadNode(const FGraphicsDevice* const GraphicsDevice, const FModelCreationDesc& ModelCreationDesc,
    const uint32_t NodeIndex, const tinygltf::Model& GLTFModel, const FTransform& LocalTransform)
{
    const tinygltf::Node& node = GLTFModel.nodes[NodeIndex];

    XMFLOAT3 NodeRotation = node.rotation.size() == 3 ? XMFLOAT3{ (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2] } : XMFLOAT3{ 0, 0, 0 };
    XMFLOAT3 NodeScale = node.scale.size() == 3 ? XMFLOAT3{ (float)node.scale[0], (float)node.scale[1], (float)node.scale[2] } : XMFLOAT3{ 1, 1, 1 };
    XMFLOAT3 NodeTranslation = node.translation.size() == 3 ? XMFLOAT3{ (float)node.translation[0], (float)node.translation[1], (float)node.translation[2] } : XMFLOAT3{ 0,0,0 };

    FTransform NodeTransform;
    NodeTransform.Set(NodeRotation, NodeScale, NodeTranslation);
    NodeTransform = LocalTransform.Multiply(NodeTransform);

    if (node.mesh >= 0)
    {
		const tinygltf::Mesh& nodeMesh = GLTFModel.meshes[node.mesh];
		for (const size_t i : std::views::iota(0u, nodeMesh.primitives.size()))
		{
			tinygltf::Primitive primitive = nodeMesh.primitives[i];

			std::unique_ptr<FMesh> Mesh = std::make_unique<FMesh>();
			const std::wstring MeshName = ModelName + L"Mesh " + std::to_wstring(NodeIndex);

			std::vector<XMFLOAT3> Positions{};
			std::vector<XMFLOAT2> TextureCoords{};
			std::vector<XMFLOAT3> Normals{};
			std::vector<XMFLOAT3> Tangents{};
			std::vector<UINT> Indice{};


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
                if (positionStride == 0)
                    positionStride = sizeof(float) * 3;  // vec3
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
			const unsigned char* indicePtr;
			int indexStride;
			tinygltf::Accessor indexAccessor{};
			{
				indexAccessor = model.accessors[primitive.indices];
				size_t indexCount = indexAccessor.count;
				const tinygltf::BufferView& bufferView = model.bufferViews[indexAccessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

				indexStride = indexAccessor.ByteStride(bufferView);
				indicePtr = &buffer.data[indexAccessor.byteOffset + bufferView.byteOffset];

				Indice.resize(indexCount);
				if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
				{
					const uint8_t* src = reinterpret_cast<const uint8_t*>(indicePtr);
					for (const size_t i : std::views::iota(0u, indexCount)) {
						Indice[i] = src[i];
					}
				}
				else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					const uint16_t* src = reinterpret_cast<const uint16_t*>(indicePtr);
					for (const size_t i : std::views::iota(0u, indexCount)) {
						Indice[i] = src[i];
					}
				}
				else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					const uint32_t* src = reinterpret_cast<const uint32_t*>(indicePtr);
					for (const size_t i : std::views::iota(0u, indexCount)) {
						Indice[i] = src[i];
					}
				}
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
					
			// Calculate tangents for each vertex
			bool bCalculateTangents = false;
			if (Materials[primitive.material].get() && Materials[primitive.material].get()->NormalTexture)
			{
				bCalculateTangents = true;
			}

			if (bCalculateTangents)
			{
				GenerateTangentVectorList(Tangents, Positions, TextureCoords, Indice);
			}
			else
			{
				GenerateSimpleTangentVectorList(Tangents, Positions, Normals, Indice);
			}

			// Normalize tangents
			for (int i=0; i<Tangents.size(); i++)
			{
				XMFLOAT3& tangent = Tangents[i];
				XMVECTOR tangentVec = XMLoadFloat3(&tangent);
				XMVECTOR normalizedTangentVec = XMVector3Normalize(tangentVec);
				XMStoreFloat3(&tangent, normalizedTangentVec);
			}

			Mesh->PositionBuffer = GraphicsDevice->CreateBuffer<XMFLOAT3>(
				FBufferCreationDesc{
					.Usage = EBufferUsage::StructuredBuffer,
					.Name = MeshName + L" position buffer",
				},
				Positions);

			Mesh->TextureCoordsBuffer = GraphicsDevice->CreateBuffer<XMFLOAT2>(
				FBufferCreationDesc{
					.Usage = EBufferUsage::StructuredBuffer,
					.Name = MeshName + L" texture coord buffer",
				},
				TextureCoords);

			Mesh->NormalBuffer = GraphicsDevice->CreateBuffer<XMFLOAT3>(
				FBufferCreationDesc{
					.Usage = EBufferUsage::StructuredBuffer,
					.Name = MeshName + L" normal buffer",
				},
				Normals);
			
			Mesh->TangentBuffer = GraphicsDevice->CreateBuffer<XMFLOAT3>( // Add tangent buffer creation
				FBufferCreationDesc{
					.Usage = EBufferUsage::StructuredBuffer,
					.Name = MeshName + L" tangent buffer",
				},
				Tangents);

			Mesh->IndexBuffer = GraphicsDevice->CreateBuffer<UINT>(
				FBufferCreationDesc{
					.Usage = EBufferUsage::StructuredBuffer,
					.Name = MeshName + L" index buffer",
				},
				Indice);

			Mesh->IndicesCount = static_cast<uint32_t>(Indice.size());
			Mesh->Material = Materials[primitive.material];
            Mesh->Transform = NodeTransform;

			Meshes.push_back(std::move(Mesh));
		}
    } // node.mesh >= 0

    for (const int& ChildIndex : node.children)
    {
        LoadNode(GraphicsDevice, ModelCreationDesc, ChildIndex, GLTFModel, NodeTransform);
    }
}
