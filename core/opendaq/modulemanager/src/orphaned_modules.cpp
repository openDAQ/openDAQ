#include <opendaq/orphaned_modules.h>

#include "coretypes/errors.h"

#if defined(_MSC_VER) && !defined(NDEBUG)
    #include <crtdbg.h>
#endif

BEGIN_NAMESPACE_OPENDAQ

static constexpr char daqGetObjectCount[] = "daqGetObjectCount";

#if defined(_MSC_VER) && !defined(NDEBUG)

// Constructed on first use: the orphaned module list opens a scope during static initialization.
static std::mutex& untrackedSync()
{
    static std::mutex sync;
    return sync;
}

static int untrackedDepth = 0;
static int untrackedFlags = 0;

UntrackedAllocations::UntrackedAllocations()
{
    std::scoped_lock lock(untrackedSync());
    if (untrackedDepth++ == 0)
    {
        untrackedFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        _CrtSetDbgFlag(untrackedFlags & ~_CRTDBG_ALLOC_MEM_DF);
    }
}

UntrackedAllocations::~UntrackedAllocations()
{
    std::scoped_lock lock(untrackedSync());
    if (--untrackedDepth == 0)
        _CrtSetDbgFlag(untrackedFlags);
}

#endif

OrphanedModules::OrphanedModules()
{
    [[maybe_unused]] const UntrackedAllocations untracked;
    moduleSharedLibs.reserve(20);
}

OrphanedModules::~OrphanedModules()
{
    tryUnload();
}

void OrphanedModules::add(boost::dll::shared_library sharedLib)
{
    std::scoped_lock lock(sync);

    // The buffer this grows into lives as long as the process.
    [[maybe_unused]] const UntrackedAllocations untracked;
    moduleSharedLibs.push_back(std::move(sharedLib));
}

void OrphanedModules::tryUnload()
{
    std::scoped_lock lock(sync);

    for (auto it = moduleSharedLibs.begin(); it != moduleSharedLibs.end();)
    {
        if (canUnloadModule(*it))
            it = moduleSharedLibs.erase(it);
        else
            ++it;
    }
}

bool OrphanedModules::canUnloadModule(const boost::dll::shared_library& moduleSharedLib)
{
    if (!moduleSharedLib.has(daqGetObjectCount))
        return true;

    using GetObjectCount = ErrCode(SizeT*);
    GetObjectCount* getObjectCount = moduleSharedLib.get<GetObjectCount>(daqGetObjectCount);

    SizeT objectCount;
    if (OPENDAQ_SUCCEEDED(getObjectCount(&objectCount)))
    {
        return objectCount == 0;
    }

    return true;
}

END_NAMESPACE_OPENDAQ
