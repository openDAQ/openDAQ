#include <opendaq/module.h>
#include <coretypes/impl.h>

#include "mock_module.h"

extern "C"
daq::ErrCode PUBLIC_EXPORT createModule(daq::IModule** module)
{
    if (module == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return daq::createObject<daq::IModule, MockModuleImpl>(module);
}

extern "C"
daq::ErrCode PUBLIC_EXPORT checkDependencies(daq::IString** message)
{
    return OPENDAQ_SUCCESS;
}
