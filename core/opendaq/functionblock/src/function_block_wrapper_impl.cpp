#include <opendaq/function_block_wrapper_impl.h>
#include <coretypes/validation.h>
#include <opendaq/property_wrapper_impl.h>

BEGIN_NAMESPACE_OPENDAQ

// TODO properly extract parent globalId for global id

FunctionBlockWrapperImpl::FunctionBlockWrapperImpl(const FunctionBlockPtr& functionBlock,
                                                   bool includeInputPortsByDefault,
                                                   bool includeSignalsByDefault,
                                                   bool includePropertiesByDefault,
                                                   bool includeFunctionBlocksByDefault)
    : Super(functionBlock.getFunctionBlockType(), functionBlock.getContext(), functionBlock.getParent(), functionBlock.getLocalId())
    , functionBlock(functionBlock)
    , includeInputPortsByDefault(includeInputPortsByDefault)
    , includeSignalsByDefault(includeSignalsByDefault)
    , includePropertiesByDefault(includePropertiesByDefault)
    , includeFunctionsBlocksByDefault(includeFunctionBlocksByDefault)
{
}

ErrCode FunctionBlockWrapperImpl::includeObject(IString* objectName,
                                                std::unordered_set<std::string>& includedObjects,
                                                std::unordered_set<std::string>& excludedObjects,
                                                bool includeObjectsByDefault)
{
    const auto objectNameStr = StringPtr::Borrow(objectName).toStdString();

    auto lock = this->getRecursiveConfigLock();

    if (includeObjectsByDefault)
    {
        const auto it = excludedObjects.find(objectNameStr);
        if (it != excludedObjects.end())
        {
            excludedObjects.erase(it);
            return OPENDAQ_SUCCESS;
        }

        return OPENDAQ_IGNORED;
    }

    const auto it = includedObjects.find(objectNameStr);

    if (it != includedObjects.end())
        return OPENDAQ_IGNORED;

    includedObjects.insert(objectNameStr);

    return OPENDAQ_SUCCESS;
}

ErrCode FunctionBlockWrapperImpl::excludeObject(IString* objectName,
                                                               std::unordered_set<std::string>& includedObjects,
                                                               std::unordered_set<std::string>& excludedObjects,
                                                               bool includeObjectsByDefault)
{
    const auto objectNameStr = StringPtr::Borrow(objectName).toStdString();

    auto lock = this->getRecursiveConfigLock();

    if (!includeObjectsByDefault)
    {
        const auto it = includedObjects.find(objectNameStr);
        if (it != includedObjects.end())
        {
            includedObjects.erase(it);
            return OPENDAQ_SUCCESS;
        }

        return OPENDAQ_IGNORED;
    }

    const auto it = excludedObjects.find(objectNameStr);

    if (it != excludedObjects.end())
        return OPENDAQ_IGNORED;

    excludedObjects.insert(objectNameStr);

    return OPENDAQ_SUCCESS;
}


ErrCode FunctionBlockWrapperImpl::includeInputPort(IString* inputPortName)
{
    OPENDAQ_PARAM_NOT_NULL(inputPortName);

    return includeObject(inputPortName, includedInputPorts, excludedInputPorts, includeInputPortsByDefault);
}

ErrCode FunctionBlockWrapperImpl::excludeInputPort(IString* inputPortName)
{
    OPENDAQ_PARAM_NOT_NULL(inputPortName);

    return excludeObject(inputPortName, includedInputPorts, excludedInputPorts, includeInputPortsByDefault);
}

ErrCode FunctionBlockWrapperImpl::includeSignal(IString* signalLocalId)
{
    OPENDAQ_PARAM_NOT_NULL(signalLocalId);

    return includeObject(signalLocalId, includedSignals, excludedSignals, includeSignalsByDefault);
}

