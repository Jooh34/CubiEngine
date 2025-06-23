# Specify output paths for all configurations.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Bin/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Bin/Debug)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Bin/Release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Bin/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Bin/Release)

# 자동으로 헤더 및 소스 파일 수집
file(GLOB_RECURSE CUBIENGINE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.hpp"
)

file(GLOB_RECURSE CUBIENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
)

# Main.cpp는 직접 추가 (프로젝트 루트에 있으므로)
set(MAIN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp")

# 전체 파일 목록 통합
set(SRC_FILES ${CUBIENGINE_HEADERS} ${CUBIENGINE_SOURCES} ${MAIN_SOURCE})

# 가상 헤더/소스 그룹 구조를 Visual Studio에서 보기 좋게 분류
foreach(header ${CUBIENGINE_HEADERS})
    file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}/Include" "${header}")
    get_filename_component(group_dir "${rel_path}" PATH)
    source_group("Header Files\\${group_dir}" FILES "${header}")
endforeach()

foreach(source ${CUBIENGINE_SOURCES})
    file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}/Source" "${source}")
    get_filename_component(group_dir "${rel_path}" PATH)
    source_group("Source Files\\${group_dir}" FILES "${source}")
endforeach()

add_executable(CubiEngine ${SRC_FILES})

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