@echo off

if exist External\DirectXAgilitySDK\ (
    echo Already downloaded DirectXAgilitySDK
) else (
    echo Downloading DirectXAgilitySDK ...

    powershell -Command "Invoke-WebRequest -Uri https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.715.0-preview -OutFile agility.zip"
    powershell -Command "& {Expand-Archive agility.zip External/DirectXAgilitySDK}"
)

if exist out\build\x64-Debug\Bin\Debug\D3D12\ (
    echo DirectXAgilitySDK is already copied to the output directory.
) else (
    echo Copying DirectXAgilitySDK to the output directory ...

    xcopy External\DirectXAgilitySDK\build\native\bin\x64\* out\build\x64-Debug\Bin\Debug\D3D12\
    xcopy External\DirectXAgilitySDK\build\native\bin\x64\* out\build\x64-Release\Bin\Release\D3D12\
)

if exist External\DirectXShaderCompiler\ (
    echo Already downloaded DirectXShaderCompiler
) else (
    echo Downloading DirectXShaderCompiler ...

    powershell -Command "Invoke-WebRequest -Uri https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2407/dxc_2024_07_31.zip -OutFile dxc.zip"
    powershell -Command "& {Expand-Archive dxc.zip External/DirectXShaderCompiler}"
)

if exist out\build\x64-Debug\Bin\Debug\ (
    echo DirectXShaderCompiler is already copied to the output directory.
) else (
    echo Copying DirectXShaderCompiler to the output directory ...

    xcopy External\DirectXShaderCompiler\bin\x64\* out\build\x64-Debug\Bin\Debug\
    xcopy External\DirectXShaderCompiler\bin\x64\* out\build\x64-Release\Bin\Release\
)

if exist ThirdParty\PIX\lib\WinPixEventRuntime.lib (
    echo Already downloaded WinPixEventRuntime
) else (
    echo Downloading WinPixEventRuntime ...

    powershell -Command "Invoke-WebRequest -Uri https://www.nuget.org/api/v2/package/WinPixEventRuntime/1.0.240308001 -OutFile pix.zip"
    powershell -Command "& {Expand-Archive pix.zip External/WinPixEventRuntime}"
    if not exist ThirdParty\PIX\lib mkdir ThirdParty\PIX\lib
    copy External\WinPixEventRuntime\bin\x64\WinPixEventRuntime.dll ThirdParty\PIX\lib\
    copy External\WinPixEventRuntime\bin\x64\WinPixEventRuntime.lib ThirdParty\PIX\lib\
)

@REM download and extract the bistro model

if exist Assets\Models\Bistro\ (
    echo Already downloaded Bistro
) else (
    echo Downloading Bistro ...

    powershell -Command "Invoke-WebRequest -Uri https://developer.nvidia.com/bistro -OutFile bistro.zip"
    powershell -Command "& {Expand-Archive bistro.zip Assets/Models/Bistro}"
)