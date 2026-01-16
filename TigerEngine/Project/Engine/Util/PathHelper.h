#pragma once
#include <pch.h>
#include <optional>

class PathHelper
{
public:
    static std::optional<std::filesystem::path> FindDirectory(std::string dirName);
    static std::filesystem::path GetExeDir();
};

