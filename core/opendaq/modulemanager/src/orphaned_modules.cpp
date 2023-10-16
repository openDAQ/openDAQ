#include <opendaq/orphaned_modules.h>

#include "coretypes/errors.h"

BEGIN_NAMESPACE_OPENDAQ

static constexpr char daqGetObjectCount[] = "daqGetObjectCount";

OrphanedModules::OrphanedModules()
{
    moduleSharedLibs.reserve(20); // To prevent false positive memory leaks in tests
}

OrphanedModules::~OrphanedModules()
{
    tryUnload();
}

void OrphanedModules::add(boost::dll::shared_library sharedLib)
{
    std::scoped_lock lock(sync);
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

    moduleSharedLibs.shrink_to_fit();
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
