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

#include <directxmath.h>
#include <Windows.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <wrl.h>
#include <dxgi1_6.h>

// Custom includes.
#include "Utils.h"
#include "Graphics/d3dx12.h"

// Namespace aliases.
namespace wrl = Microsoft::WRL;
namespace Dx = DirectX;

#ifdef _DEBUG
constexpr bool DEBUG_MODE = true;
#else
constexpr bool DEBUG_MODE = false;
#endif

constexpr uint32_t GNumCbvSrvUavDescriptorHeap = 10000;
constexpr uint32_t GNumRtvDescriptorHeap = 50;
constexpr uint32_t GNumDsvDescriptorHeap = 50;
constexpr uint32_t GNumSamplerDescriptorHeap = 1024;

