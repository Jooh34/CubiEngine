#pragma once

class FFileSystem
{
public:
    static inline std::string GetFullPath(const std::string_view LocalPath)
    {
        return std::move(s_rootDirectoryPath + LocalPath.data());
    }

    static inline std::wstring GetFullPath(const std::wstring_view LocalPath)
    {
        return std::move(StringToWString(s_rootDirectoryPath) + LocalPath.data());
    }

    static inline std::string GetAssetPath()
    {
        return std::move(s_rootDirectoryPath + "Assets/");
    }

    static void LocateRootDirectory();

private:
    static inline std::string s_rootDirectoryPath{};
};