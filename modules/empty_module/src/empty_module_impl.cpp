#include <empty_module/empty_module_impl.h>
#include <empty_module/version.h>

#include <coretypes/version_info_factory.h>

#include <utility>

BEGIN_NAMESPACE_EMPTY_MODULE

EmptyModule::EmptyModule(ContextPtr context)
    : Module("Empty module",
            daq::VersionInfo(EMPTY_MODULE_MAJOR_VERSION, EMPTY_MODULE_MINOR_VERSION, EMPTY_MODULE_PATCH_VERSION),
            std::move(context)
    )
{
}

END_NAMESPACE_EMPTY_MODULE
