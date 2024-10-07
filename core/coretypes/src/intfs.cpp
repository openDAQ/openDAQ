#include <coretypes/intfs.h>
#include <coretypes/customalloc.h>
#include <mutex>
#include <cstring>
#include <fmt/core.h>

using namespace daq;

#if !defined(NDEBUG)
    #include <unordered_set>

    using ObjectContainer = std::unordered_set<IBaseObject*, std::hash<IBaseObject*>, std::equal_to<>, CustomAllocator<IBaseObject*>>;

    // make a function for global variable to ensure proper creation order (Scott Meyers, Effective C++, 3rd edition, item 3)
    ObjectContainer& getObjects()
    {
        static ObjectContainer* objects = new ObjectContainer();
        return *objects;
    }

    std::mutex& getObjectsMutex()
    {
        static std::mutex mtx;
        return mtx;
    }
#endif

extern "C"
PUBLIC_EXPORT void daqTrackObject(IBaseObject* obj)
{
#ifndef NDEBUG
    std::lock_guard<std::mutex> lock(getObjectsMutex());
    getObjects().insert(obj);
#endif
}

extern "C"
PUBLIC_EXPORT void daqUntrackObject(IBaseObject* obj)
{
#ifndef NDEBUG
    std::lock_guard<std::mutex> lock(getObjectsMutex());
    const auto it = getObjects().find(obj);
    if (it != getObjects().end())
        getObjects().erase(it);
#endif
}

extern "C"
PUBLIC_EXPORT size_t daqGetTrackedObjectCount()
{
#ifndef NDEBUG
    std::lock_guard<std::mutex> lock(getObjectsMutex());
    return getObjects().size();
#else
    return 0;
#endif
}

extern "C"
PUBLIC_EXPORT void daqPrintTrackedObjects()
{
#ifndef NDEBUG
    std::lock_guard<std::mutex> lock(getObjectsMutex());
    for (auto obj : getObjects())
    {
        assert(obj != nullptr);
        fmt::print("{:p}: {}\n", (void*) obj, objectToString(obj));
    }
#endif
}

extern "C"
PUBLIC_EXPORT void daqClearTrackedObjects()
{
#ifndef NDEBUG
    std::lock_guard<std::mutex> lock(getObjectsMutex());
    getObjects().clear();
#endif
}

extern "C"
PUBLIC_EXPORT daq::Bool daqIsTrackingObjects()
{
#ifndef NDEBUG
    return true;
#else
    return false;
#endif
}

extern "C"
PUBLIC_EXPORT ErrCode daqDuplicateCharPtr(ConstCharPtr source, CharPtr* dest)
{
    if (dest == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (source == nullptr)
        *dest = nullptr;
    else
    {
        const auto len = std::strlen(source) + 1;
        *dest = static_cast<CharPtr>(daqAllocateMemory(len));
        if (*dest == nullptr)
            return OPENDAQ_ERR_NOMEMORY;

#if defined(__STDC_SECURE_LIB__) || defined(__STDC_LIB_EXT1__)
        strcpy_s(*dest, len, source);
#else
        strncpy(*dest, source, len);
#endif
    }
    return OPENDAQ_SUCCESS;
}

extern "C"
ErrCode PUBLIC_EXPORT daqDuplicateCharPtrN(ConstCharPtr source, SizeT length, CharPtr* dest)
{
    if (dest == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (source == nullptr)
        *dest = nullptr;
    else
    {
        const auto len = length + 1;
        *dest = static_cast<CharPtr>(daqAllocateMemory(len));
        if (*dest == nullptr)
            return OPENDAQ_ERR_NOMEMORY;

#if defined(__STDC_SECURE_LIB__) || defined(__STDC_LIB_EXT1__)
        memcpy_s(*dest, len - 1, source, len - 1);
#else
        memcpy(*dest, source, len - 1);
#endif
        (*dest)[len - 1] = '\0';
    }
    return OPENDAQ_SUCCESS;
}
