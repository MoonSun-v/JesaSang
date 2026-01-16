#include "PathHelper.h"

std::optional<std::filesystem::path> PathHelper::FindDirectory(std::string dirName)
{
    std::filesystem::path cur = GetExeDir(); // fx::current_path는 IDE와 환경에 따라 다를 수 있으므로 따로 이름 가져오기
    for (int i = 0; i < 5; i++)
    {
        std::filesystem::path candidate = cur / dirName; // 현재 폴더
        if (std::filesystem::exists(candidate) && std::filesystem::is_directory(candidate)) // 해당 위치에 폴더가 존재한다.
        {
            return candidate;
        }
        cur = cur.parent_path();
    }

    std::string errorMessage = "failed find scenes directory\n Dir : module/" + dirName;
    MessageBoxA(nullptr, errorMessage.c_str(), "Error", MB_OK);
    return std::nullopt;
}

std::filesystem::path PathHelper::GetExeDir()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}