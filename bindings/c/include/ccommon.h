#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(_WIN32)
    #define EXPORTED __attribute__ ((visibility ("default")))
#elif defined(BUILDING_SHARED_LIBRARY)
    #define EXPORTED __declspec(dllexport)
#else
    #define EXPORTED __declspec(dllimport)
#endif


#include <stddef.h>
#include <stdint.h>

    typedef uint32_t ErrCode;
    typedef uint8_t Bool;
    typedef int64_t Int;
    typedef uint64_t UInt;
    typedef double Float;
    typedef char* CharPtr;
    typedef const char* ConstCharPtr;
    typedef void* VoidPtr;
    typedef size_t SizeT;
    typedef uint32_t EnumType;

    const Bool True = 1;
    const Bool False = 0;

#ifdef __cplusplus
}
#endif
