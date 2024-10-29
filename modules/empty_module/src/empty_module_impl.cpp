#include <empty_module/empty_module_impl.h>
#include <empty_module/version.h>
#include <coretypes/version_info_factory.h>
#include <utility>
#include <opendaq/module_info_factory.h>

BEGIN_NAMESPACE_EMPTY_MODULE

EmptyModule::EmptyModule(const ContextPtr& ctx)
    : Module(daq::ModuleInfo(daq::VersionInfo(EMPTY_MODULE_MAJOR_VERSION, EMPTY_MODULE_MINOR_VERSION, EMPTY_MODULE_PATCH_VERSION),
                             "EmptyModule",
                             "EmptyModule"),
             ctx)
{
}

END_NAMESPACE_EMPTY_MODULE
