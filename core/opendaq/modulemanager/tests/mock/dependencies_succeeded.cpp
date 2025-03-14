#include <opendaq/module.h>
#include <coretypes/impl.h>

#include "mock_module.h"

extern "C"
daq::ErrCode PUBLIC_EXPORT createModule(daq::IModule** module)
{
    OPENDAQ_PARAM_NOT_NULL(module);

    return daq::createObject<daq::IModule, MockModuleImpl>(module);
}

extern "C"
daq::ErrCode PUBLIC_EXPORT checkDependencies(daq::IString** message)
{
    return OPENDAQ_SUCCESS;
}
