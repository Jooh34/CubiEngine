#pragma once

#include <OpenImageDenoise/oidn.h>
#include "Graphics/Resource.h"

class FGraphicsDevice;

struct FSharedLinearImage
{
	// D3D12-side
	ComPtr<ID3D12Resource> Buffer; // committed buffer (DEFAULT heap, SHARED)
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint{}; // RowPitch/Footprint
	UINT64 TotalBytes = 0; // fp.Footprint.RowPitch * height
	HANDLE SharedHandle = nullptr; // CreateSharedHandle(buffer)

	// OIDN-side
	OIDNBuffer OidnBuffer = nullptr; // oidnNewSharedBufferFromWin32Handle(...)

	void ReleaseInterop()
	{
		if (OidnBuffer) { oidnReleaseBuffer(OidnBuffer); OidnBuffer = nullptr; }
		if (SharedHandle) { CloseHandle(SharedHandle); SharedHandle = nullptr; }
	}
};

struct FrameInterop
{
	ComPtr<ID3D12Fence> Fence;
	UINT64 FenceValue = 0;
	HANDLE FenceEvent = nullptr;
};

class FOIDenoiser
{
public:
	struct FCreateDesc
	{
		ID3D12Device* D3DDevice = nullptr;
		IDXGIAdapter1* DxgiAdapter = nullptr; // used to match GPU by LUID
		bool bHDR = true;
		int Quality = OIDN_QUALITY_FAST; // FAST or BALANCED for real-time
		bool bCleanAux = false; // set true if albedo/normal are prefiltered
	};

	void Initialize(const FCreateDesc& InCreateDesc);
	FSharedLinearImage CreateLinearImageForTexture(ID3D12Resource* SrcTexture);
	void ImportToOidn(FSharedLinearImage& Img);
	void BindImages(FSharedLinearImage& Color, FSharedLinearImage* Albedo, FSharedLinearImage* Normal, FSharedLinearImage& Output, FTexture* SrcColor);
	void Execute();

	void RefreshBuffers(
		ID3D12Resource* TexColor,
		ID3D12Resource* TexAlbedo,
		ID3D12Resource* TexNormal,
		ID3D12Resource* TexDenoisedOut
	);

	void SubmitAndWait(ID3D12CommandQueue* Q, ID3D12Fence* Fence, UINT64& Value, HANDLE FenceEvent);

	void AddPass(
		const FGraphicsDevice* GraphicsDevice,
		FTexture* SrcColor,
		FTexture* SrcAlbedo,
		FTexture* SrcNormal,
		FTexture* DenoiseOutput);

	virtual ~FOIDenoiser()
	{
		Color.ReleaseInterop();
		Output.ReleaseInterop();
		Albedo.ReleaseInterop();
		Normal.ReleaseInterop();

		if (OidnDevice) { oidnReleaseDevice(OidnDevice); OidnDevice = nullptr; }
		if (OidnFilter) { oidnReleaseFilter(OidnFilter); OidnFilter = nullptr; }
	}

	OIDNDevice CreateOIDNDeviceForD3D12(wrl::ComPtr<IDXGIAdapter> Adapter);

	void ExecuteFilter();

	// Public images (created once, reused)
	FSharedLinearImage Color{};
	FSharedLinearImage Output{};
	
	// auxiliary
	FSharedLinearImage Albedo{};
	FSharedLinearImage Normal{};

private:
	FCreateDesc CreateDesc{};
	OIDNDevice OidnDevice = nullptr;
	OIDNFilter OidnFilter = nullptr;

	bool bInitialized = false;
};