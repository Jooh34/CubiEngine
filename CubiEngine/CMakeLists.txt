# Specify output paths for all configurations.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Bin/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Bin/Debug)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Bin/Release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Bin/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Bin/Release)

set(SRC_FILES
    "Include/Pch.h"
    "Include/Utils.h"

    "Source/Core/Application.cpp"
    "Include/Core/Application.h"
    "Source/Core/FileSystem.cpp"
    "Include/Core/FileSystem.h"

    "Source/Scene/Model.cpp"
    "Include/Scene/Model.h"
    "Source/Scene/Scene.cpp"
    "Include/Scene/Scene.h"

    "Include/Graphics/d3dx12.h"
    "Include/Graphics/Material.h"

    "Include/Graphics/Resource.cpp"
    "Include/Graphics/Resource.h"
    "Source/Graphics/GraphicsDevice.cpp"
    "Include/Graphics/GraphicsDevice.h"
    "Source/Graphics/DescriptorHeap.cpp"
    "Include/Graphics/DescriptorHeap.h"
    "Source/Graphics/CommandQueue.cpp"
    "Include/Graphics/CommandQueue.h"
    "Source/Graphics/PipelineState.cpp"
    "Include/Graphics/PipelineState.h"
    "Source/Graphics/Context.cpp"
    "Include/Graphics/Context.h"
    "Source/Graphics/GraphicsContext.cpp"
    "Include/Graphics/GraphicsContext.h"
    "Source/Graphics/CopyContext.cpp"
    "Include/Graphics/CopyContext.h"
    "Source/Graphics/ShaderCompiler.cpp"
    "Include/Graphics/ShaderCompiler.h"
    "Source/Graphics/MemoryAllocator.cpp"
    "Include/Graphics/MemoryAllocator.h"

    "Source/Renderer/Renderer.cpp"
    "Include/Renderer/Renderer.h"
    "Source/Renderer/UnlitPass.cpp"
    "Include/Renderer/UnlitPass.h"
)

add_executable(CubiEngine ${SRC_FILES} "Main.cpp")

target_include_directories(CubiEngine PUBLIC "Include" "../Shaders")
target_link_libraries(CubiEngine PUBLIC External d3d12.lib d3dcompiler.lib dxcompiler.lib)

# Enable hot reload in Visual studio 2022.
if (MSVC AND WIN32 AND NOT MSVC_VERSION VERSION_LESS 142)
    target_link_options(CubiEngine PRIVATE $<$<CONFIG:Debug>:/INCREMENTAL>)
    target_compile_options(CubiEngine PRIVATE $<$<CONFIG:Debug>:/Zi>)
endif()

# Setup precompiled headers.
target_precompile_headers(
    CubiEngine
    PUBLIC
    "Include/Pch.h"
)

# Copy dll,exe from DirectXShaderCompiler and DirectXAgilitySDK
set(fileList
    D3D12Core.dll D3D12Core.pdb
    d3d12SDKLayers.dll d3d12SDKLayers.pdb
    d3dconfig.exe d3dconfig.pdb
)

foreach(Var IN LISTS fileList)
    add_custom_command(
        TARGET CubiEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/External/DirectXAgilitySDK/build/native/bin/x64/${Var}
                ${CMAKE_BINARY_DIR}/Bin/$<CONFIG>/D3D12/${Var})
endforeach()

set(fileList
    dxc.exe dxcompiler.dll dxil.dll
)

foreach(Var IN LISTS fileList)
    add_custom_command(
        TARGET CubiEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/External/DirectXShaderCompiler/bin/x64/${Var}
                ${CMAKE_BINARY_DIR}/Bin/$<CONFIG>/${Var})
endforeach()

add_custom_command(
    TARGET CubiEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/External/DirectXShaderCompiler/lib/x64/dxcompiler.lib
            ${CMAKE_BINARY_DIR}/Bin/$<CONFIG>/dxcompiler.lib)