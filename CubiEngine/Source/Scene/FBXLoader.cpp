#include "Scene/FBXLoader.h"
#include "Core/FileSystem.h"
#include "Graphics/Resource.h"
#include "Graphics/Material.h"
#include "Graphics/GraphicsDevice.h"
#include <DirectXTex.h>

FFBXLoader::FFBXLoader(const FGraphicsDevice* const GraphicsDevice, const FModelCreationDesc& ModelCreationDesc)
{
    std::string ModelPath = "Assets/" + std::string(ModelCreationDesc.ModelPath.data());

    if (ModelPath.find_last_of("/") != std::string::npos)
    {
        ModelDir = ModelPath.substr(0, ModelPath.find_last_of("/")) + "/";
    }

    Assimp::Importer Importer;

    std::string FullPath = FFileSystem::GetFullPath(ModelPath);
    const aiScene* scene = Importer.ReadFile(FullPath,
        aiProcess_Triangulate |
        //aiProcess_ConvertToLeftHanded |
        aiProcess_FlipUVs |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace
    );

    if (!scene || !scene->HasMeshes())
    {
        throw std::runtime_error("FBX load failed: " + std::string(Importer.GetErrorString()));
    }
    
	LoadMaterials(GraphicsDevice, scene);
	LoadMeshes(GraphicsDevice, scene, ModelCreationDesc);

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

D3D12_TEXTURE_ADDRESS_MODE FFBXLoader::ConvertTextureAddressMode(aiTextureMapMode mode) const
{
    switch (mode)
    {
        case aiTextureMapMode_Wrap:  return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case aiTextureMapMode_Clamp: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case aiTextureMapMode_Mirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        default: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    }
}

void FFBXLoader::LoadMaterials(const FGraphicsDevice* const GraphicsDevice, const aiScene* Scene)
{
    auto LoadTexture = [&](aiMaterial* material, std::string& material_name, aiTextureType type, DXGI_FORMAT format, FTexture& outTexture, FSampler& outSampler)
        {
            if (material->GetTextureCount(type) > 0)
            {
                aiString texPath;
                if (material->GetTexture(type, 0, &texPath) == AI_SUCCESS)
                {
                    FTextureCreationDesc TextureDesc{};
                    TextureDesc.Name = StringToWString(std::string(material_name));
                    TextureDesc.Path = StringToWString(ModelDir + std::string(texPath.C_Str()));
                    TextureDesc.Usage = ETextureUsage::DDSTextureFromPath;
                    TextureDesc.Format = format;
                    TextureDesc.MipLevels = 6;

                    outTexture = GraphicsDevice->CreateTexture(TextureDesc);
                }
                else
                {
                    return AI_FAILURE;
                }

                aiTextureMapMode wrapU = aiTextureMapMode_Wrap;
                aiTextureMapMode wrapV = aiTextureMapMode_Wrap;

                material->Get(AI_MATKEY_MAPPINGMODE_U(type, 0), wrapU);
                material->Get(AI_MATKEY_MAPPINGMODE_V(type, 0), wrapV);

                FSamplerCreationDesc Desc{};
                Desc.SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                Desc.SamplerDesc.AddressU = ConvertTextureAddressMode(wrapU);
                Desc.SamplerDesc.AddressV = ConvertTextureAddressMode(wrapV);
                Desc.SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                Desc.SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
                Desc.SamplerDesc.MinLOD = 0.0f;
                Desc.SamplerDesc.MipLODBias = 0.0f;
                Desc.SamplerDesc.MaxAnisotropy = 1;
                Desc.SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
                outSampler = GraphicsDevice->CreateSampler(Desc);

                return AI_SUCCESS;
            }
            else
            {
                return AI_FAILURE;
            }
        };

	for (uint32_t materialIndex = 0; materialIndex < Scene->mNumMaterials; materialIndex++)
	{
		aiMaterial* material = Scene->mMaterials[materialIndex];

		aiString name;
		material->Get(AI_MATKEY_NAME, name);

		std::shared_ptr<FPBRMaterial> PbrMaterial = std::make_shared<FPBRMaterial>();
        PbrMaterial->Name = name.C_Str();

		std::string AlbedoName = PbrMaterial->Name + " Albedo";
		std::string NormalName = PbrMaterial->Name + " Normal";
		std::string ORMName = PbrMaterial->Name + " ORM";

        auto Result = LoadTexture(material, AlbedoName, aiTextureType_BASE_COLOR, DXGI_FORMAT_UNKNOWN, PbrMaterial->AlbedoTexture, PbrMaterial->AlbedoSampler);
        if (Result == AI_FAILURE)
        {
            LoadTexture(material, AlbedoName, aiTextureType_DIFFUSE, DXGI_FORMAT_UNKNOWN, PbrMaterial->AlbedoTexture, PbrMaterial->AlbedoSampler);
        }
        LoadTexture(material, NormalName, aiTextureType_NORMALS, DXGI_FORMAT_UNKNOWN, PbrMaterial->NormalTexture, PbrMaterial->NormalSampler);
        LoadTexture(material, ORMName, aiTextureType_SPECULAR, DXGI_FORMAT_UNKNOWN, PbrMaterial->ORMTexture, PbrMaterial->ORMSampler);


        PbrMaterial->MaterialBuffer = GraphicsDevice->CreateBuffer<interlop::MaterialBuffer>(FBufferCreationDesc{
            .Usage = EBufferUsage::ConstantBuffer,
            .Name = StringToWString(PbrMaterial->Name) + L"_MaterialBuffer",
		});

        aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f); 
        if (material->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS)
        {
            PbrMaterial->MaterialBufferData = {
				.albedoColor = XMFLOAT3 { baseColor.r, baseColor.g, baseColor.b },
                .roughnessFactor = 1.0f,
                .metallicFactor = 1.0f,
            };
        }

        PbrMaterial->MaterialBuffer.Update(&PbrMaterial->MaterialBufferData);
        PbrMaterial->MaterialIndex = materialIndex;

		Materials.push_back(PbrMaterial);
	}
}

