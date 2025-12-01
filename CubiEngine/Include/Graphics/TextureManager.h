#pragma once

#include <map>
#include <vector>
#include <string>
#include "Graphics/Resource.h"

class FTextureManager
{
public:
	FTextureManager();

	std::vector<std::string> GetDebugTextureKeyList(bool bStartWithNone);
	FTexture* GetDebugTexture(const std::string& Name);
	bool AddDebugTexture(const std::string& Name, FTexture* Texture);
	bool RemoveDebugTexture(const std::string& Name, const FTexture* Texture);

private:
	std::map<std::string, FTexture*> DebugTextureMap;
};