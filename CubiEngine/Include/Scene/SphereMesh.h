#pragma once

#include "Scene/Mesh.h"
#include "Graphics/GraphicsDevice.h"

class FSphereMesh : public FMesh
{
public:
	FSphereMesh(const FGraphicsDevice* const GraphicsDevice, FMeshCreationDesc MeshCreationDesc);
};
