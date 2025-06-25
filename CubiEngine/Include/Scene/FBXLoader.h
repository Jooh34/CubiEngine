#pragma once

#include "Graphics/Resource.h"
#include "Scene/Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class FGraphicsDevice;

class FFBXLoader
{
public:
    FFBXLoader(const FGraphicsDevice* const GraphicsDevice, const FModelCreationDesc& ModelCreationDesc);

	void LoadSamplers(const FGraphicsDevice* const GraphicsDevice, const aiScene* Scene);

    D3D12_TEXTURE_ADDRESS_MODE ConvertTextureAddressMode(aiTextureMapMode mode) const;
    void LoadMaterials(const FGraphicsDevice* const GraphicsDevice, const aiScene* Scene);
    void LoadMeshes(const FGraphicsDevice* const GraphicsDevice, const aiScene* Scene, const FModelCreationDesc& ModelCreationDesc);

    std::vector<FSampler> Samplers;
    std::vector<std::shared_ptr<FPBRMaterial>> Materials;

    std::vector<std::unique_ptr<FMesh>> Meshes{};
private:
    std::string ModelDir;
};