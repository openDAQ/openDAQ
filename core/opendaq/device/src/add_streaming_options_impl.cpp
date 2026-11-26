#include <opendaq/add_streaming_options_impl.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/eval_value_factory.h>
#include <opendaq/streaming_type_utils.h>
#include <algorithm>

BEGIN_NAMESPACE_OPENDAQ

namespace add_streaming_options
{
static const char* StreamingType = "StreamingType";
static const char* StreamingMode = "StreamingMode";
static const char* NoStreamingType = "None";

static const char* PropValuesKey = "propValues";
static const char* TrimUnselectedConfigsKey = "trimUnselected";
static const char* TrimExceptionsKey = "trimExceptions";

StringPtr toString(daq::StreamingMode mode)
{
    switch (mode)
    {
        case daq::StreamingMode::MinHops:
            return "MinHops";
        case daq::StreamingMode::MinConnections:
            return "MinConnections";
    }

    DAQ_THROW_EXCEPTION(InvalidParameterException, "Unknown streaming mode {}", static_cast<Int>(mode));
}

DictPtr<Int, IString> streamingModeNames()
{
    auto names = Dict<Int, IString>();
    for (const auto mode : {daq::StreamingMode::MinHops, daq::StreamingMode::MinConnections})
        names.set(static_cast<Int>(mode), toString(mode));
    return names;
}
}

AddStreamingOptionsImpl::AddStreamingOptionsImpl(const DictPtr<IString, IAddStreamingConfig>& streamingConfigs)
    : Super()
{
    auto selectionValues = List<IString>(add_streaming_options::NoStreamingType);
    for (const auto& [typeId, config] : streamingConfigs)
    {
        if (isStreamingTypeIdReserved(typeId))
            DAQ_THROW_EXCEPTION(InvalidParameterException, R"(Streaming type ID "{}" is reserved)", typeId);

        selectionValues.pushBack(typeId);
    }

    const auto addOptionsProperty = [this](const PropertyBuilderPtr& builder)
    {
        builder.setVisible(
            EvalValue(fmt::format("${} != '{}'", add_streaming_options::StreamingType, add_streaming_options::NoStreamingType)));
        checkErrorInfo(Super::addProperty(builder.build()));
    };

    Super::addProperty(StringPropertyBuilder(add_streaming_options::StreamingType, add_streaming_options::NoStreamingType)
                           .setSelectionValues(selectionValues)
                           .build());
    addOptionsProperty(SparseSelectionPropertyBuilder(
        add_streaming_options::StreamingMode, add_streaming_options::streamingModeNames(), static_cast<Int>(StreamingMode::MinHops)));

    for (const auto& [typeId, config] : streamingConfigs)
    {
        const auto visible = EvalValue(fmt::format("${} == '{}'", add_streaming_options::StreamingType, typeId));
        Super::addProperty(ObjectPropertyBuilder(typeId, config).setVisible(visible).build());
    }
}

AddStreamingOptionsImpl::AddStreamingOptionsImpl(Partial)
    : Super()
{
}

ErrCode AddStreamingOptionsImpl::getAvailableStreamingConfigs(IDict** configs)
{
    OPENDAQ_PARAM_NOT_NULL(configs);

    return daqTry([&]
    {
        auto dict = Dict<IString, IAddStreamingConfig>();
        for (const auto& typeId : offeredTypeIds())
            dict.set(typeId, getConfig(typeId));

        *configs = dict.detach();
    });
}

ErrCode AddStreamingOptionsImpl::getStreamingType(IString** typeId)
{
    OPENDAQ_PARAM_NOT_NULL(typeId);

    return daqTry([&]
    {
        StringPtr id = objPtr.getPropertyValue(add_streaming_options::StreamingType);
        *typeId = (id == add_streaming_options::NoStreamingType) ? nullptr : id.detach();
    });
}

ErrCode AddStreamingOptionsImpl::setStreamingType(IString* typeId)
{
    const StringPtr id = typeId;
    const StringPtr value = (id.assigned() && id.getLength() > 0) ? id : StringPtr(add_streaming_options::NoStreamingType);
    return Super::setPropertyValue(String(add_streaming_options::StreamingType), value);
}

ErrCode AddStreamingOptionsImpl::getStreamingConfig(IAddStreamingConfig** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    return daqTry([&]
    {
        StringPtr id;
        checkErrorInfo(getStreamingType(&id));
        *config = id.assigned() ? getConfig(id).detach() : nullptr;
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
        const auto impl = dynamic_cast<AddStreamingOptionsImpl*>(obj.getObject());
        impl->configureClonedMembers(getCloneParameters());
        impl->trimData = trimData;
        *cloned = obj.detach();
    });
}

