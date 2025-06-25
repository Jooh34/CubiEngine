#pragma once

#include "Scene/Mesh.h"
#include "Graphics/GraphicsDevice.h"

class FCubeMesh : public FMesh
{
public:
	FCubeMesh(const FGraphicsDevice* const GraphicsDevice, FMeshCreationDesc MeshCreationDesc);
};