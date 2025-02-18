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

FEditor::FEditor(FGraphicsDevice* Device, SDL_Window* Window, uint32_t Width, uint32_t Height)
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

    FDescriptorHandle FontDescriptorHandle = Device->GetCbvSrvUavDescriptorHeap()->GetCurrentDescriptorHandle();

    ImGui_ImplSDL2_InitForD3D(Window);
    ImGui_ImplDX12_Init(Device->GetDevice(), FGraphicsDevice::FRAMES_IN_FLIGHT,
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

void FEditor::Render(FGraphicsContext* GraphicsContext, FScene* Scene)
{
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
    
    AddCombo("Debug Visualize", Scene->DebugVisualizeList, Scene->DebugVisualizeList.size(), Scene->DebugVisualizeIndex);
    const char* wfItems[] = { "Off", "Sampling", "IBL", "Albedo only"};
    AddCombo("White Furnace Method", wfItems, IM_ARRAYSIZE(wfItems), Scene->WhiteFurnaceMethod);
    /*if (ImGui::BeginCombo("White Furnace Method", wfItems[Scene->WhiteFurnaceMethod]))
    {
        for (int i = 0; i < IM_ARRAYSIZE(wfItems); i++)
        {
            bool isSelected = (Scene->WhiteFurnaceMethod == i);
            if (ImGui::Selectable(wfItems[i], isSelected))
                Scene->WhiteFurnaceMethod = i;

            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }*/

    ImGui::Checkbox("TAA", &Scene->bUseTaa);
    ImGui::Checkbox("Tone Mapping", &Scene->bToneMapping);
    ImGui::Checkbox("Gamma Correction", &Scene->bGammaCorrection);
    ImGui::Checkbox("Energy Compensation", &Scene->bUseEnergyCompensation);
    ImGui::Checkbox("CSM Debug", &Scene->bCSMDebug);

    ImGui::End();
}

void FEditor::RenderCameraProperties(FScene* Scene)
{
    ImGui::SetNextWindowPos(ImVec2(0, 200));
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);
    ImGui::Begin("Camera Properties");
    
    ImGui::InputFloat3("Camera Position", &Scene->GetCamera().GetCameraPosition().x);

    ImGui::SliderFloat("Far Clip Distance", &Scene->GetCamera().FarZ, 10.0f, 5000.0f);

    ImGui::End();
}

void FEditor::RenderGIProperties(FScene* Scene)
{
    ImGui::SetNextWindowPos(ImVec2(0, 400));
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);
    ImGui::Begin("GI Properties");

    const char* giItems[] = { "Off", "SSGI" };
    AddCombo("GI Method", giItems, IM_ARRAYSIZE(giItems), Scene->GIMethod);
    ImGui::SliderFloat("SSGI Intensity", &Scene->SSGIIntensity, 0.0f, 30.0f);
    ImGui::SliderFloat("SSGI RayLength", &Scene->SSGIRayLength, 0.0f, 2000.0f);
    ImGui::SliderInt("SSGI NumSteps", &Scene->SSGINumSteps, 1, 256);

    ImGui::End();
}

void FEditor::RenderLightProperties(FScene* Scene)
{
    ImGui::SetNextWindowPos(ImVec2(0, 600));
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);

    ImGui::Begin("Light Properties");
    interlop::LightBuffer& LightBuffer = Scene->Light.LightBufferData;

    bool& bUseEnvmap = Scene->bUseEnvmap;
    ImGui::Checkbox("Use EnvMap", &bUseEnvmap);

    if (ImGui::TreeNode("Directional Light"))
    {
        constexpr uint32_t DirectionalLightIndex = 0u;
        
        ImGui::SliderFloat("Intensity", &LightBuffer.intensity[DirectionalLightIndex], 0.0f, 100.0f);
        ImGui::SliderFloat("MaxDistance", &LightBuffer.maxDistance[DirectionalLightIndex], 100.0f, 5000.0f);

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
    ImGui::End();
}

void FEditor::OnWindowResized(uint32_t Width, uint32_t Height)
{
    ImGui::GetMainViewport()->WorkSize = ImVec2((float)Width, (float)Height);
    ImGui::GetMainViewport()->Size = ImVec2((float)Width, (float)Height);
}
