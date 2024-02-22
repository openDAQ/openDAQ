#include <opendaq/path_tool.h>

#ifdef _WIN32
    #include <windows.h>
#elif __linux__
    #include <unistd.h>
    #include <limits.h>
#elif __APPLE__
    #include <mach-o/dyld.h>
#endif

namespace path_tool
{
    #ifdef _WIN32
        std::string GetExecutableDirectory()
        {
            char buffer[MAX_PATH];
            GetModuleFileNameA(NULL, buffer, MAX_PATH);
            std::string directory = buffer;
            return directory.substr(0, directory.find_last_of("\\/"));
        }
    #elif __linux__
        std::string GetExecutableDirectory()
        {
            char buffer[PATH_MAX];
            ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
            if (len != -1)
            {
                buffer[len] = '\0';
                return std::string(buffer).substr(0, std::string(buffer).find_last_of("/"));
            }
            return "";
        }
    #elif __APPLE__
        std::string GetExecutableDirectory()
        {
            char buffer[PATH_MAX];
            uint32_t size = sizeof(buffer);
            if (_NSGetExecutablePath(buffer, &size) == 0)
            {
                buffer[size] = '\0';
                return std::string(buffer).substr(0, std::string(buffer).find_last_of("/"));
            }
            return "";
        }
    #else
        std::string GetExecutableDirectory()
        {
            return "";
        }
    #endif

    std::string ConcatenatePath(const std::string& directory, const std::string& file) 
    {
        std::string result = directory;
        if (!result.empty() && result.back() != '/' && result.back() != '\\')
            result += '/';

        result += file;
        return result;
    }
};