ErrCode FunctionBlockWrapperImpl::excludeSignal(IString* signalLocalId)
{
    OPENDAQ_PARAM_NOT_NULL(signalLocalId);

    return excludeObject(signalLocalId, includedSignals, excludedSignals, includeSignalsByDefault);
}

ErrCode FunctionBlockWrapperImpl::includeProperty(IString* propertyName)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    return includeObject(propertyName, includedProperties, excludedProperties, includePropertiesByDefault);
}

ErrCode FunctionBlockWrapperImpl::excludeProperty(IString* propertyName)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    return excludeObject(propertyName, includedProperties, excludedProperties, includePropertiesByDefault);
}

ErrCode FunctionBlockWrapperImpl::includeFunctionBlock(IString* functionBlockLocalId)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlockLocalId);

    return includeObject(functionBlockLocalId, includedFbs, excludedFbs, includeFunctionsBlocksByDefault);
}

ErrCode FunctionBlockWrapperImpl::excludeFunctionBlock(IString* functionBlockLocalId)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlockLocalId);

    return excludeObject(functionBlockLocalId, includedFbs, excludedFbs, includeFunctionsBlocksByDefault);
}

ErrCode FunctionBlockWrapperImpl::setPropertyCoercer(IString* propertyName, ICoercer* coercer)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    return setOverridenObject(propertyName, coercers, coercer);
}

ErrCode FunctionBlockWrapperImpl::setPropertyValidator(IString* propertyName, IValidator* validator)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    return setOverridenObject(propertyName, validators, validator);
}

ErrCode FunctionBlockWrapperImpl::setPropertySelectionValues(IString* propertyName, IList* enumValues)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    auto lock = this->getRecursiveConfigLock();

    return wrapHandler(
        [this, &propertyName, &enumValues]()
        {
            const auto propertyNameStr = StringPtr::Borrow(propertyName);

            if (!isPropertyVisible(propertyNameStr))
                throw NotFoundException();

            if (!functionBlock.hasProperty(propertyNameStr))
                throw NotFoundException();

            const auto enumValuesPtr = ListPtr<IInteger>::Borrow(enumValues);

            if (enumValuesPtr.assigned())
            {
                std::unordered_set<size_t> evl;
                for (const auto& v : enumValuesPtr)
                    evl.insert(v);
                enumValuesMap.insert_or_assign(propertyNameStr, std::move(evl));
            }
            else
                enumValuesMap.erase(propertyNameStr);
        });
}

ErrCode FunctionBlockWrapperImpl::getWrappedFunctionBlock(IFunctionBlock** functionBlock)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlock);

    *functionBlock = this->functionBlock.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

template <class TInterface, class GetName>
ListPtr<TInterface> FunctionBlockWrapperImpl::getObjects(
    const ListPtr<TInterface> innerObjects,
    std::unordered_set<std::string>& includedObjects,
    std::unordered_set<std::string>& excludedObjects,
    bool includeObjectsByDefault, GetName&& getName)
{
    auto objectList = List<TInterface>();

    for (const auto& object : innerObjects)
    {
        std::string objectName = getName(object);
        if (includeObjectsByDefault)
        {
            if (excludedObjects.find(objectName) == excludedObjects.end())
                objectList.pushBack(object);
        }
        else
        {
            if (includedObjects.find(objectName) != includedObjects.end())
                objectList.pushBack(object);
        }
    }

    return objectList;
}

