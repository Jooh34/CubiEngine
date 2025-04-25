#pragma once

struct Shader
{
    wrl::ComPtr<IDxcBlob> shaderBlob{};
    wrl::ComPtr<IDxcBlob> rootSignatureBlob{};
};

enum class ShaderTypes : uint8_t
{
    Vertex,
    Pixel,
    Compute,
    Raytracing,
    RootSignature,
};

namespace ShaderCompiler
{
    Shader Compile(const ShaderTypes& shaderType, const std::wstring_view shaderPath,
        const std::wstring_view entryPoint, const bool extractRootSignature = false);
}