void FFBXLoader::LoadMeshes(const FGraphicsDevice* const GraphicsDevice, const aiScene* Scene, const FModelCreationDesc& ModelCreationDesc)
{
	for (uint32_t meshIndex = 0; meshIndex < Scene->mNumMeshes; meshIndex++)
	{
		aiMesh* mesh = Scene->mMeshes[meshIndex];
        std::string sMeshName = mesh->mName.C_Str();
        std::wstring MeshName = StringToWString(sMeshName);

        std::wcout << MeshName << std::endl;
        std::cout << " : " << mesh->mNumVertices << std::endl;

		std::unique_ptr<FMesh> ResultMesh = std::make_unique<FMesh>();

        std::vector<XMFLOAT3> Positions{};
        std::vector<XMFLOAT2> TextureCoords{};
        std::vector<XMFLOAT3> Normals{};
        std::vector<XMFLOAT3> Tangents{};
		std::vector<UINT> Indice{};

        Positions.reserve(mesh->mNumVertices);
        TextureCoords.reserve(mesh->mNumVertices);
        Normals.reserve(mesh->mNumVertices);
        Tangents.reserve(mesh->mNumVertices);

        if (mesh->HasPositions())
        {
			for (int v = 0; v < mesh->mNumVertices; ++v)
			{
				aiVector3D pos = mesh->mVertices[v];
				//const XMFLOAT3 XMPosition = { pos.x, pos.y, pos.z };
				const XMFLOAT3 XMPosition = { pos.x, pos.z, -pos.y };

				Positions.push_back(XMPosition);
			}
        }

        if (mesh->HasNormals())
        {
			for (int v = 0; v < mesh->mNumVertices; ++v)
			{
				aiVector3D normal = mesh->mNormals[v];
				const XMFLOAT3 XMNormal = { normal.x, normal.y, normal.z };

				Normals.push_back(XMNormal);
			}
        }

        if (mesh->HasTangentsAndBitangents())
        {
			for (int v = 0; v < mesh->mNumVertices; ++v)
			{
				aiVector3D tangent = mesh->mTangents[v];
				const XMFLOAT3 XMTangent = { tangent.x, tangent.y, tangent.z };
				Tangents.push_back(XMTangent);
			}
        }

        if (mesh->HasTextureCoords(0))
        {
            for (int v = 0; v < mesh->mNumVertices; ++v)
            {
                aiVector3D texCoord = mesh->mTextureCoords[0][v];
                const XMFLOAT2 texCoord2D = { texCoord.x, texCoord.y };
                TextureCoords.push_back(texCoord2D);
            }
        }

        // ¿Œµ¶Ω∫ √≥∏Æ
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
        {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned int i = 0; i < face.mNumIndices; ++i)
            {
                unsigned int face_index = face.mIndices[i];
                Indice.push_back(static_cast<UINT>(face_index));
            }
        }

        ResultMesh->IndicesCount = static_cast<uint32_t>(Indice.size());

        ResultMesh->PositionBuffer = GraphicsDevice->CreateBuffer<XMFLOAT3>(
            FBufferCreationDesc{
                .Usage = EBufferUsage::StructuredBuffer,
                .Name = MeshName + L" position buffer",
            },
            Positions);

        ResultMesh->TextureCoordsBuffer = GraphicsDevice->CreateBuffer<XMFLOAT2>(
            FBufferCreationDesc{
                .Usage = EBufferUsage::StructuredBuffer,
                .Name = MeshName + L" texture coord buffer",
            },
            TextureCoords);

        ResultMesh->NormalBuffer = GraphicsDevice->CreateBuffer<XMFLOAT3>(
            FBufferCreationDesc{
                .Usage = EBufferUsage::StructuredBuffer,
                .Name = MeshName + L" normal buffer",
            },
            Normals);

        ResultMesh->TangentBuffer = GraphicsDevice->CreateBuffer<XMFLOAT3>( // Add tangent buffer creation
            FBufferCreationDesc{
                .Usage = EBufferUsage::StructuredBuffer,
                .Name = MeshName + L" tangent buffer",
            },
            Tangents);

        ResultMesh->IndexBuffer = GraphicsDevice->CreateBuffer<UINT>(
            FBufferCreationDesc{
                .Usage = EBufferUsage::StructuredBuffer,
                .Name = MeshName + L" index buffer",
            },
            Indice);

        ResultMesh->Material = Materials[mesh->mMaterialIndex];
		ResultMesh->Transform.Set(ModelCreationDesc.Rotation, ModelCreationDesc.Scale, ModelCreationDesc.Translate);

        Meshes.push_back(std::move(ResultMesh));
    }
}
