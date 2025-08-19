#include "Core/Editor.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/GraphicsContext.h"
#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "Scene/Scene.h"
#include "Core/FileSystem.h"
#include "Renderer/PostProcess.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_sdl2.h>
#include <sstream>
#include <iomanip>

FEditor::FEditor(FGraphicsDevice* Device, SDL_Window* Window, uint32_t Width, uint32_t Height)
    :Device(Device), Window(Window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)Width, (float)Height);

    const std::string iniFilePath = FFileSystem::GetFullPath("imgui.ini");
    const auto relativeIniFilePath = std::filesystem::relative(iniFilePath);
    std::string ini = relativeIniFilePath.string();
    io.IniFilename = ini.c_str();

    FDescriptorHandle FontDescriptorHandle = Device->GetCbvSrvUavDescriptorHeap()->GetCurrentDescriptorHandle();

    ImGui_ImplSDL2_InitForD3D(Window);
    ImGui_ImplDX12_Init(Device->GetDevice(), FRAMES_IN_FLIGHT,
        Device->GetSwapChainFormat(),
        Device->GetCbvSrvUavDescriptorHeap()->GetDescriptorHeap(),
        FontDescriptorHandle.CpuDescriptorHandle, FontDescriptorHandle.GpuDescriptorHandle);

    Device->GetCbvSrvUavDescriptorHeap()->OffsetCurrentHandle();
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
    
    RenderDebugProperties(Scene);
    RenderCameraProperties(Scene);
    RenderGIProperties(Scene);
    RenderLightProperties(Scene);
    RenderPostProcessProperties(Scene);
    RenderProfileProperties(Scene);

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GraphicsContext->GetCommandList());
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

void FEditor::RenderDebugProperties(FScene* Scene)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);

    ImGui::Begin("Debug Properties");
    
    std::vector<std::string> KeyList = { "None" };
	Device->AppendDebugTextureKeyList(KeyList);

    AddCombo("Debug Visualize", KeyList, KeyList.size(), Scene->SelectedTextureIndex);
	Scene->SelectedDebugTexture = Device->GetDebugTexture(KeyList[Scene->SelectedTextureIndex]);

    ImGui::SliderFloat("VisDebugMin", &Scene->VisualizeDebugMin, 0.f, 1.f);
    ImGui::SliderFloat("VisDebugMax", &Scene->VisualizeDebugMax, 0.f, 1.f);

	ImGui::Checkbox("Enable Diffuse", &Scene->bEnableDiffuse);
	ImGui::Checkbox("Enable Specular", &Scene->bEnableSpecular);
    ImGui::Checkbox("Energy Compensation", &Scene->bUseEnergyCompensation);

    const char* diffuseItems[] = { "Lambertian", "Disney_Burley"};
    AddCombo("Diffuse Model", diffuseItems, IM_ARRAYSIZE(diffuseItems), Scene->DiffuseMethod);

    const char* renderingModeItems[] = { "Rasterize", "Debug Raytracing", "PathTrace"};
    AddCombo("Rendering Mode", renderingModeItems, IM_ARRAYSIZE(renderingModeItems), Scene->RenderingMode);

    ImGui::SliderInt("PathTracing SamplePerPixel", &Scene->PathTracingSamplePerPixel, 1, 64);

    ImGui::SliderInt("Max FPS", &Scene->MaxFPS, 30, 144);

    const char* wfItems[] = { "Off", "Sampling", "IBL", "Albedo only"};
    AddCombo("White Furnace Method", wfItems, IM_ARRAYSIZE(wfItems), Scene->WhiteFurnaceMethod);

    ImGui::End();
}

void FEditor::RenderCameraProperties(FScene* Scene)
{
    ImGui::SetNextWindowPos(ImVec2(0, 200));
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);

    ImGui::Begin("Camera Properties");
    
    if (ImGui::TreeNode("Auto Exposure"))
    {
        ImGui::Checkbox("Enable Auto Exposure", &Scene->bUseEyeAdaptation);
        ImGui::SliderFloat("HistogramLogMin", &Scene->HistogramLogMin, -10.f, 0.f);
        ImGui::SliderFloat("HistogramLogMax", &Scene->HistogramLogMax, 0.f, 10.f);
        ImGui::SliderFloat("Time Coeff", &Scene->TimeCoeff, 0.0f, 1.0f);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Camera"))
    {
        ImGui::InputFloat3("Camera Position", &Scene->GetCamera().GetCameraPosition().x);
        ImGui::SliderFloat("Fov", &Scene->GetCamera().FovY, 30.0f, 120.0f);
        ImGui::SliderFloat("Far Clip Distance", &Scene->GetCamera().FarZ, 1000.0f, 10000.0f);
        ImGui::TreePop();
    }

    ImGui::End();
}

