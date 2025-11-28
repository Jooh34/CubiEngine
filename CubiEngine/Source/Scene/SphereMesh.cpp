#include "Scene/SphereMesh.h"
#include "Graphics/Material.h"
#include "Graphics/D3D12DynamicRHI.h"
#include "Graphics/GraphicsContext.h"

FSphereMesh::FSphereMesh(FMeshCreationDesc MeshCreationDesc)
{
    assert(MeshCreationDesc.BaseColorValue.x >= 0.f);
    assert(MeshCreationDesc.EmissiveValue.x >= 0.f);
    assert(MeshCreationDesc.RoughnessValue >= 0.f);
    assert(MeshCreationDesc.MetallicValue >= 0.f);

    const std::wstring Name = std::wstring(MeshCreationDesc.Name);
    Transform.Set(MeshCreationDesc.Rotation, MeshCreationDesc.Scale, MeshCreationDesc.Translate);

    UINT sliceCount = 20;
    UINT stackCount = 20;
    float radius = 0.5f;

    std::vector<XMFLOAT3> Positions;
    std::vector<XMFLOAT3> Normals;
    std::vector<XMFLOAT3> Tangents;
    std::vector<XMFLOAT2> TextureCoords;

    for (UINT stack = 0; stack <= stackCount; ++stack) {
        float theta = stack * M_PI / stackCount;

        for (UINT slice = 0; slice <= sliceCount; ++slice) {
            float phi = slice * 2 * M_PI / sliceCount;

            float x = radius * sinf(theta) * cosf(phi);
            float y = radius * cosf(theta);
            float z = radius * sinf(theta) * sinf(phi);

            XMFLOAT3 pos(x, y, z);
            XMFLOAT3 normal;
            XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&pos)));

            XMFLOAT3 tangent(-sinf(phi), 0.0f, cosf(phi));
            XMFLOAT2 uv(1.0f - (float)slice / sliceCount, (float)stack / stackCount);

            Positions.push_back(pos);
            Normals.push_back(normal);
            Tangents.push_back(tangent);
            TextureCoords.push_back(uv);
        }
    }

    std::vector<UINT> Indice;

    UINT ringVertexCount = sliceCount + 1;
    for (UINT stack = 0; stack < stackCount; ++stack) {
        for (UINT slice = 0; slice < sliceCount; ++slice) {
            UINT a = (stack)*ringVertexCount + slice;
            UINT b = (stack + 1) * ringVertexCount + slice;
            UINT c = (stack + 1) * ringVertexCount + slice + 1;
            UINT d = (stack)*ringVertexCount + slice + 1;

            Indice.push_back(a); Indice.push_back(b); Indice.push_back(c); // CCW
            Indice.push_back(a); Indice.push_back(c); Indice.push_back(d); // CCW
        }
    }

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
