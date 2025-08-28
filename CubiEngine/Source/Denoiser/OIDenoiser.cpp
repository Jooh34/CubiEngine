#include "Denoiser/OIDenoiser.h"
#include "Graphics/GraphicsDevice.h"

static void OidnCheck(OIDNDevice Dev, const char* Where)
{
	const char* Msg = nullptr;
	auto Err = oidnGetDeviceError(Dev, &Msg);
	if (Err != OIDN_ERROR_NONE)
	{
		char Buf[1024];
		std::snprintf(Buf, sizeof(Buf), "OIDN error at %s: %s", Where ? Where : "?", Msg ? Msg : "unknown");
		OutputDebugStringA(Buf); OutputDebugStringA("\n");
		assert(false);
	}
}

static void GetAdapterLuidBytes(IDXGIAdapter1* Adapter, uint8_t Out[8])
{
	DXGI_ADAPTER_DESC1 Desc{}; Adapter->GetDesc1(&Desc);
	static_assert(sizeof(Desc.AdapterLuid) == 8, "Unexpected LUID size");
	std::memcpy(Out, &Desc.AdapterLuid, 8);
}

void FOIDenoiser::Initialize(const FCreateDesc& InCreateDesc)
{
	if (bInitialized) return;
	bInitialized = true;

	CreateDesc = InCreateDesc;

	if (InCreateDesc.DxgiAdapter)
	{
		uint8_t Luid[8]{}; GetAdapterLuidBytes(InCreateDesc.DxgiAdapter, Luid);
		OidnDevice = oidnNewDeviceByLUID(Luid);
	}
	if (!OidnDevice) OidnDevice = oidnNewDevice(OIDN_DEVICE_TYPE_DEFAULT);
	
	oidnCommitDevice(OidnDevice);
	OidnCheck(OidnDevice, "CommitDevice");

	OidnFilter = oidnNewFilter(OidnDevice, "RT");
	oidnSetFilterBool(OidnFilter, "hdr", InCreateDesc.bHDR);
	oidnSetFilterInt(OidnFilter, "quality", InCreateDesc.Quality);
	if (InCreateDesc.bCleanAux) oidnSetFilterBool(OidnFilter, "cleanAux", true);

	return;
}

FSharedLinearImage FOIDenoiser::CreateLinearImageForTexture(ID3D12Resource* SrcTexture)
{
	FSharedLinearImage Img{};
	const auto TexDesc = SrcTexture->GetDesc();
	UINT64 TotalBytes = 0; UINT NumRows = 0; UINT64 RowSize = 0; D3D12_PLACED_SUBRESOURCE_FOOTPRINT Fp{};
	CreateDesc.D3DDevice->GetCopyableFootprints(&TexDesc, 0, 1, 0, &Fp, &NumRows, &RowSize, &TotalBytes);

	CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC BufDesc = CD3DX12_RESOURCE_DESC::Buffer(TotalBytes, D3D12_RESOURCE_FLAG_NONE);

	HRESULT Hr = CreateDesc.D3DDevice->CreateCommittedResource(
		&HeapProps,
		D3D12_HEAP_FLAG_SHARED,
		&BufDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&Img.Buffer));

	assert(SUCCEEDED(Hr));

	Img.Footprint = Fp; Img.TotalBytes = TotalBytes;
	Hr = CreateDesc.D3DDevice->CreateSharedHandle(Img.Buffer.Get(), nullptr, GENERIC_ALL, nullptr, &Img.SharedHandle);
	assert(SUCCEEDED(Hr));

	return Img;
}

void FOIDenoiser::ImportToOidn(FSharedLinearImage& Img)
{
	if (Img.OidnBuffer) return;
	Img.OidnBuffer = oidnNewSharedBufferFromWin32Handle(
		OidnDevice, OIDN_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE,
		(void*)Img.SharedHandle, nullptr, size_t(Img.TotalBytes));
	OidnCheck(OidnDevice, "ImportSharedBuffer");
}

