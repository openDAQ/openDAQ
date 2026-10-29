#include <opendaq/add_streaming_options_impl.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/eval_value_factory.h>
#include <algorithm>

BEGIN_NAMESPACE_OPENDAQ

namespace add_streaming_options
{
static const char* StreamingType = "StreamingType";
static const char* StreamingMode = "StreamingMode";

static const char* PropValuesKey = "propValues";

StringPtr toString(daq::StreamingMode mode)
{
    switch (mode)
    {
        case daq::StreamingMode::Disabled:
            return "Disabled";
        case daq::StreamingMode::MinHops:
            return "MinHops";
        case daq::StreamingMode::MinConnections:
            return "MinConnections";
    }

    DAQ_THROW_EXCEPTION(InvalidParameterException, "Unknown streaming mode {}", static_cast<Int>(mode));
}

bool isListable(const StringPtr& typeId)
{
    const std::string id = typeId;
    return !id.empty() && id != StreamingType && id != StreamingMode && id.find_first_of(".'") == std::string::npos;
}

ListPtr<IString> streamingModeNames()
{
    return List<IString>(
        toString(daq::StreamingMode::Disabled), toString(daq::StreamingMode::MinHops), toString(daq::StreamingMode::MinConnections));
}
}

AddStreamingOptionsImpl::AddStreamingOptionsImpl(const DictPtr<IString, IAddStreamingConfig>& streamingConfigs)
    : Super()
{
    auto selectionValues = List<IString>("");
    for (const auto& [typeId, config] : streamingConfigs)
    {
        if (add_streaming_options::isListable(typeId))
            selectionValues.pushBack(typeId);
    }

    Super::addProperty(StringPropertyBuilder(add_streaming_options::StreamingType, "").setSelectionValues(selectionValues).build());
    Super::addProperty(SelectionProperty(
        add_streaming_options::StreamingMode, add_streaming_options::streamingModeNames(), static_cast<Int>(StreamingMode::MinHops)));

    for (const auto& typeId : offeredTypeIds())
    {
        const auto config = streamingConfigs.get(typeId);
        const auto visible = EvalValue(fmt::format("${} == '{}'", add_streaming_options::StreamingType, typeId));
        Super::addProperty(ObjectPropertyBuilder(typeId, config).setVisible(visible).build());
    }
}

AddStreamingOptionsImpl::AddStreamingOptionsImpl(Partial)
    : Super()
{
}

ErrCode AddStreamingOptionsImpl::getStreamingConfigs(IDict** configs)
{
    OPENDAQ_PARAM_NOT_NULL(configs);

    return daqTry([&]
    {
        auto dict = Dict<IString, IAddStreamingConfig>();
        for (const auto& typeId : offeredTypeIds())
            dict.set(typeId, objPtr.getPropertyValue(typeId).asPtr<IAddStreamingConfig>());

        *configs = dict.detach();
    });
}

ErrCode AddStreamingOptionsImpl::setStreamingType(IString* typeId)
{
    const StringPtr value = typeId ? StringPtr(typeId) : StringPtr("");
    return Super::setPropertyValue(String(add_streaming_options::StreamingType), value);
}

ErrCode AddStreamingOptionsImpl::getStreamingConfig(IAddStreamingConfig** config, IString* typeId)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    return daqTry([&]
    {
        StringPtr id = typeId;
        if (!id.assigned())
        {
            id = objPtr.getPropertyValue(add_streaming_options::StreamingType);
            if (id.getLength() == 0)
            {
                *config = nullptr;
                return;
            }
        }
        else if (!isOffered(id))
        {
            DAQ_THROW_EXCEPTION(NotFoundException, "Streaming type \"{}\" is not offered", id);
        }

        *config = objPtr.getPropertyValue(id).asPtr<IAddStreamingConfig>().detach();
    });
}

ErrCode AddStreamingOptionsImpl::getStreamingMode(StreamingMode* mode)
{
    OPENDAQ_PARAM_NOT_NULL(mode);

    return daqTry([&]
    {
        const Int value = objPtr.getPropertyValue(add_streaming_options::StreamingMode);
        *mode = static_cast<StreamingMode>(value);
    });
}

ErrCode AddStreamingOptionsImpl::setStreamingMode(StreamingMode mode)
{
    return Super::setPropertyValue(String(add_streaming_options::StreamingMode), Integer(static_cast<Int>(mode)));
}

ErrCode AddStreamingOptionsImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    return daqTry([&]
    {
        auto lock = getRecursiveConfigLock2();
        auto obj = createWithImplementation<IPropertyObject, AddStreamingOptionsImpl>(Partial());
        dynamic_cast<AddStreamingOptionsImpl*>(obj.getObject())->configureClonedMembers(getCloneParameters());
        *cloned = obj.detach();
    });
}

ErrCode AddStreamingOptionsImpl::updateInternal(ISerializedObject* obj, IBaseObject* context)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    Bool isFrozenObj;
    OPENDAQ_RETURN_IF_FAILED(isFrozen(&isFrozenObj));
    if (isFrozenObj)
        return OPENDAQ_IGNORED;

    return daqTry([&]
    {
        const auto saved = SerializedObjectPtr::Borrow(obj);
        const auto values = saved.readSerializedObject(add_streaming_options::PropValuesKey);

        StringPtr selected = "";
        if (values.hasKey(add_streaming_options::StreamingType))
            selected = values.readString(add_streaming_options::StreamingType);
        checkErrorInfo(setStreamingType(isOffered(selected) ? selected : StringPtr("")));

        if (values.hasKey(add_streaming_options::StreamingMode))
            checkErrorInfo(setStreamingMode(static_cast<StreamingMode>(values.readInt(add_streaming_options::StreamingMode))));
        else
            checkErrorInfo(Super::clearPropertyValue(String(add_streaming_options::StreamingMode)));

        for (const auto& typeId : offeredTypeIds())
        {
            if (!values.hasKey(typeId))
                continue;

            const auto streamingConfig = objPtr.getPropertyValue(typeId).asPtr<IUpdatable>();
            checkErrorInfo(streamingConfig->updateInternal(values.readSerializedObject(typeId), context));
        }
    });
}

ErrCode AddStreamingOptionsImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr AddStreamingOptionsImpl::SerializeId()
{
    return "AddStreamingOptions";
}

ErrCode AddStreamingOptionsImpl::Deserialize(ISerializedObject* serialized,
                                             IBaseObject* context,
                                             IFunction* factoryCallback,
                                             IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&]
    {
        const auto createOptions = [](const SerializedObjectPtr&, const BaseObjectPtr&, const StringPtr&)
        {
            return createWithImplementation<IAddStreamingOptions, AddStreamingOptionsImpl>(Partial());
        };

        *obj = Super::DeserializePropertyObject(serialized, context, factoryCallback, createOptions).detach();
    });
}

ListPtr<IString> AddStreamingOptionsImpl::offeredTypeIds()
{
    const ListPtr<IString> selectionValues = objPtr.getProperty(add_streaming_options::StreamingType).getSelectionValues();
    auto typeIds = List<IString>();
    for (const auto& typeId : selectionValues)
    {
        if (typeId.getLength() > 0)
            typeIds.pushBack(typeId);
    }

    return typeIds;
}

bool AddStreamingOptionsImpl::isOffered(const StringPtr& typeId)
{
    const auto typeIds = offeredTypeIds();
    return std::find(typeIds.begin(), typeIds.end(), typeId) != typeIds.end();
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AddStreamingOptionsImpl)

END_NAMESPACE_OPENDAQ
