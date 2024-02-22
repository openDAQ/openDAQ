#pragma once

#include <string>

namespace path_tool
{
    std::string GetExecutableDirectory();
    std::string ConcatenatePath(const std::string& directory, const std::string& file);
};
