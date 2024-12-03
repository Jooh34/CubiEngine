#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <source_location>
#include <format>
#include <array>
#include <filesystem>
#include <ranges>
#include <unordered_map>
#include <string_view>
#include <queue>

#include <directxmath.h>
#include <Windows.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>
#include <tiny_gltf.h>
#include <stb_image.h>

// Custom includes.
#include "Utils.h"
#include "Graphics/d3dx12.h"

// Namespace aliases.
namespace wrl = Microsoft::WRL;
namespace Dx = DirectX;

using Dx::XMFLOAT2;
using Dx::XMFLOAT3;
using Dx::XMFLOAT4;
using Dx::XMMATRIX;
using Dx::XMVECTOR;
using Dx::XMVector3TransformCoord;
using Dx::XMVector3Normalize;
using Dx::XMVectorAdd;
using Dx::XMMatrixLookAtLH;
using Dx::XMMatrixPerspectiveFovLH;
using Dx::XMMatrixRotationRollPitchYaw;
using Dx::XMMatrixInverse;
using Dx::XMVectorScale;
using Dx::XMVectorSubtract;
using Dx::XMLoadFloat4;

#ifdef _DEBUG
constexpr bool DEBUG_MODE = true;
#else
constexpr bool DEBUG_MODE = false;
#endif

constexpr uint32_t GNumCbvSrvUavDescriptorHeap = 10000;
constexpr uint32_t GNumRtvDescriptorHeap = 500;
constexpr uint32_t GNumDsvDescriptorHeap = 500;
constexpr uint32_t GNumSamplerDescriptorHeap = 1024;

constexpr uint32_t InitialWidth = 1200;
constexpr uint32_t InitialHeight = 900;

constexpr uint32_t GCubeMapTextureDimension = 1024u;

