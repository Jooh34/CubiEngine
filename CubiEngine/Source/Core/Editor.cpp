#include "Graphics/D3D12DynamicRHI.h"
#include "Core/Editor.h"
#include "Graphics/GraphicsContext.h"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "Scene/Scene.h"
#include "Core/FileSystem.h"
#include "Renderer/PostProcess.h"
#include "Graphics/TextureManager.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_sdl2.h>
#include <sstream>
#include <iomanip>

void RenderRenderingProperties(FScene* Scene);
void RenderPathTracingProperties(FScene* Scene);

FEditor::FEditor(SDL_Window* Window, uint32_t Width, uint32_t Height)
    :Window(Window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)Width, (float)Height);

    const std::string iniFilePath = FFileSystem::GetFullPath("imgui.ini");
    const auto relativeIniFilePath = std::filesystem::relative(iniFilePath);
    std::string ini = relativeIniFilePath.string();
    io.IniFilename = ini.c_str();

    FDescriptorHeap* DescriptorHeap = RHIGetCbvSrvUavDescriptorHeap();
    FDescriptorHandle FontDescriptorHandle = DescriptorHeap->GetCurrentDescriptorHandle();

    ImGui_ImplSDL2_InitForD3D(Window);
    ImGui_ImplDX12_Init(RHIGetDevice(), FRAMES_IN_FLIGHT,
        RHIGetSwapChainFormat(),
        DescriptorHeap->GetD3D12DescriptorHeap(),
        FontDescriptorHandle.CpuDescriptorHandle,
        FontDescriptorHandle.GpuDescriptorHandle
    );

    DescriptorHeap->OffsetCurrentHandle();
}

FEditor::~FEditor()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void FEditor::GameTick(float DeltaTime, FInput* Input)
{
    if (Input->GetKeyDown(SDL_SCANCODE_X))
    {
		bShowUI = !bShowUI;
    }
}

