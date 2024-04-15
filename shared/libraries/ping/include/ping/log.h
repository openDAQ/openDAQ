#pragma once
#include <fmt/core.h>
#include <cstdio>

#if defined(_WIN32)
    #if __has_include(<fmt/xchar.h>)
        #include <fmt/xchar.h>
    #endif
    #include <Windows.h>
#endif

#if defined(DISCOVERY_VERBOSE) || defined(_DEBUG) || !defined(NDEBUG)
    #if defined(_WIN32)
        #define LOG(pattern, ...) OutputDebugStringA(fmt::format((pattern), ##__VA_ARGS__).data())
        #define LOGW(pattern, ...) OutputDebugStringW(fmt::format((L##pattern), ##__VA_ARGS__).data())

        #define LOG_ERROR(pattern, ...) OutputDebugStringA(fmt::format((pattern), ##__VA_ARGS__).data());
        #define LOGW_ERROR(pattern, ...) OutputDebugStringW(fmt::format((L##pattern), ##__VA_ARGS__).data());
    #else
         #include <fmt/compile.h>

         #define LOG(pattern, ...) fmt::print(FMT_STRING(pattern), ##__VA_ARGS__)
         #define LOGW(pattern, ...) LOG(pattern, ##__VA_ARGS__)

         #define LOG_ERROR(pattern, ...) fmt::print(stderr, FMT_STRING(pattern), ##__VA_ARGS__)
         #define LOGW_ERROR(pattern, ...) LOG_ERROR(pattern, ##__VA_ARGS__)
    #endif
#else
    #define LOG(pattern, ...)
    #define LOGW(pattern, ...)

    #if defined(_WIN32)
        #define LOG_ERROR(pattern, ...) OutputDebugStringA(fmt::format((pattern), ##__VA_ARGS__).data());
        #define LOGW_ERROR(pattern, ...) OutputDebugStringW(fmt::format((L##pattern), ##__VA_ARGS__).data())
    #else
        #include <fmt/compile.h>

        #define LOG_ERROR(pattern, ...) fmt::print(stderr, FMT_STRING(pattern), ##__VA_ARGS__)
        #define LOGW_ERROR(pattern, ...) LOG_ERROR(pattern, ##__VA_ARGS__)
    #endif

#endif