void FEditor::RenderGIProperties(FScene* Scene)
{
    ImGui::SetNextWindowPos(ImVec2(0, 400));
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);
    ImGui::Begin("GI Properties");

    if (ImGui::TreeNode("SSAO"))
    {
        ImGui::Checkbox("Use SSAO", &Scene->bUseSSAO);
        ImGui::SliderInt("SSAO Kernel Size", &Scene->SSAOKernelSize, 8, 64);
        ImGui::SliderFloat("SSAO Kernel Radius", &Scene->SSAOKernelRadius, 1e-3f, 1e-2f);
        ImGui::InputFloat("SSAO Depth Bias", &Scene->SSAODepthBias, 1e-6, 1e-5, "%.6f");
        ImGui::Checkbox("SSAO Range Check", &Scene->SSAOUseRangeCheck);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("SSGI"))
    {
        const char* giItems[] = { "Off", "SSGI" };
        AddCombo("GI Method", giItems, IM_ARRAYSIZE(giItems), Scene->GIMethod);
        ImGui::SliderFloat("SSGI Intensity", &Scene->SSGIIntensity, 0.0f, 20.0f);
        ImGui::SliderFloat("SSGI RayLength", &Scene->SSGIRayLength, 0.0f, 3.0f);
        ImGui::SliderInt("SSGI NumSteps", &Scene->SSGINumSteps, 1, 256);
        ImGui::SliderFloat("SSGI CompareToleranceScale", &Scene->CompareToleranceScale, 1.f, 30.f);
        ImGui::SliderInt("SSGI NumSamples", &Scene->SSGINumSamples, 1, 16);

        ImGui::SliderInt("SSGI GaussianKernelSize", &Scene->SSGIGaussianKernelSize, 1, 32);
        ImGui::SliderFloat("SSGI GaussianStdDev", &Scene->SSGIGaussianStdDev, 0.1f, 20.0f);

        const char* sampleingMethodItems[] = { "UniformSampleHemisphere", "ImportanceSampleCosDir", "ConcentricSampleDisk", "ConcentricSampleDiskUE5"};
        AddCombo("Sampling Method", sampleingMethodItems, IM_ARRAYSIZE(sampleingMethodItems), Scene->StochasticNormalSamplingMethod);
        ImGui::TreePop();
    }
    
    ImGui::End();
}

void FEditor::RenderLightProperties(FScene* Scene)
{
    ImGui::SetNextWindowPos(ImVec2(0, 600));
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);

    ImGui::Begin("Light Properties");
    interlop::LightBuffer& LightBuffer = Scene->Light.LightBufferData;

    ImGui::SliderFloat("Envmap Intensity", &Scene->EnvmapIntensity, 0.f, 100.f);

    ImGui::Checkbox("Light Dance Debug", &Scene->bLightDanceDebug);
    ImGui::SliderFloat("Light Dance Speed", &Scene->bLightDanceSpeed, 0.1f, 10.0f);

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
        AddCombo("Shadow Method", shadowMethodItems, IM_ARRAYSIZE(shadowMethodItems), Scene->ShadowMethod);

        ImGui::Checkbox("Use Variance Shadow Map", &Scene->bUseVSM);
        ImGui::Checkbox("CSM Debug", &Scene->bCSMDebug);
        ImGui::InputFloat("Shadow Bias", &Scene->ShadowBias, 0.00001, 0.0001, "%.5f");
        ImGui::InputFloat("CSM Exponential Factor", &Scene->CSMExponentialFactor, 0.01, 0.1);

        ImGui::TreePop();
    }

    ImGui::End();
}

void FEditor::RenderPostProcessProperties(FScene* Scene)
{
    ImGui::SetNextWindowPos(ImVec2(0, 800));
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);
    ImGui::Begin("PostProcess Properties");

    const char* toneMappingItems[] = { "Off", "Reinhard", "ReinhardModifed", "ACES" };
    AddCombo("Tone Mapping Method", toneMappingItems, IM_ARRAYSIZE(toneMappingItems), Scene->ToneMappingMethod);
    ImGui::Checkbox("TAA", &Scene->bUseTaa);
    ImGui::Checkbox("Gamma Correction", &Scene->bGammaCorrection);
    ImGui::Checkbox("Bloom", &Scene->bUseBloom);
    ImGui::InputFloat4("Bloom Tint", &Scene->BloomTint[0], "%.4f");

    ImGui::End();
}

void FEditor::RenderGPUProfileData()
{
    std::vector<FProfileData>& GPUProfileData = Device->GetGPUProfiler().GetProfileData();

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
    ImGuiIO& io = ImGui::GetIO();

    float W = 300.0f;
    float H = 200.0f;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - W, 600));
    ImGui::SetNextWindowSize(ImVec2(W, H), ImGuiCond_Once);
    
    ImGui::Begin("Profile");
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << Scene->CPUFrameMsTime;
    std::string durationStr = oss.str();
    std::string NameString = "CPU : " + durationStr + "ms";
    ImGui::Text(NameString.c_str());

    RenderGPUProfileData();
    ImGui::End();
}

void FEditor::OnWindowResized(uint32_t Width, uint32_t Height)
{
    ImGui::GetMainViewport()->WorkSize = ImVec2((float)Width, (float)Height);
    ImGui::GetMainViewport()->Size = ImVec2((float)Width, (float)Height);
}
