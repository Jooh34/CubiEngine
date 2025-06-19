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

using wrl::ComPtr;

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
using Dx::XMStoreFloat4;
using Dx::XMVectorSet;
using Dx::XMVector4Transform;
using Dx::XMVectorDivide;
using Dx::XMVectorMin;
using Dx::XMVectorMax;
using Dx::XMVectorGetX;
using Dx::XMVectorGetY;
using Dx::XMVectorGetZ;
using Dx::XMVectorGetW;
using Dx::XMVector3Cross;

#ifdef _DEBUG
#define ENABLE_PIX_EVENT 1
constexpr bool DEBUG_MODE = true;
#else
#define ENABLE_PIX_EVENT 0
constexpr bool DEBUG_MODE = false;
#endif

constexpr uint32_t GNumCbvSrvUavDescriptorHeap = 10000;
constexpr uint32_t GNumRtvDescriptorHeap = 500;
constexpr uint32_t GNumDsvDescriptorHeap = 500;
constexpr uint32_t GNumSamplerDescriptorHeap = 1024;

constexpr uint32_t InitialWidth = 1600;
constexpr uint32_t InitialHeight = 900;

constexpr uint32_t GCubeMapTextureDimension = 1024u;
constexpr uint32_t GPreFilteredCubeMapTextureDimension = 1024u;
constexpr uint32_t GIrradianceCubeMapTextureDimension = 128u;
constexpr uint32_t GBRDFLutTextureDimension = 32u;

inline uint32_t GFrameCount = 0;
static constexpr uint32_t FRAMES_IN_FLIGHT = 3u;

constexpr uint32_t GShadowDepthDimension = 4096u;
constexpr uint32_t GNumCascadeShadowMap = 4;

#define MAX_GAUSSIAN_KERNEL_SIZE 32
#define MAX_SSAO_KERNEL_SIZE 64
