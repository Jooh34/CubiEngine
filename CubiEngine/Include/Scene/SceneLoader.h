#pragma once

class FScene;
class FGraphicsDevice;

enum ESceneType
{
	Sponza = 0,
	MetalRoughness,
	FlightHelmet,
	Suzanne,
	Bistro,
	CornellBox
};

class FSceneLoader
{
public:
	static void LoadScene(ESceneType SceneType, FScene* Scene, FGraphicsDevice* Device);
};