void AddStreamingOptionsImpl::removeUnselected()
{
    const PropertyPtr selectionProperty = objPtr.getProperty(add_streaming_options::StreamingType);
    const StringPtr defaultSelection = selectionProperty.getDefaultValue();
    const StringPtr selection = objPtr.getPropertyValue(add_streaming_options::StreamingType);
    const auto isKept = [&](const StringPtr& typeId)
    {
        if (typeId == defaultSelection || typeId == selection)
            return true;

        const bool isException = std::find(trimData.exceptions.begin(), trimData.exceptions.end(), typeId) != trimData.exceptions.end();
        return trimData.trimUnselected == isException;
    };

    const ListPtr<IString> offered = selectionProperty.getSelectionValues();
    auto selectionValues = List<IString>();
    for (const auto& typeId : offered)
    {
        if (isKept(typeId))
            selectionValues.pushBack(typeId);
        else if (getConfig(typeId).assigned())
            checkErrorInfo(Super::removeProperty(typeId));
    }

    if (selectionValues.getCount() == offered.getCount())
        return;

    checkErrorInfo(Super::removeProperty(String(add_streaming_options::StreamingType)));
    checkErrorInfo(Super::addProperty(
        StringPropertyBuilder(add_streaming_options::StreamingType, defaultSelection).setSelectionValues(selectionValues).build()));
    checkErrorInfo(Super::setPropertyValue(String(add_streaming_options::StreamingType), selection));
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

        const auto selected = SavedStreamingType(saved);
        checkErrorInfo(setStreamingType(isOffered(selected) ? selected : nullptr));

        if (values.hasKey(add_streaming_options::StreamingMode))
            checkErrorInfo(setStreamingMode(static_cast<StreamingMode>(values.readInt(add_streaming_options::StreamingMode))));
        else
            checkErrorInfo(Super::clearPropertyValue(String(add_streaming_options::StreamingMode)));

        for (const auto& typeId : offeredTypeIds(true))
        {
            if (!values.hasKey(typeId))
                continue;

            const auto streamingConfig = objPtr.getPropertyValue(typeId).asPtr<IUpdatable>();
            checkErrorInfo(streamingConfig->updateInternal(values.readSerializedObject(typeId), context));
        }
    });
}

ErrCode AddStreamingOptionsImpl::serializeCustomValues(ISerializer* serializer, bool forUpdate)
{
    OPENDAQ_RETURN_IF_FAILED(Super::serializeCustomValues(serializer, forUpdate));
    if (forUpdate)
        return OPENDAQ_SUCCESS;

    return daqTry([&]
    {
        const auto serializerPtr = SerializerPtr::Borrow(serializer);
        if (!trimData.trimUnselected)
        {
            serializerPtr.key(add_streaming_options::TrimUnselectedConfigsKey);
            serializerPtr.writeBool(trimData.trimUnselected);
        }

        if (trimData.exceptions.getCount() > 0)
        {
            serializerPtr.key(add_streaming_options::TrimExceptionsKey);
            trimData.exceptions.serialize(serializerPtr);
        }
    });
}

ErrCode AddStreamingOptionsImpl::serializePropertyValue(const StringPtr& name,
                                                        const ObjectPtr<IBaseObject>& value,
                                                        ISerializer* serializer,
                                                        bool forUpdate)
{
    if (!forUpdate || !value.supportsInterface<IAddStreamingConfig>())
        return Super::serializePropertyValue(name, value, serializer, forUpdate);

    StringPtr selected;
    OPENDAQ_RETURN_IF_FAILED(getStreamingType(&selected));
    if (name != selected)
        return OPENDAQ_SUCCESS;

    return Super::serializePropertyValue(name, value, serializer, forUpdate);
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
        const auto createOptions = [](const SerializedObjectPtr& serialized, const BaseObjectPtr& context, const StringPtr&)
        {
            auto options = createWithImplementation<IAddStreamingOptions, AddStreamingOptionsImpl>(Partial());
            const auto impl = dynamic_cast<AddStreamingOptionsImpl*>(options.getObject());
            if (serialized.hasKey(add_streaming_options::TrimUnselectedConfigsKey))
                impl->trimData.trimUnselected = serialized.readBool(add_streaming_options::TrimUnselectedConfigsKey);
            if (serialized.hasKey(add_streaming_options::TrimExceptionsKey))
                impl->trimData.exceptions = serialized.readObject(add_streaming_options::TrimExceptionsKey, context);
            return options;
        };

        *obj = Super::DeserializePropertyObject(serialized, context, factoryCallback, createOptions).detach();
    });
}

StringPtr AddStreamingOptionsImpl::SavedStreamingType(const SerializedObjectPtr& saved)
{
    const auto values = saved.readSerializedObject(add_streaming_options::PropValuesKey);
    if (!values.hasKey(add_streaming_options::StreamingType))
        return add_streaming_options::NoStreamingType;

    return values.readString(add_streaming_options::StreamingType);
}

ListPtr<IString> AddStreamingOptionsImpl::offeredTypeIds(bool withConfig)
{
    const ListPtr<IString> selectionValues = objPtr.getProperty(add_streaming_options::StreamingType).getSelectionValues();
    auto typeIds = List<IString>();
    for (const auto& typeId : selectionValues)
    {
        if (typeId != add_streaming_options::NoStreamingType && (!withConfig || getConfig(typeId).assigned()))
            typeIds.pushBack(typeId);
    }

    return typeIds;
}

AddStreamingConfigPtr AddStreamingOptionsImpl::getConfig(const StringPtr& typeId)
{
    if (!objPtr.hasProperty(typeId))
        return nullptr;

    return objPtr.getPropertyValue(typeId).asPtrOrNull<IAddStreamingConfig>();
}

bool AddStreamingOptionsImpl::isOffered(const StringPtr& typeId)
{
    const auto typeIds = offeredTypeIds();
    return std::find(typeIds.begin(), typeIds.end(), typeId) != typeIds.end();
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AddStreamingOptionsImpl)

END_NAMESPACE_OPENDAQ
