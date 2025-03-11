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
    "Source/Core/Input.cpp"
    "Include/Core/Input.h"
    "Source/Core/Editor.cpp"
    "Include/Core/Editor.h"

    "Source/Scene/Model.cpp"
    "Include/Scene/Model.h"
    "Source/Scene/Scene.cpp"
    "Include/Scene/Scene.h"
    "Source/Scene/Camera.cpp"
    "Include/Scene/Camera.h"
    "Source/Scene/Light.cpp"
    "Include/Scene/Light.h"

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
    "Source/Graphics/ComputeContext.cpp"
    "Include/Graphics/ComputeContext.h"
    "Source/Graphics/CopyContext.cpp"
    "Include/Graphics/CopyContext.h"
    "Source/Graphics/ShaderCompiler.cpp"
    "Include/Graphics/ShaderCompiler.h"
    "Source/Graphics/MemoryAllocator.cpp"
    "Include/Graphics/MemoryAllocator.h"
    "Source/Graphics/MipmapGenerator.cpp"
    "Include/Graphics/MipmapGenerator.h"
    "Source/Graphics/Profiler.cpp"
    "Include/Graphics/Profiler.h"
    "Source/Graphics/Query.cpp"
    "Include/Graphics/Query.h"

    "Source/Renderer/Renderer.cpp"
    "Include/Renderer/Renderer.h"
    "Source/Renderer/UnlitPass.cpp"
    "Include/Renderer/UnlitPass.h"
    "Source/Renderer/DeferredGPass.cpp"
    "Include/Renderer/DeferredGPass.h"
    "Source/Renderer/DebugPass.cpp"
    "Include/Renderer/DebugPass.h"
    "Source/Renderer/PostProcess.cpp"
    "Include/Renderer/PostProcess.h"
    "Source/Renderer/CubeMap.cpp"
    "Include/Renderer/CubeMap.h"
    "Source/Renderer/ShadowDepthPass.cpp"
    "Include/Renderer/ShadowDepthPass.h"
    "Source/Renderer/TemporalAA.cpp"
    "Include/Renderer/TemporalAA.h"
    "Source/Renderer/ScreenSpaceGI.cpp"
    "Include/Renderer/ScreenSpaceGI.h"
    "Source/Renderer/EyeAdaptationPass.cpp"
    "Include/Renderer/EyeAdaptationPass.h"
)

add_executable(CubiEngine ${SRC_FILES} "Main.cpp")

target_include_directories(CubiEngine PUBLIC "Include" "../Shaders"
    ${CMAKE_SOURCE_DIR}/ThirdParty/PIX/include
)
target_link_libraries(CubiEngine PUBLIC External d3d12.lib d3dcompiler.lib dxcompiler.lib
    ${CMAKE_SOURCE_DIR}/ThirdParty/PIX/lib/WinPixEventRuntime.lib
)

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

# Copy dll
add_custom_command(
    TARGET CubiEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/ThirdParty/PIX/lib/WinPixEventRuntime.dll
            ${CMAKE_BINARY_DIR}/Bin/$<CONFIG>/WinPixEventRuntime.dll)

set(fileList
    D3D12Core.dll D3D12Core.pdb
    d3d12SDKLayers.dll d3d12SDKLayers.pdb
    d3dconfig.exe d3dconfig.pdb
    DirectSR.dll DirectSR.pdb
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
# Copy dll

add_custom_command(
    TARGET CubiEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/External/DirectXShaderCompiler/lib/x64/dxcompiler.lib
            ${CMAKE_BINARY_DIR}/Bin/$<CONFIG>/dxcompiler.lib)