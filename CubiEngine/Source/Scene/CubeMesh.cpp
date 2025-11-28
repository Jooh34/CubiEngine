#include "Scene/CubeMesh.h"
#include "Graphics/Material.h"
#include "Graphics/D3D12DynamicRHI.h"
#include "Graphics/GraphicsContext.h"

FCubeMesh::FCubeMesh(FMeshCreationDesc MeshCreationDesc)
{
	assert(MeshCreationDesc.BaseColorValue.x >= 0.f);
	assert(MeshCreationDesc.EmissiveValue.x >= 0.f);
	assert(MeshCreationDesc.RoughnessValue >= 0.f);
	assert(MeshCreationDesc.MetallicValue >= 0.f);

	const std::wstring Name = std::wstring(MeshCreationDesc.Name);
	Transform.Set(MeshCreationDesc.Rotation, MeshCreationDesc.Scale, MeshCreationDesc.Translate);

	std::vector<XMFLOAT3> Positions{
		// Front (-Z)
		{-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
		// Back  (+Z)
		{ 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
		// Top   (+Y)
		{-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
		// Bottom(-Y)
		{-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f},
		// Right (+X)
		{ 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f},
		// Left  (-X)
		{-0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f, 0.5f},
	};

	std::vector<XMFLOAT2> TextureCoords{
		{0,1},{1,1},{1,0},{0,0}, // Front
		{0,1},{1,1},{1,0},{0,0}, // Back
		{0,1},{1,1},{1,0},{0,0}, // Top
		{0,1},{1,1},{1,0},{0,0}, // Bottom
		{0,1},{1,1},{1,0},{0,0}, // Right
		{0,1},{1,1},{1,0},{0,0}, // Left
	};

	std::vector<XMFLOAT3> Normals{
		{ 0, 0,-1},{ 0, 0,-1},{ 0, 0,-1},{ 0, 0,-1},  // Front
		{ 0, 0, 1},{ 0, 0, 1},{ 0, 0, 1},{ 0, 0, 1},  // Back
		{ 0, 1, 0},{ 0, 1, 0},{ 0, 1, 0},{ 0, 1, 0},  // Top
		{ 0,-1, 0},{ 0,-1, 0},{ 0,-1, 0},{ 0,-1, 0},  // Bottom
		{ 1, 0, 0},{ 1, 0, 0},{ 1, 0, 0},{ 1, 0, 0},  // Right
		{-1, 0, 0},{-1, 0, 0},{-1, 0, 0},{-1, 0, 0},  // Left
	};

	std::vector<XMFLOAT3> Tangents{
		{ 1, 0, 0},{ 1, 0, 0},{ 1, 0, 0},{ 1, 0, 0},  // Front
		{-1, 0, 0},{-1, 0, 0},{-1, 0, 0},{-1, 0, 0},  // Back
		{ 1, 0, 0},{ 1, 0, 0},{ 1, 0, 0},{ 1, 0, 0},  // Top
		{ 1, 0, 0},{ 1, 0, 0},{ 1, 0, 0},{ 1, 0, 0},  // Bottom
		{ 0, 0, 1},{ 0, 0, 1},{ 0, 0, 1},{ 0, 0, 1},  // Right
		{ 0, 0,-1},{ 0, 0,-1},{ 0, 0,-1},{ 0, 0,-1},  // Left
	};

	std::vector<UINT> Indice{
		 0, 2, 1,   0, 3, 2,   // Front (-Z)
		 4, 6, 5,   4, 7, 6,   // Back  (+Z)
		 8,10, 9,   8,11,10,   // Top
		12,14,13,  12,15,14,   // Bottom
		16,18,17,  16,19,18,   // Right
		20,22,21,  20,23,22    // Left
	};

	PositionBuffer = RHICreateBuffer<XMFLOAT3>(
		FBufferCreationDesc{
			.Usage = EBufferUsage::StructuredBuffer,
			.Name = Name + L" position buffer",
		},
		Positions);

	TextureCoordsBuffer = RHICreateBuffer<XMFLOAT2>(
		FBufferCreationDesc{
			.Usage = EBufferUsage::StructuredBuffer,
			.Name = Name + L" texture coord buffer",
		},
		TextureCoords);

	NormalBuffer = RHICreateBuffer<XMFLOAT3>(
		FBufferCreationDesc{
			.Usage = EBufferUsage::StructuredBuffer,
			.Name = Name + L" normal buffer",
		},
		Normals);

	TangentBuffer = RHICreateBuffer<XMFLOAT3>( // Add tangent buffer creation
		FBufferCreationDesc{
			.Usage = EBufferUsage::StructuredBuffer,
			.Name = Name + L" tangent buffer",
		},
		Tangents);

	IndexBuffer = RHICreateBuffer<UINT>(
		FBufferCreationDesc{
			.Usage = EBufferUsage::StructuredBuffer,
			.Name = Name + L" index buffer",
		},
		Indice);
	IndicesCount = static_cast<uint32_t>(Indice.size());

	Material = std::make_shared<FPBRMaterial>();
	Material->MaterialBuffer = RHICreateBuffer<interlop::MaterialBuffer>(FBufferCreationDesc{
		.Usage = EBufferUsage::ConstantBuffer,
		.Name = Name + L"_MaterialBuffer",
	});

	Material->MaterialBufferData = {
		.albedoColor = MeshCreationDesc.BaseColorValue,
		.roughnessFactor = MeshCreationDesc.RoughnessValue,
		.metallicFactor = MeshCreationDesc.MetallicValue,
		.emissiveColor = MeshCreationDesc.EmissiveValue,
		.refractionFactor = MeshCreationDesc.RefractionFactor,
		.IOR = MeshCreationDesc.IOR
	};

	Material->MaterialBuffer.Update(&Material->MaterialBufferData);

	FGraphicsContext* GraphicsContext = RHIGetCurrentGraphicsContext();
	GraphicsContext->Reset();
	GenerateRaytracingGeometry();

	// sync gpu immediatly
	RHIGetDirectCommandQueue()->ExecuteContext(GraphicsContext);
	RHIGetDirectCommandQueue()->Flush();
}
