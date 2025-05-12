#include <properties_module/properties_module_impl.h>
#include <properties_module/version.h>

#include <coretypes/version_info_factory.h>

#include <utility>

BEGIN_NAMESPACE_PROPERTIES_MODULE

PropertiesModule::PropertiesModule(ContextPtr context)
    : Module("PropertiesModule",
            daq::VersionInfo(PROPERTIES_MODULE_MAJOR_VERSION, PROPERTIES_MODULE_MINOR_VERSION, PROPERTIES_MODULE_PATCH_VERSION),
            std::move(context)
    )
{
}

END_NAMESPACE_PROPERTIES_MODULE
