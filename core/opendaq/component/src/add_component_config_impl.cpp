#include <opendaq/add_component_config.h>
#include <opendaq/add_component_config_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class AddComponentConfigImpl : public GenericAddComponentConfigImpl<IAddComponentConfig>
{
public:
    using Super = GenericAddComponentConfigImpl<IAddComponentConfig>;

    explicit AddComponentConfigImpl(const ListPtr<IString>& transports = nullptr)
        : Super(transports)
    {
    }

    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override
    {
        OPENDAQ_PARAM_NOT_NULL(cloned);

        auto obj = createWithImplementation<IAddComponentConfig, AddComponentConfigImpl>();

        return daqTry([this, &obj, &cloned]
        {
            auto implPtr = dynamic_cast<AddComponentConfigImpl*>(obj.getObject());
            implPtr->configureClonedMembers(getCloneParameters());

            *cloned = obj.detach();
            return OPENDAQ_SUCCESS;
        });
    }

    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override
    {
        OPENDAQ_PARAM_NOT_NULL(id);

        *id = SerializeId();
        return OPENDAQ_SUCCESS;
    }

    static ConstCharPtr SerializeId()
    {
        return "AddComponentConfig";
    }

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
    {
        OPENDAQ_PARAM_NOT_NULL(obj);

        return daqTry([&obj, &serialized, &context, &factoryCallback]
        {
            // The "Transport" selection property comes back with the serialized local properties
            *obj = Super::DeserializePropertyObject(
                       serialized,
                       context,
                       factoryCallback,
                       [](const SerializedObjectPtr& /*serialized*/, const BaseObjectPtr& /*context*/, const StringPtr& /*className*/)
                       {
                           return createWithImplementation<IAddComponentConfig, AddComponentConfigImpl>();
                       })
                       .detach();
        });
    }
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AddComponentConfigImpl)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AddComponentConfig, IAddComponentConfig,
    IList*, transports
)

END_NAMESPACE_OPENDAQ
