#include "Scene/SceneLoader.h"
#include "Scene/Scene.h"
#include "Renderer/Cubemap.h"

void FSceneLoader::LoadScene(ESceneType SceneType, FScene* Scene, FGraphicsDevice* Device)
{
    // set environment map
    Scene->EnviromentMap = std::make_unique<FCubeMap>(Device, FCubeMapCreationDesc{
        .EquirectangularTexturePath = L"Assets/Textures/pisa.hdr",
        .Name = L"Environment Map"
	});

	switch (SceneType)
	{
        case ESceneType::Sponza:
		{
            // Directional Light
            float LightPosition[4] = { -0.5, -1, -0.4, 0 };
            float LightColor[4] = { 1,1,1,1 };
            float Intensity = 50.f;
            Scene->AddLight(LightPosition, LightColor, Intensity);

            // Point Light
            float PointLightPosition[4] = { 0.f,100.f,0.f,1.f };
            float PointLightColor[4] = { 1,1,1,1 };
            Scene->AddLight(PointLightPosition, PointLightColor, 1.f);

            FModelCreationDesc SponzaDesc = {
                .ModelPath = "Models/Sponza/sponza.glb",
                .ModelName = L"Sponza",
            };
            Scene->AddModel(SponzaDesc);

            FModelCreationDesc Suzanne = {
                 .ModelPath = "Models/Suzanne/glTF/Suzanne.gltf",
                 .ModelName = L"Suzanne",
				 .Rotation = { 0.f, 90.f, 0.f },
                 .Scale = {100.f, 100.f, 100.f},
				 .Translate = { -500.f, 100.f, 0.f },
            };
            Scene->AddModel(Suzanne);

            FModelCreationDesc MirrorBall = {
                 .ModelPath = "Models/Sphere/sphere.gltf",
                 .ModelName = L"MirrorBall",
                 .Rotation = { 0.f, 0.f, 0.f },
                 .Scale = {50.f, 50.f, 50.f},
                 .Translate = { -500.f, 300.f, 0.f },
                 .OverrideBaseColorValue = { 1, 1, 1 },
                 .OverrideRoughnessValue = 0.f,
                 .OverrideMetallicValue = 1.f,
            };
			Scene->AddModel(MirrorBall);

            FModelCreationDesc SphereDesc = {
                 .ModelPath = "Models/Sphere/sphere.gltf",
                 .ModelName = L"Sphere",
                 .Scale = {20.f, 20.f, 20.f},
                 .Translate = { -500.f, 700.f, 400.f },
                 .OverrideBaseColorValue = { 1.f, 1.f, 1.f },
                 .OverrideRoughnessValue = 0.1f,
                 .OverrideMetallicValue = 1.f,
                 .OverrideEmissiveValue = { 3e3, 3e3, 3e3 },
            };
			Scene->AddModel(SphereDesc);

            Scene->GetCamera().SetCamPosition(750, 600, 100);
            Scene->GetCamera().SetCamRotation(0.03, -1.7, 0.f);
			break;
		}
		case ESceneType::MetalRoughness:
		{
            FModelCreationDesc Desc = {
				.ModelPath = "Models/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf",
				.ModelName = L"MetalRoughSpheres",
            };

            // use envmap
            Scene->GIMethod = 0;
            Scene->bUseEnvmap = true;
            Scene->EnvmapIntensity = 1.f;

            // Directional Light
            float LightPosition[4] = { -0.5, -1, -0.4, 0 };
            float LightColor[4] = { 1,1,1,1 };
            float Intensity = 5.f;
            Scene->AddLight(LightPosition, LightColor);

			Scene->AddModel(Desc);
			break;
		}
		case ESceneType::FlightHelmet:
        {
            FModelCreationDesc Desc = {
				.ModelPath = "Models/FlightHelmet/glTF/FlightHelmet.gltf",
				.ModelName = L"FlightHelmet",
            };

            // use envmap
            Scene->GIMethod = 0;
            Scene->bUseEnvmap = true;
            Scene->EnvmapIntensity = 1.f;

            // Directional Light
            float LightPosition[4] = { -0.5, -1, -0.4, 0 };
            float LightColor[4] = { 1,1,1,1 };
            float Intensity = 5.f;
            Scene->AddLight(LightPosition, LightColor);

            Scene->AddModel(Desc);
            break;
        }
		case ESceneType::Suzanne:
		{
            FModelCreationDesc Desc = {
                 .ModelPath = "Models/Suzanne/glTF/Suzanne.gltf",
                 .ModelName = L"Suzanne",
            };

            // use envmap
            Scene->GIMethod = 0;
            Scene->bUseEnvmap = true;
            Scene->EnvmapIntensity = 1.f;

            // Directional Light
            float LightPosition[4] = { -0.5, -1, -0.4, 0 };
            float LightColor[4] = { 1,1,1,1 };
            float Intensity = 1.f;
            Scene->AddLight(LightPosition, LightColor);
            Scene->AddModel(Desc);
            break;
		}
		case ESceneType::Bistro:
        {
            FModelCreationDesc Desc = {
				.ModelPath = "Models/Bistro/Bistro_v5_2/BistroExterior.fbx",
				.ModelName = L"Bistro Exterior",
            };

            // Directional Light
            float LightPosition[4] = { 0.5, -0.5, 0.5, 0 };
            float LightColor[4] = { 1,1,1,1 };
            float Intensity = 5.f;
            Scene->AddLight(LightPosition, LightColor);
            Scene->AddModel(Desc);

            // use envmap
            Scene->GIMethod = 0;
            Scene->bUseEnvmap = true;
            Scene->EnvmapIntensity = 0.05f;

            // exterior
            Scene->HistogramLogMin = -5.f;
            Scene->HistogramLogMax = 5.f;

            // set environment map
            Scene->EnviromentMap = std::make_unique<FCubeMap>(Device, FCubeMapCreationDesc{
                .EquirectangularTexturePath = L"Assets/Models/Bistro/Bistro_v5_2/san_giuseppe_bridge_4k.hdr",
                .Name = L"Bistro Environment Map"
			});

            // Camera
            Scene->GetCamera().FarZ = 10000.f;
        }
	}
}