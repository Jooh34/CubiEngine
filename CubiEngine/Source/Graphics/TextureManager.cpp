#include "Graphics/TextureManager.h"

FTextureManager::FTextureManager()
{
}

std::vector<std::string> FTextureManager::GetDebugTextureKeyList(bool bStartWithNone)
{
	std::vector<std::string> Keys;
    if (bStartWithNone)
    {
        Keys.push_back("(None)");
    }

	Keys.reserve(Keys.size() + DebugTextureMap.size());
	for (auto& kv : DebugTextureMap) {
		if (!kv.first.empty()) {
			Keys.push_back(kv.first);
		}
	}

    return Keys;
}

FTexture* FTextureManager::GetDebugTexture(const std::string& Name)
{
    auto it = DebugTextureMap.find(Name);
    if (it != DebugTextureMap.end())
    {
        return it->second;
    }
    return nullptr;
}

bool FTextureManager::AddDebugTexture(const std::string& Name, FTexture* Texture)
{
    if (Name.empty() || !Texture) {
        return false;
    }

    DebugTextureMap[Name] = Texture;
    return true;
}

bool FTextureManager::RemoveDebugTexture(const std::string& Name, const FTexture* Texture)
{
    if (Name.empty() || !Texture) {
        return false;
    }

    auto it = DebugTextureMap.find(Name);
    if (it == DebugTextureMap.end()) {
        return false;
    }

    if (it->second != Texture) {
        return false;
    }

    DebugTextureMap.erase(it);
    return true;
}