void FOIDenoiser::BindImages(FSharedLinearImage& Color, FSharedLinearImage* Albedo, FSharedLinearImage* Normal, FSharedLinearImage& Output, FTexture* SrcColor)
{
	const size_t W = SrcColor->Width, H = SrcColor->Height;
	const size_t PxStride = 8;
	oidnSetFilterImage(OidnFilter, "color", Color.OidnBuffer, OIDN_FORMAT_HALF3, W, H, 0, PxStride, Color.Footprint.Footprint.RowPitch);
	if (Albedo)
		oidnSetFilterImage(OidnFilter, "albedo", Albedo->OidnBuffer, OIDN_FORMAT_HALF3, W, H, 0, PxStride, Albedo->Footprint.Footprint.RowPitch);
	if (Normal)
		oidnSetFilterImage(OidnFilter, "normal", Normal->OidnBuffer, OIDN_FORMAT_HALF3, W, H, 0, PxStride, Normal->Footprint.Footprint.RowPitch);
	oidnSetFilterImage(OidnFilter, "output", Output.OidnBuffer, OIDN_FORMAT_HALF3, W, H, 0, PxStride, Output.Footprint.Footprint.RowPitch);
	oidnCommitFilter(OidnFilter);
}

void FOIDenoiser::Execute()
{
	oidnExecuteFilter(OidnFilter);
	OidnCheck(OidnDevice, "Execute");
}

void FOIDenoiser::RefreshBuffers(
	ID3D12Resource* TexColor,
	ID3D12Resource* TexAlbedo,
	ID3D12Resource* TexNormal,
	ID3D12Resource* TexDenoisedOut
)
{
	Color.ReleaseInterop();
	if (TexAlbedo) Albedo.ReleaseInterop();
	if (TexNormal) Normal.ReleaseInterop();
	Output.ReleaseInterop();
	

	Color = CreateLinearImageForTexture(TexColor);
	if (TexAlbedo) Albedo = CreateLinearImageForTexture(TexAlbedo);
	if (TexNormal) Normal = CreateLinearImageForTexture(TexNormal);
	Output = CreateLinearImageForTexture(TexDenoisedOut);

	ImportToOidn(Color);
	if (TexAlbedo) ImportToOidn(Albedo);
	if (TexNormal) ImportToOidn(Normal);
	ImportToOidn(Output);
}

void FOIDenoiser::SubmitAndWait(ID3D12CommandQueue* Q, ID3D12Fence* Fence, UINT64& Value, HANDLE FenceEvent)
{
	Q->Signal(Fence, ++Value);
	if (Fence->GetCompletedValue() < Value)
	{
		Fence->SetEventOnCompletion(Value, FenceEvent);
		WaitForSingleObject(FenceEvent, INFINITE);
	}
}

void FOIDenoiser::AddPass(
	const FGraphicsDevice* GraphicsDevice,
	FTexture* SrcColor,
	FTexture* DenoiseOutput
)
{
	FGraphicsContext* GraphicsContext = GraphicsDevice->GetCurrentGraphicsContext();
	GraphicsContext->AddResourceBarrier(SrcColor, D3D12_RESOURCE_STATE_COPY_SOURCE);
	GraphicsContext->ExecuteResourceBarriers();

	D3D12_TEXTURE_COPY_LOCATION SrcLoc(SrcColor->GetResource(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX);
	SrcLoc.SubresourceIndex = 0;
	D3D12_TEXTURE_COPY_LOCATION DstLoc(Color.Buffer.Get(), D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT);
	DstLoc.PlacedFootprint = Color.Footprint;
	GraphicsContext->CopyTextureRegion(&DstLoc, 0, 0, 0, &SrcLoc, nullptr);

	FrameInterop Interop;

	if (!Interop.Fence)
	{
		GraphicsDevice->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Interop.Fence));
		Interop.FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	SubmitAndWait(GraphicsDevice->GetDirectCommandQueue()->GetCommandQueue(), Interop.Fence.Get(), Interop.FenceValue, Interop.FenceEvent);

	BindImages(Color, nullptr, nullptr, Output, SrcColor);
	Execute();

	GraphicsContext = GraphicsDevice->GetCurrentGraphicsContext();
	GraphicsContext->AddResourceBarrier(DenoiseOutput, D3D12_RESOURCE_STATE_COPY_DEST);
	GraphicsContext->ExecuteResourceBarriers();

	D3D12_TEXTURE_COPY_LOCATION SrcLocBack(Output.Buffer.Get(), D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT);
	SrcLocBack.PlacedFootprint = Output.Footprint;
	D3D12_TEXTURE_COPY_LOCATION DstLocBack(DenoiseOutput->GetResource(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX);
	DstLocBack.SubresourceIndex = 0;
	GraphicsContext->CopyTextureRegion(&DstLocBack, 0, 0, 0, &SrcLocBack, nullptr);
}