void FEditor::Render(FGraphicsContext* GraphicsContext, FScene* Scene)
{
    if (!bShowUI) return;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplSDL2_NewFrame(Window);
    ImGui::NewFrame();
    
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Menu"))
        {
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    ImGuiViewport* Viewport = ImGui::GetMainViewport();
    ImVec2 TopRight = ImVec2(Viewport->WorkPos.x + Viewport->WorkSize.x, Viewport->WorkPos.y);
    ImGui::SetNextWindowPos(TopRight, ImGuiCond_Once, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(540, 780), ImGuiCond_Once);
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::Begin("Cubi Editor");

    if (ImGui::BeginTabBar("EditorTabs"))
    {
        if (ImGui::BeginTabItem("Rendering"))
        {
            RenderRenderingProperties(Scene);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Debug"))
        {
            RenderDebugProperties(Scene);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Camera"))
        {
            RenderCameraProperties(Scene);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("GI"))
        {
            RenderGIProperties(Scene);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Lighting"))
        {
            RenderLightProperties(Scene);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Post"))
        {
            RenderPostProcessProperties(Scene);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Path Tracing"))
        {
            RenderPathTracingProperties(Scene);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Profile"))
        {
            RenderProfileProperties(Scene);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GraphicsContext->GetD3D12CommandList());
}

void AddCombo(const char* Name, const char* Items[], int ItemSize, int& Value)
{
    if (ImGui::BeginCombo(Name, Items[Value]))
    {
        for (int i = 0; i < ItemSize; i++)
        {
            bool isSelected = (Value == i);
            if (ImGui::Selectable(Items[i], isSelected))
                Value = i;

            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void AddCombo(const char* Name, const std::vector<std::string> Items, int ItemSize, int& Value)
{
    if (ImGui::BeginCombo(Name, Items[Value].c_str()))
    {
        for (int i = 0; i < ItemSize; i++)
        {
            bool isSelected = (Value == i);
            if (ImGui::Selectable(Items[i].c_str(), isSelected))
                Value = i;

            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

void RenderRenderingProperties(FScene* Scene)
{
    FSceneRenderSettings& Settings = Scene->GetRenderSettings();

    const char* renderingModeItems[] = { "Rasterize", "Debug Raytracing", "PathTrace"};
    AddCombo("Rendering Mode", renderingModeItems, IM_ARRAYSIZE(renderingModeItems), Settings.RenderingMode);

    ImGui::SliderInt("Max FPS", &Settings.MaxFPS, 30, 144);

    ImGui::Checkbox("Enable Diffuse", &Settings.bEnableDiffuse);
    ImGui::Checkbox("Enable Specular", &Settings.bEnableSpecular);
    ImGui::Checkbox("Energy Compensation", &Settings.bUseEnergyCompensation);

    const char* diffuseItems[] = { "Lambertian", "Disney_Burley"};
    AddCombo("Diffuse Model", diffuseItems, IM_ARRAYSIZE(diffuseItems), Settings.DiffuseMethod);

    const char* wfItems[] = { "Off", "Sampling", "IBL", "Albedo only"};
    AddCombo("White Furnace Method", wfItems, IM_ARRAYSIZE(wfItems), Settings.WhiteFurnaceMethod);
}

void RenderPathTracingProperties(FScene* Scene)
{
    FSceneRenderSettings& Settings = Scene->GetRenderSettings();

    ImGui::Checkbox("Enable Denoiser", &Settings.bEnablePathTracingDenoiser);
    ImGui::Checkbox("Use Albedo/Normal", &Settings.bDenoiserAlbedoNormal);
    ImGui::SliderInt("Samples Per Pixel", &Settings.PathTracingSamplePerPixel, 1, 64);
}

void FEditor::RenderDebugProperties(FScene* Scene)
{
    FSceneRenderSettings& Settings = Scene->GetRenderSettings();
    
    FTextureManager* TextureManager = RHIGetTextureManager();
    std::vector<std::string> KeyList = TextureManager->GetDebugTextureKeyList(true);
    AddCombo("Debug Visualize", KeyList, KeyList.size(), Settings.SelectedTextureIndex);
	Settings.SelectedDebugTexture = TextureManager->GetDebugTexture(KeyList[Settings.SelectedTextureIndex]);

    ImGui::SliderFloat("VisDebugMin", &Settings.VisualizeDebugMin, 0.f, 1.f);
    ImGui::SliderFloat("VisDebugMax", &Settings.VisualizeDebugMax, 0.f, 1.f);
}

void FEditor::RenderCameraProperties(FScene* Scene)
{
    FSceneRenderSettings& Settings = Scene->GetRenderSettings();
    
    ImGui::InputFloat3("Camera Position", &Scene->GetCamera().GetCameraPosition().x);
    ImGui::SliderFloat("Fov", &Scene->GetCamera().FovY, 30.0f, 120.0f);
    ImGui::SliderFloat("Far Clip Distance", &Scene->GetCamera().FarZ, 1000.0f, 10000.0f);
}

void FEditor::RenderGIProperties(FScene* Scene)
{
    FSceneRenderSettings& Settings = Scene->GetRenderSettings();

    ImGui::Checkbox("Use SSAO", &Settings.bUseSSAO);
    ImGui::SliderInt("SSAO Kernel Size", &Settings.SSAOKernelSize, 8, 64);
    ImGui::SliderFloat("SSAO Kernel Radius", &Settings.SSAOKernelRadius, 1e-3f, 1e-2f);
    ImGui::InputFloat("SSAO Depth Bias", &Settings.SSAODepthBias, 1e-6, 1e-5, "%.6f");
    ImGui::Checkbox("SSAO Range Check", &Settings.SSAOUseRangeCheck);

    ImGui::Separator();

    const char* giItems[] = { "Off", "SSGI" };
    AddCombo("GI Method", giItems, IM_ARRAYSIZE(giItems), Settings.GIMethod);
    ImGui::SliderFloat("SSGI Intensity", &Settings.SSGIIntensity, 0.0f, 20.0f);
    ImGui::SliderFloat("SSGI RayLength", &Settings.SSGIRayLength, 0.0f, 3.0f);
    ImGui::SliderInt("SSGI NumSteps", &Settings.SSGINumSteps, 1, 256);
    ImGui::SliderFloat("SSGI CompareToleranceScale", &Settings.CompareToleranceScale, 1.f, 30.f);
    ImGui::SliderInt("SSGI NumSamples", &Settings.SSGINumSamples, 1, 16);

    ImGui::SliderInt("SSGI GaussianKernelSize", &Settings.SSGIGaussianKernelSize, 1, 32);
    ImGui::SliderFloat("SSGI GaussianStdDev", &Settings.SSGIGaussianStdDev, 0.1f, 20.0f);

    const char* sampleingMethodItems[] = { "UniformSampleHemisphere", "ImportanceSampleCosDir", "ConcentricSampleDisk", "ConcentricSampleDiskUE5"};
    AddCombo("Sampling Method", sampleingMethodItems, IM_ARRAYSIZE(sampleingMethodItems), Settings.StochasticNormalSamplingMethod);
}

void FEditor::RenderLightProperties(FScene* Scene)
{
    FSceneRenderSettings& Settings = Scene->GetRenderSettings();
    interlop::LightBuffer& LightBuffer = Scene->Light.LightBufferData;

    ImGui::SliderFloat("Envmap Intensity", &Settings.EnvmapIntensity, 0.f, 100.f);

    ImGui::Checkbox("Light Dance Debug", &Settings.bLightDanceDebug);
    ImGui::SliderFloat("Light Dance Speed", &Settings.bLightDanceSpeed, 0.1f, 10.0f);

    if (ImGui::TreeNode("Directional Light"))
    {
        constexpr uint32_t DirectionalLightIndex = 0u;
        
        ImGui::SliderFloat("Intensity", &LightBuffer.intensityDistance[DirectionalLightIndex].x, 0.0f, 1000.0f);
        ImGui::SliderFloat("MaxDistance", &LightBuffer.intensityDistance[DirectionalLightIndex].y, 100.0f, 5000.0f);

        DirectX::XMFLOAT4& Position = LightBuffer.lightPosition[DirectionalLightIndex];

        ImGui::SliderFloat3("Directional Light Directional", &Position.x, -1.0f, 1.0f);

        //
        DirectX::XMFLOAT4& Color = LightBuffer.lightColor[DirectionalLightIndex];

        ImGui::ColorPicker3("Light Color", &Color.x,
            ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB |
            ImGuiColorEditFlags_HDR);

        LightBuffer.lightColor[DirectionalLightIndex] = { Color.x, Color.y, Color.z, Color.w };

        ImGui::TreePop();
    }
    
    for (uint32_t i = 1; i < LightBuffer.numLight; i++)
    {
        if (ImGui::TreeNode(("Point Light " + std::to_string(i)).c_str()))
        {
            ImGui::SliderFloat("Intensity", &LightBuffer.intensityDistance[i].x, 0.0f, 10000.0f);
            ImGui::SliderFloat("MaxDistance", &LightBuffer.intensityDistance[i].y, 100.0f, 5000.0f);
            DirectX::XMFLOAT4& Position = LightBuffer.lightPosition[i];
            ImGui::SliderFloat3("Light Position", &Position.x, -500, 500);

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("Shadow"))
    {
        const char* shadowMethodItems[] = { "None", "ShadowMapping", "Raytracing Shadow" };
        AddCombo("Shadow Method", shadowMethodItems, IM_ARRAYSIZE(shadowMethodItems), Settings.ShadowMethod);

        ImGui::Checkbox("Use Variance Shadow Map", &Settings.bUseVSM);
        ImGui::Checkbox("CSM Debug", &Settings.bCSMDebug);
        ImGui::InputFloat("Shadow Bias", &Settings.ShadowBias, 0.00001, 0.0001, "%.5f");
        ImGui::InputFloat("CSM Exponential Factor", &Settings.CSMExponentialFactor, 0.01, 0.1);

        ImGui::TreePop();
    }
}

void FEditor::RenderPostProcessProperties(FScene* Scene)
{
    FSceneRenderSettings& Settings = Scene->GetRenderSettings();

    const char* toneMappingItems[] = { "Off", "Reinhard", "ReinhardModifed", "ACES" };
    AddCombo("Tone Mapping Method", toneMappingItems, IM_ARRAYSIZE(toneMappingItems), Settings.ToneMappingMethod);
    ImGui::Checkbox("TAA", &Settings.bUseTaa);
    ImGui::Checkbox("Gamma Correction", &Settings.bGammaCorrection);
    ImGui::Checkbox("Bloom", &Settings.bUseBloom);
    ImGui::InputFloat4("Bloom Tint", &Settings.BloomTint[0], "%.4f");

    if (ImGui::TreeNode("Auto Exposure"))
    {
        ImGui::Checkbox("Enable Auto Exposure", &Settings.bUseEyeAdaptation);
        ImGui::SliderFloat("HistogramLogMin", &Settings.HistogramLogMin, -10.f, 0.f);
        ImGui::SliderFloat("HistogramLogMax", &Settings.HistogramLogMax, 0.f, 10.f);
        ImGui::SliderFloat("Time Coeff", &Settings.TimeCoeff, 0.0f, 1.0f);
        ImGui::TreePop();
    }
}

void FEditor::RenderGPUProfileData()
{
    std::vector<FProfileData>& GPUProfileData = RHIGetGPUProfiler().GetProfileData();

    int Depth = 0;
    for (const FProfileData& Data : GPUProfileData)
    {
        if (Data.bHasChildren)
        {
            if (Data.Name != nullptr)
            {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << Data.Duration;
                std::string durationStr = oss.str();

                std::string spaces(Depth*2, ' ');

                std::string NameString = spaces + std::string(Data.Name) + " : " + durationStr + "ms";
                ImGui::Text(NameString.c_str());

                Depth++;
            }
            else
            {
                Depth--;
            }
        }
        else
        {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << Data.Duration;
            std::string durationStr = oss.str();

            std::string spaces(Depth*2, ' ');

            std::string NameString = spaces + std::string(Data.Name) + " : " + durationStr + "ms";
            ImGui::Text(NameString.c_str());
        }
    }
}

void FEditor::RenderProfileProperties(FScene* Scene)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << Scene->CPUFrameMsTime;
    std::string durationStr = oss.str();
    std::string NameString = "CPU : " + durationStr + "ms";
    ImGui::Text(NameString.c_str());

    RenderGPUProfileData();
}

void FEditor::OnWindowResized(uint32_t Width, uint32_t Height)
{
    ImGui::GetMainViewport()->WorkSize = ImVec2((float)Width, (float)Height);
    ImGui::GetMainViewport()->Size = ImVec2((float)Width, (float)Height);
}
