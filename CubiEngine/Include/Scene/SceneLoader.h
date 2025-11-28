#pragma once

class FScene;


enum ESceneType
{
	Sponza = 0,
	MetalRoughness,
	FlightHelmet,
	Suzanne,
	Bistro,
	ABeautifulGame,
	CornellBox,
	CustomCornellBox,
};

class FSceneLoader
{
public:
	static void LoadScene(ESceneType SceneType, FScene* Scene);
};