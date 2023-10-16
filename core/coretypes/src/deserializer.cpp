#include <coretypes/deserializer.h>
#include <coretypes/errors.h>
#include <unordered_map>

BEGIN_NAMESPACE_OPENDAQ

class DeserializeFactoryRegistry
{
public:
    static DeserializeFactoryRegistry& get()
    {
        static DeserializeFactoryRegistry instance;
        return instance;
    }

    ErrCode registerFactory(ConstCharPtr id, daqDeserializerFactory factory)
    {
        try
        {
            factories.insert({id, factory});
        }
        catch (...)
        {
            return OPENDAQ_ERR_FACTORY_NOT_REGISTERED;
        }
        return OPENDAQ_SUCCESS;
    }

    ErrCode unregisterFactory(ConstCharPtr id)
    {
        try
        {
            auto iter = factories.find(id);

            if (iter == factories.cend())
            {
                return OPENDAQ_ERR_FACTORY_NOT_REGISTERED;
            }

            factories.erase(iter);
        }
        catch (...)
        {
            return OPENDAQ_ERR_GENERALERROR;
        }

        return OPENDAQ_SUCCESS;
    }

    ErrCode getFactory(ConstCharPtr id, daqDeserializerFactory* factory) const
    {
        auto iter = factories.find(id);

        if (iter == factories.cend())
        {
            return OPENDAQ_ERR_FACTORY_NOT_REGISTERED;
        }

        *factory = *iter->second;

        return OPENDAQ_SUCCESS;
    }

private:
    DeserializeFactoryRegistry()
    {
    }

    std::unordered_map<std::string, daqDeserializerFactory> factories;
};

ErrCode PUBLIC_EXPORT daqRegisterSerializerFactory(ConstCharPtr id, daqDeserializerFactory factory)
{
    return DeserializeFactoryRegistry::get().registerFactory(id, factory);
}

ErrCode PUBLIC_EXPORT daqUnregisterSerializerFactory(ConstCharPtr id)
{
    return DeserializeFactoryRegistry::get().unregisterFactory(id);
}

ErrCode PUBLIC_EXPORT daqGetSerializerFactory(ConstCharPtr id, daqDeserializerFactory* factory)
{
    return DeserializeFactoryRegistry::get().getFactory(id, factory);
}

END_NAMESPACE_OPENDAQ