ErrCode FunctionBlockWrapperImpl::getInputPorts(IList** ports, ISearchFilter* searchFilter)
{
    OPENDAQ_PARAM_NOT_NULL(ports);

    const auto innerPorts = functionBlock.getInputPorts(searchFilter);

    auto lock = this->getRecursiveConfigLock();

    auto portList = getObjects<IInputPort>(innerPorts,
                                                 includedInputPorts,
                                                 excludedInputPorts,
                                                 includeInputPortsByDefault,
                                                 [](const InputPortPtr& port) { return port.getLocalId(); });
    *ports = portList.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode FunctionBlockWrapperImpl::getSignals(IList** signals, ISearchFilter* searchFilter)
{
    OPENDAQ_PARAM_NOT_NULL(signals);

    const auto innerSignals = functionBlock.getSignals(searchFilter);

    auto lock = this->getRecursiveConfigLock();

    auto signalList = getObjects<ISignal>(innerSignals,
                                           includedSignals,
                                           excludedSignals,
                                           includeSignalsByDefault,
                                           [](const SignalPtr& signal) { return signal.getLocalId(); });
    *signals = signalList.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode FunctionBlockWrapperImpl::getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlocks);

    const auto innerFbs = functionBlock.getFunctionBlocks(searchFilter);

    auto lock = this->getRecursiveConfigLock();

    auto fbList = getObjects<IFunctionBlock>(
        innerFbs,
     includedFbs,
     excludedFbs,
        includeFunctionsBlocksByDefault,
        [](const FunctionBlockPtr& fb) { return fb.getLocalId(); });
    *functionBlocks = fbList.detach();
    return OPENDAQ_SUCCESS;
}

bool FunctionBlockWrapperImpl::isSelectionAvailable(const StringPtr& propertyNameStr, const BaseObjectPtr& value)
{
    const auto prop = functionBlock.getProperty(propertyNameStr);
    if (prop.getValueType() == ctInt && prop.getSelectionValues().assigned())
    {
        const auto eIt = enumValuesMap.find(propertyNameStr);
        if (eIt != enumValuesMap.end())
        {
            const auto val = static_cast<size_t>(static_cast<Int>(BaseObjectPtr(value)));
            const auto vIt = eIt->second.find(val);
            if (vIt == eIt->second.end())
                return false;
        }
    }
    return true;
}

ErrCode FunctionBlockWrapperImpl::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    auto propertyNameStr = StringPtr::Borrow(propertyName);

    auto lock = this->getRecursiveConfigLock();

    return wrapHandler(
        [this, &propertyNameStr, &value]()
        {
            auto valuePtr = BaseObjectPtr::Borrow(value);

            StringPtr childName;
            StringPtr subName;
            if (isChildProperty(propertyNameStr, childName, subName))
            {
                if (!isPropertyVisible(childName))
                    throw NotFoundException();
            }
            else
            {
                if (!isPropertyVisible(propertyNameStr))
                    throw NotFoundException();

                if (!isSelectionAvailable(propertyNameStr, valuePtr))
                    throw NotFoundException("Selection value not available");

                auto cIt = coercers.find(propertyNameStr);
                if (cIt != coercers.end())
                {
                    auto coercer = cIt->second;
                    valuePtr = coercer.coerce(borrowPtr<PropertyObjectPtr>(), valuePtr);
                }

                auto it = validators.find(propertyNameStr);
                if (it != validators.end())
                {
                    auto validator = it->second;
                    validator.validate(borrowPtr<PropertyObjectPtr>(), valuePtr);
                }
            }

            functionBlock.setPropertyValue(propertyNameStr, valuePtr);
        });
}

ErrCode FunctionBlockWrapperImpl::getPropertyValue(IString* propertyName, IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    auto lock = this->getRecursiveConfigLock();

    if (isPropertyVisible(propertyName))
        return functionBlock->getPropertyValue(propertyName, value);

    return OPENDAQ_ERR_NOTFOUND;
}

ErrCode FunctionBlockWrapperImpl::getPropertySelectionValue(IString* propertyName, IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    auto lock = this->getRecursiveConfigLock();

    if (isPropertyVisible(propertyName))
        return functionBlock->getPropertySelectionValue(propertyName, value);

    return OPENDAQ_ERR_NOTFOUND;
}

ErrCode FunctionBlockWrapperImpl::clearPropertyValue(IString* propertyName)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    auto lock = this->getRecursiveConfigLock();

    if (isPropertyVisible(propertyName))
        return functionBlock->clearPropertyValue(propertyName);

    return OPENDAQ_ERR_NOTFOUND;
}

