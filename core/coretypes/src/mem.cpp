#include <coretypes/mem.h>
#include <cstdlib>
#include <exception>
#include <new>

BEGIN_NAMESPACE_OPENDAQ

static void* malloc(size_t len)
{
    return std::malloc(len);
}

static void free(void* ptr)
{
    std::free(ptr);
}

static void free(void* ptr, std::size_t /*size*/)
{
    std::free(ptr);
}

END_NAMESPACE_OPENDAQ

extern "C"
PUBLIC_EXPORT void* daqAllocateMemory(size_t len)
{
    return daq::malloc(len);
}

extern "C"
PUBLIC_EXPORT void daqFreeMemory(void* ptr)
{
    daq::free(ptr);
}

void* operator new(size_t len)
{
    const auto ptr = daq::malloc(len);
    if (ptr == nullptr)
        throw std::bad_alloc();
    return ptr;
}

void* operator new(size_t len, const std::nothrow_t&) noexcept
{
    return daq::malloc(len);
}

void operator delete(void* p) noexcept
{
    daq::free(p);
}

void operator delete(void* p, std::size_t size) noexcept
{
    daq::free(p, size);
}

void operator delete[](void* p, std::size_t size) noexcept
{
    daq::free(p, size);
}

void operator delete(void* p, const std::nothrow_t&) noexcept
{
    daq::free(p);
}

void* operator new[](size_t len)
{
    const auto ptr = daq::malloc(len);
    if (ptr == nullptr)
        throw std::bad_alloc();
    return ptr;
}

void operator delete[](void* p) noexcept
{
    daq::free(p);
}

void* operator new[](size_t len, const std::nothrow_t&) noexcept
{
    return daq::malloc(len);
}

void operator delete[](void* p, const std::nothrow_t&) noexcept
{
    daq::free(p);
}
