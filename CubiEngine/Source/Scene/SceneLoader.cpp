#include "Scene/SceneLoader.h"
#include "Scene/Scene.h"
#include "Renderer/Cubemap.h"
#include "Scene/CubeMesh.h"
#include "Scene/SphereMesh.h"
#include "Math/CubiMath.h"

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
                 .Scale = {1.f, 1.f, 1.f},
				 .Translate = { -5.f, 1.f, 0.f },
            };
            Scene->AddModel(Suzanne);

            FModelCreationDesc MirrorBall = {
                 .ModelPath = "Models/Sphere/sphere.gltf",
                 .ModelName = L"MirrorBall",
                 .Rotation = { 0.f, 0.f, 0.f },
                 .Scale = {1.f, 1.f, 1.f},
                 .Translate = { -5.f, 3.f, 0.f },
                 .OverrideBaseColorValue = { 1, 1, 1 },
                 .OverrideRoughnessValue = 0.f,
                 .OverrideMetallicValue = 1.f,
            };
			Scene->AddModel(MirrorBall);

            FModelCreationDesc SphereDesc = {
                 .ModelPath = "Models/Sphere/sphere.gltf",
                 .ModelName = L"Sphere",
                 .Scale = {0.2f, 0.2f, 0.2f},
                 .Translate = { -5.f, 7.f, 4.f },
                 .OverrideBaseColorValue = { 1.f, 1.f, 1.f },
                 .OverrideRoughnessValue = 0.1f,
                 .OverrideMetallicValue = 1.f,
                 .OverrideEmissiveValue = { 1e2, 1e2, 1e2 },
            };
			Scene->AddModel(SphereDesc);

            Scene->GetCamera().SetCamPosition(7.5f, 6, 1);
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
            Scene->EnvmapIntensity = 0.05f;

            // exterior
            Scene->HistogramLogMin = -5.f;
            Scene->HistogramLogMax = 5.f;

            // Camera
            Scene->GetCamera().FarZ = 10000.f;
            break;
        }
        case ESceneType::ABeautifulGame:
        {
            FModelCreationDesc Desc = {
                .ModelPath = "Models/ABeautifulGame/glTF/ABeautifulGame.gltf",
                .ModelName = L"ABeautifulGame",
            };

            // Directional Light
            float LightPosition[4] = { 0.5, -0.5, 0.5, 0 };
            float LightColor[4] = { 1,1,1,1 };
            float Intensity = 500.f;
            Scene->AddLight(LightPosition, LightColor);
            Scene->AddModel(Desc);

            // use envmap
            Scene->GIMethod = 0;
            //Scene->EnvmapIntensity = 1.f;

            // Camera
            Scene->GetCamera().NearZ = 0.01f;
            Scene->GetCamera().FarZ = 100.f;
            Scene->GetCamera().SetCamMovementSpeed(0.01f);

            Scene->EnviromentMap = std::make_unique<FCubeMap>(Device, FCubeMapCreationDesc{
                .EquirectangularTexturePath = L"Assets/Textures/WhiteFurnace.hdr",
                .Name = L"WhiteFurnace Map"
            });
            break;
        }
        case ESceneType::CornellBox:
        {
            Scene->EnvmapIntensity = 0.f;

            static constexpr float Size = 1.f;
            static constexpr float EPS = 1e-6;
			Scene->GetCamera().SetCamPosition(0, Size/2.f, -1.5*Size);

            FMesh* Floor = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox Floor",
				.Rotation = { 0.f, 0.f, 0.f },
                .Scale = { Size, EPS, Size },
                .Translate = { 0.f, 0.f, 0.f },
				.BaseColorValue = { 0.725f, 0.71f, 0.68f },
				.RoughnessValue = 1.f,
				.MetallicValue = 0.f,
                .EmissiveValue = { 0, 0, 0 },
			});
			Scene->AddMesh(Floor);

            FMesh* Ceiling = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox Ceiling",
                .Rotation = { 0.f, 0.f, 0.f },
                .Scale = { Size, EPS, Size },
                .Translate = { 0.f, Size, 0.f },
                .BaseColorValue = { 0.725f, 0.71f, 0.68f },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 0, 0, 0 },
                });
            Scene->AddMesh(Ceiling);

            FMesh* BackWall = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox BackWall",
                .Rotation = { 0.f, 0.f, 0.f },
                .Scale = { Size, Size, EPS },
                .Translate = { 0.f, Size/2.f, Size/2.f},
                .BaseColorValue = { 0.725f, 0.71f, 0.68f },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 0, 0, 0 },
			});
            Scene->AddMesh(BackWall);

            FMesh* LeftWall = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox LeftWall",
                .Rotation = { 0.f, 0.f, 0.f },
                .Scale = { EPS, Size, Size },
                .Translate = { -Size/2.f, Size/2.f, 0.f },
                .BaseColorValue = { 0.63, 0.065, 0.05 },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 0, 0, 0 },
			});
            Scene->AddMesh(LeftWall);

            FMesh* RightWall = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox RightWall",
                .Rotation = { 0.f, 0.f, 0.f },
                .Scale = { EPS, Size, Size },
                .Translate = { Size / 2.f, Size / 2.f, 0.f },
                .BaseColorValue = { 0.14, 0.45, 0.091 },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 0, 0, 0 },
            });
            Scene->AddMesh(RightWall);

            FMesh* ShortBox = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox ShortBox",
                .Rotation = { 0.f, DegreeToRadian(20.f), 0.f},
                .Scale = { Size / 4.f, Size / 4.f, Size / 4.f },
                .Translate = { Size / 8.f, Size / 8.f , -Size / 6.f },
                .BaseColorValue = { 0.725f, 0.71f, 0.68f },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 0, 0, 0 },
			});
            Scene->AddMesh(ShortBox);

            FMesh* TailBox = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox TailBox",
                .Rotation = { 0.f, -DegreeToRadian(20.f), 0.f},
                .Scale = { Size / 4.f, Size / 2.f, Size / 4.f },
                .Translate = { -Size / 8.f, Size / 4.f , Size / 6.f },
                .BaseColorValue = { 0.725f, 0.71f, 0.68f },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 0, 0, 0 },
			});
            Scene->AddMesh(TailBox);

            FMesh* CeilingLight = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox CeilingLight",
                .Rotation = { 0.f, 0.f, 0.f },
                .Scale = { Size/4.f, EPS, Size/4.f },
                .Translate = { 0.f, Size, 0.f },
                .BaseColorValue = { 1,1,1 },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 17,12,4 },
			});
            Scene->AddMesh(CeilingLight);
            break;
        }
        case ESceneType::CustomCornellBox:
        {
            Scene->EnvmapIntensity = 0.f;

            static constexpr float Size = 1.f;
            static constexpr float EPS = 1e-6;
            Scene->GetCamera().SetCamPosition(0, Size / 2.f, -1.5 * Size);

            FMesh* Floor = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox Floor",
                .Rotation = { 0.f, 0.f, 0.f },
                .Scale = { Size, EPS, Size },
                .Translate = { 0.f, 0.f, 0.f },
                .BaseColorValue = { 0.725f, 0.71f, 0.68f },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 0, 0, 0 },
                });
            Scene->AddMesh(Floor);

            FMesh* Ceiling = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox Ceiling",
                .Rotation = { 0.f, 0.f, 0.f },
                .Scale = { Size, EPS, Size },
                .Translate = { 0.f, Size, 0.f },
                .BaseColorValue = { 0.725f, 0.71f, 0.68f },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 0, 0, 0 },
                });
            Scene->AddMesh(Ceiling);

            FMesh* BackWall = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox BackWall",
                .Rotation = { 0.f, 0.f, 0.f },
                .Scale = { Size, Size, EPS },
                .Translate = { 0.f, Size / 2.f, Size / 2.f},
                .BaseColorValue = { 0.725f, 0.71f, 0.68f },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 0, 0, 0 },
                });
            Scene->AddMesh(BackWall);

            FMesh* LeftWall = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox LeftWall",
                .Rotation = { 0.f, 0.f, 0.f },
                .Scale = { EPS, Size, Size },
                .Translate = { -Size / 2.f, Size / 2.f, 0.f },
                .BaseColorValue = { 0.63, 0.065, 0.05 },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 0, 0, 0 },
                });
            Scene->AddMesh(LeftWall);

            FMesh* RightWall = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox RightWall",
                .Rotation = { 0.f, 0.f, 0.f },
                .Scale = { EPS, Size, Size },
                .Translate = { Size / 2.f, Size / 2.f, 0.f },
                .BaseColorValue = { 0.14, 0.45, 0.091 },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 0, 0, 0 },
                });
            Scene->AddMesh(RightWall);

            FMesh* CeilingLight = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox CeilingLight",
                .Rotation = { 0.f, 0.f, 0.f },
                .Scale = { Size / 4.f, EPS, Size / 4.f },
                .Translate = { 0.f, Size, 0.f },
                .BaseColorValue = { 1,1,1 },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 17,12,4 },
                });
            Scene->AddMesh(CeilingLight);

            FMesh* ShortBox = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox ShortBox",
                .Rotation = { 0.f, DegreeToRadian(20.f), 0.f},
                .Scale = { Size / 4.f, Size / 4.f, Size / 4.f },
                .Translate = { Size / 8.f, Size / 8.f , -Size / 6.f },
                .BaseColorValue = { 0.725f, 0.71f, 0.68f },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 0, 0, 0 },
                });
            Scene->AddMesh(ShortBox);

            FMesh* TailBox = new FCubeMesh(Device, FMeshCreationDesc{
                .Name = L"CornellBox TailBox",
                .Rotation = { 0.f, -DegreeToRadian(20.f), 0.f},
                .Scale = { Size / 4.f, Size / 2.f, Size / 4.f },
                .Translate = { -Size / 8.f, Size / 4.f , Size / 6.f },
                .BaseColorValue = { 0.725f, 0.71f, 0.68f },
                .RoughnessValue = 1.f,
                .MetallicValue = 0.f,
                .EmissiveValue = { 0, 0, 0 },
                });
            Scene->AddMesh(TailBox);

            FModelCreationDesc Suzanne = {
                 .ModelPath = "Models/Suzanne/glTF/Suzanne.gltf",
                 .ModelName = L"Suzanne",
                 .Rotation = { 0.f, DegreeToRadian(180.f), 0.f },
                 .Scale = {Size / 10.f, Size / 10.f, Size / 10.f},
                 .Translate = { Size / 8.f, Size / 3.f , -Size / 6.f },
                 .OverrideBaseColorValue = { 0.725f, 0.71f, 0.68f },
                 .OverrideRoughnessValue = 0.f,
                 .OverrideMetallicValue = 1.f,
            };
            Scene->AddModel(Suzanne);

            FMesh* GlassBall = new FSphereMesh(Device, FMeshCreationDesc{
                 .Name = L"GlassBall",
                 .Rotation = { 0.f, 0.f, 0.f },
                 .Scale = { Size / 4.f, Size / 4.f, Size / 4.f },
                 .Translate = { -Size / 4.f, Size / 8.f, -Size / 6.f },
                 .BaseColorValue = { 1, 1, 1 },
                 .RoughnessValue = 0.f,
                 .MetallicValue = 1.f,
                 .EmissiveValue = { 0, 0, 0 },
                 .RefractionFactor = 1.f,
                 .IOR = 1.5f,
             });
            Scene->AddMesh(GlassBall);
        }
	}
}