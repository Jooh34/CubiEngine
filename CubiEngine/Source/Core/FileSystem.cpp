#include "Core/FileSystem.h"

void FFileSystem::LocateRootDirectory()
{
    auto currentDirectory = std::filesystem::current_path();

    // The asset directory is one folder within the root directory.
    while (!std::filesystem::exists(currentDirectory / "Assets"))
    {
        if (currentDirectory.has_parent_path())
        {
            currentDirectory = currentDirectory.parent_path();
        }
        else
        {
            FatalError("Assets Directory not found!");
        }
    }

    auto assetsDirectory = currentDirectory / "Assets";

    if (!std::filesystem::is_directory(assetsDirectory))
    {
        FatalError("Assets Directory that was located is not a directory!");
    }

    s_rootDirectoryPath = currentDirectory.string() + "/";

    Log(std::format("Detected root directory at path : {}.", s_rootDirectoryPath));
}