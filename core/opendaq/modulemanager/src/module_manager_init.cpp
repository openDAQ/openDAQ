#include <opendaq/logger_factory.h>
#include <opendaq/module_manager_init.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/context_factory.h>
#include <coretypes/errors.h>

using namespace daq;

ErrCode daqInitModuleManagerLibrary()
{
    ModuleManager(".");
    return OPENDAQ_SUCCESS;
}
