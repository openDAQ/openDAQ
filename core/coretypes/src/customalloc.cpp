#include <coretypes/customalloc.h>

#ifdef WINHEAP
    #undef DECLARE_OPENDAQ_INTERFACE
    #include <Windows.h>
#endif

BEGIN_NAMESPACE_OPENDAQ

HeapAllocation::HeapAllocation()
{
#ifdef WINHEAP
    heapHandle = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 0, 0);
#endif
}

HeapAllocation::HeapAllocation(const HeapAllocation& other)
#ifdef WINHEAP
    : heapHandle(other.heapHandle)
#endif
{
}

HeapAllocation& HeapAllocation::operator=(const HeapAllocation& other)
{
    if (this == &other)
        return *this;

#ifdef WINHEAP
    heapHandle = other.heapHandle;
#endif

    return *this;
}

HeapAllocation::~HeapAllocation()
{
#ifdef WINHEAP
    HeapDestroy(heapHandle);
#endif
}

void* HeapAllocation::allocMem(size_t len)
{
#ifdef WINHEAP
    return HeapAlloc(heapHandle, 0, len);
#else
    return std::malloc(len);
#endif
}

void HeapAllocation::freeMem(void* ptr)
{
#ifdef WINHEAP
    HeapFree(heapHandle, 0, ptr);
#else
    std::free(ptr);
#endif
}

HeapAllocation& getHeapAlloc()
{
    static HeapAllocation heapAlloc;
    return heapAlloc;
}

extern "C"
void PUBLIC_EXPORT daqPrepareHeapAlloc()
{
    getHeapAlloc();
}

END_NAMESPACE_OPENDAQ
