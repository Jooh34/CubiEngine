#pragma once

#include "Graphics/Resource.h"
#include "Graphics/Material.h"
#include "Graphics/Raytracing.h"
#include "ShaderInterlop/RenderResources.hlsli"
#include "Scene/Mesh.h"
#include "Math/Transform.h"


class FGraphicsContext;
class FScene;

class FGLTFModelLoader
{
public:
    FGLTFModelLoader(const FModelCreationDesc& ModelCreationDesc);

    std::wstring ModelName;
    std::string ModelDir;

    std::vector<FSampler> Samplers;
    std::vector<std::shared_ptr<FPBRMaterial>> Materials;
    
    std::vector<std::unique_ptr<FMesh>> Meshes{};

private:
	FModelCreationDesc ModelCreationDesc;

    void LoadSamplers(const tinygltf::Model& GLTFModel);
    void LoadMaterials(const tinygltf::Model& GLTFModel);
    void LoadNode(const FModelCreationDesc& ModelCreationDesc,
         const uint32_t NodeIndex, const tinygltf::Model& GLTFModel, const FTransform& LocalTransform);

	XMFLOAT3 OverrideBaseColorValue{ -1.0f, -1.0f, -1.0f };
	float OverrideRoughnessValue = -1.0f;
	float OverrideMetallicValue = -1.0f;
	XMFLOAT3 OverrideEmissiveValue{ -1.0f, -1.0f, -1.0f };
};