ErrCode FunctionBlockWrapperImpl::hasProperty(IString* propertyName, Bool* hasProperty)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    auto lock = this->getRecursiveConfigLock();

    if (isPropertyVisible(propertyName))
        return functionBlock->hasProperty(propertyName, hasProperty);

    *hasProperty = False;
    return OPENDAQ_SUCCESS;
}

PropertyPtr FunctionBlockWrapperImpl::wrapProperty(const PropertyPtr& property)
{
    const auto it = enumValuesMap.find(property.getName());
    if (it != enumValuesMap.end())
    {
        auto wrappedProperty = createWithImplementation<IProperty, PropertyWrapperImpl>(property, it->second);
        return wrappedProperty;
    }

    return property;
}

PropertyPtr FunctionBlockWrapperImpl::wrapProperty(const StringPtr& propertyName)
{
    const auto prop = functionBlock.getProperty(propertyName);
    return wrapProperty(prop);
}

ListPtr<IProperty> FunctionBlockWrapperImpl::wrapProperties(const ListPtr<IProperty>& properties)
{
    auto wrappedProps = List<IProperty>();
    for (const auto& prop: properties)
        wrappedProps.pushBack(wrapProperty(prop));
    return wrappedProps;
}


ErrCode FunctionBlockWrapperImpl::getProperty(IString* propertyName, IProperty** property)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    auto propertyNamePtr = StringPtr::Borrow(propertyName);

    auto lock = this->getRecursiveConfigLock();

    return wrapHandler(
        [this, &propertyNamePtr, &property]()
        {
            if (isPropertyVisible(propertyNamePtr))
            {
                auto prop = wrapProperty(propertyNamePtr);

                *property = prop.detach();
            }
            else
                throw NotFoundException();
        });
}

ErrCode FunctionBlockWrapperImpl::addProperty(IProperty* property)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode FunctionBlockWrapperImpl::removeProperty(IString* propertyName)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode FunctionBlockWrapperImpl::getProperties(const ListPtr<IProperty>& innerProperties, IList** properties)
{
    assert(properties != nullptr);

    auto lock = this->getRecursiveConfigLock();

    const auto propertyList = getObjects<IProperty>(innerProperties,
                                                    includedProperties,
                                                    excludedProperties,
                                                    includePropertiesByDefault,
                                                    [](const PropertyPtr& property) { return property.getName(); });

    *properties = wrapProperties(propertyList).detach();
    return OPENDAQ_SUCCESS;
}

bool FunctionBlockWrapperImpl::isPropertyVisible(IString* propertyName)
{
    assert(propertyName != nullptr);

    const auto propertyNameStr = StringPtr::Borrow(propertyName).toStdString();

    if (includePropertiesByDefault)
    {
        if (excludedProperties.find(propertyNameStr) == excludedProperties.end())
            return true;
        return false;
    }

    if (includedProperties.find(propertyNameStr) != includedProperties.end())
        return true;
    return false;
}

ErrCode FunctionBlockWrapperImpl::getVisibleProperties(IList** properties)
{
    OPENDAQ_PARAM_NOT_NULL(properties);

    const auto innerProperties = functionBlock.getVisibleProperties();
    return getProperties(innerProperties, properties);
}

ErrCode FunctionBlockWrapperImpl::getAllProperties(IList** properties)
{
    OPENDAQ_PARAM_NOT_NULL(properties);

    const auto innerProperties = functionBlock.getAllProperties();
    return getProperties(innerProperties, properties);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY,
    FunctionBlockWrapper,
    IFunctionBlock,
    IFunctionBlock*, functionBlock,
    Bool, includeInputPortsByDefault,
    Bool, includeSignalsByDefault,
    Bool, includePropertiesByDefault,
    Bool, includeFunctionBlocksByDefault)

END_NAMESPACE_OPENDAQ
