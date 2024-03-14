#include <opendaq/create_device.h>
#include <opendaq/custom_log.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{

DevicePtr createDevice(const StringPtr& connectionString,
                       const PropertyObjectPtr& config,
                       const ComponentPtr& parent,
                       const ModuleManagerPtr& manager,
                       const LoggerComponentPtr& loggerComponent)
{
    for (const auto module : manager.getModules())
    {
        bool accepted{};
        try
        {
            accepted = module.acceptsConnectionParameters(connectionString, config);
        }
        catch (NotImplementedException&)
        {
            LOG_I("{}: AcceptsConnectionString not implemented", module.getName())
            accepted = false;
        }
        catch (const std::exception& e)
        {
            LOG_W("{}: AcceptsConnectionString failed: {}", module.getName(), e.what())
            accepted = false;
        }

        if (accepted)
        {
            auto device = module.createDevice(connectionString, parent, config);
            return device;
        }
    }

    throw NotFoundException{"No module supports the specified connection string and configuration [{}]", connectionString};
}

}

END_NAMESPACE_OPENDAQ
