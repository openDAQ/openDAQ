#include <coreobjects/eval_value_impl.h>
#include <coreobjects/eval_value_parser.h>
#include <functional>
#include <coreobjects/eval_value_ptr.h>
#include <coreobjects/property_object_internal_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

EvalValueImpl::EvalValueImpl(IString* eval)
    : eval(eval)
    , node(nullptr)
    , resolveStatus(ResolveStatus::Unresolved)
    , parseErrCode(OPENDAQ_SUCCESS)
    , calculated(false)
    , useFunctionResolver(false)
{
    onCreate();
}

EvalValueImpl::EvalValueImpl(IString* eval, IFunction* func)
    : eval(eval)
    , node(nullptr)
    , resolveStatus(ResolveStatus::Unresolved)
    , parseErrCode(OPENDAQ_SUCCESS)
    , calculated(false)
    , useFunctionResolver(true)
    , func(func)
{
    onCreate();
}

EvalValueImpl::EvalValueImpl(IString* eval, ListPtr<IBaseObject> arguments)
    : eval(eval)
    , node(nullptr)
    , arguments(std::move(arguments))
    , resolveStatus(ResolveStatus::Unresolved)
    , parseErrCode(OPENDAQ_SUCCESS)
    , calculated(false)
    , useFunctionResolver(false)
{
    onCreate();
}

void EvalValueImpl::internalDispose(bool disposing)
{
    if (disposing)
    {
        eval.release();
        owner.release();
    }
}

EvalValueImpl::EvalValueImpl(const EvalValueImpl& ev, IPropertyObject* owner)
    : eval(ev.eval)
    , resolveStatus(ResolveStatus::Unresolved)
    , parseErrCode(ev.parseErrCode)
    , calculated(false)
    , useFunctionResolver(false)
{
    using namespace std::placeholders;

    this->owner = owner;
    node = ev.node->clone([this](const std::string& str, RefType refType, int argIndex, std::string& postRef, bool lock)
    {
        return getReference(str, refType, argIndex, postRef, lock);
    });

    std::unordered_set<std::string> refs;
    for (auto ref : *ev.propertyReferences)
        refs.insert(ref);
    propertyReferences = std::make_unique<std::unordered_set<std::string>>(std::move(refs));
}

EvalValueImpl::EvalValueImpl(const EvalValueImpl& ev, IPropertyObject* owner, IFunction* func)
    : eval(ev.eval)
    , resolveStatus(ResolveStatus::Unresolved)
    , parseErrCode(ev.parseErrCode)
    , calculated(false)
    , useFunctionResolver(true)
    , func(func)
{
    using namespace std::placeholders;

    this->owner = owner;
    node = ev.node->clone([this](const std::string& str, RefType refType, int argIndex, std::string& postRef, bool lock)
    {
        return getReference(str, refType, argIndex, postRef, lock);
    });

    std::unordered_set<std::string> refs;
    for (auto ref : *ev.propertyReferences)
        refs.insert(ref);
    propertyReferences = std::make_unique<std::unordered_set<std::string>>(std::move(refs));
}

void EvalValueImpl::onCreate()
{
    using namespace std::placeholders;

    ParseParams params{
        nullptr,
        nullptr,
        useFunctionResolver,
        [this](const std::string& str, RefType refType, int argIndex, std::string& postRef, bool lock)
        {
            return getReference(str, refType, argIndex, postRef, lock);
        }
    };

    ConstCharPtr s;
    parseErrCode = eval->getCharPtr(&s);
    if (OPENDAQ_FAILED(parseErrCode))
        return;

    bool parsed = parseEvalValue(s, &params);
    resolveStatus = ResolveStatus::Unresolved;

    node = std::move(params.node);
    propertyReferences = std::move(params.propertyReferences);

    parseErrCode = parsed ? OPENDAQ_SUCCESS : OPENDAQ_ERR_PARSEFAILED;
    if (!parsed)
        parseErrMessage = params.errMessage;
}

ErrCode EvalValueImpl::setOwner(IPropertyObject* value)
{
    owner = value;
    return OPENDAQ_SUCCESS;
}

// OPENDAQ_TODO: refactor getReference

void EvalValueImpl::checkForEvalValue(BaseObjectPtr& prop) const
{
    EvalValuePtr e = prop.asPtrOrNull<IEvalValue>(true);
    if (e != nullptr)
        prop = e.cloneWithOwner(owner.getRef());
}

BaseObjectPtr EvalValueImpl::getReferenceFromPrefix(const PropertyObjectPtr& propObject, const std::string& str, RefType refType, bool lock) const
{
    BaseObjectPtr value;

    if (refType == RefType::Property)
    {
        value = propObject.getProperty(str);
    }
    else if (refType == RefType::Value)
    {
        value = lock ? propObject.getPropertyValue(str) : propObject.asPtr<IPropertyObjectInternal>().getPropertyValueNoLock(str);
        checkForEvalValue(value);
    }
    else if (refType == RefType::SelectedValue)
    {
        value = lock ? propObject.getPropertySelectionValue(str) : propObject.asPtr<IPropertyObjectInternal>().getPropertySelectionValueNoLock(str);
        checkForEvalValue(value);
    }
    else if (refType == RefType::PropertyNames)
    {
        auto propNames = List<IString>();
        const PropertyObjectPtr child = lock ? propObject.getPropertyValue(str) : propObject.asPtr<IPropertyObjectInternal>().getPropertyValueNoLock(str);
        for (const auto& prop : child.getAllProperties())
        {
            propNames.pushBack(prop.getName());
        }
        value = propNames;
    }
    
    return value;
}

BaseObjectPtr EvalValueImpl::getReference(const std::string& str, RefType refType, int argIndex, std::string& postRef, bool lock) const
{
    if (argIndex > -1)
    {
        if (!arguments.assigned() || argIndex > int(arguments.getCount()))
        {
            return nullptr;
        }

        return getReferenceFromPrefix(arguments[argIndex], str, refType, lock);
    }

    if (refType == RefType::Func)
        return func.call(String(str));

    if (!owner.assigned())
        return nullptr;

    auto pos = str.find(':');

    PropertyObjectPtr ownerRef = owner.getRef();
    if (pos == std::string::npos)
        return getReferenceFromPrefix(ownerRef, str, refType, lock);

    std::string prefix = str.substr(0, pos);
    postRef = str.substr(pos + 1, std::wstring::npos);
#if defined(_WIN32)
    if (_stricmp("value", postRef.c_str()) == 0)
#else
    if (strcasecmp("value", postRef.c_str()) == 0)
#endif
        return getReferenceFromPrefix(ownerRef, prefix, RefType::Value, lock);
#if defined(_WIN32)
    if (_stricmp("selectedvalue", postRef.c_str()) == 0)
#else
    if (strcasecmp("selectedvalue", postRef.c_str()) == 0)
#endif
        return getReferenceFromPrefix(ownerRef, prefix, RefType::SelectedValue, lock);
#if defined(_WIN32)
    if (_stricmp("propertynames", postRef.c_str()) == 0)
#else
    if (strcasecmp("propertynames", postRef.c_str()) == 0)
#endif
        return getReferenceFromPrefix(ownerRef, prefix, RefType::PropertyNames, lock);
    return nullptr;

    /*
    #if defined(_WIN32)
    if (_wcsicmp(L"clear", postRef.c_str()) == 0)
    #else
    if (wcscasecmp(L"clear", postRef.c_str()) == 0)
    #endif
    return owner.getPropertyValue(prefix);

    throw std::invalid_argument("Invalid reference");*/
}

int EvalValueImpl::resolveReferences(bool lock)
{
    assert(node);

    int r = node->visit([lock](BaseNode* input)
    {
        return input->resolveReference(lock);
    });

    resolveStatus = r == 0 ? ResolveStatus::Resolved
                           : ResolveStatus::Failed;
    return r;
}

ErrCode EvalValueImpl::checkParseAndResolve(bool lock)
{
    OPENDAQ_RETURN_IF_FAILED(parseErrCode);
    int r = resolveReferences(lock);
    if (r != 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_RESOLVEFAILED);

    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::getCoreType(CoreType* coreType)
{
    OPENDAQ_PARAM_NOT_NULL(coreType);

    ErrCode err = checkParseAndResolve(false);
    OPENDAQ_RETURN_IF_FAILED(err);

    try
    {
        *coreType = calc().getCoreType();
        return OPENDAQ_SUCCESS;
    }
    catch (...)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_CALCFAILED);
    };
}

ErrCode EvalValueImpl::getEval(IString** evalString)
{
    OPENDAQ_PARAM_NOT_NULL(evalString);

    *evalString = this->eval.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

BaseObjectPtr EvalValueImpl::calc()
{
    calculated = true;
    return node->getResult();
}

ErrCode EvalValueImpl::getResult(IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    ErrCode err = checkParseAndResolve(true);
    OPENDAQ_RETURN_IF_FAILED(err);

    try
    {
        *obj = calc().addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }
    catch (...)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_CALCFAILED);
    };
}

ErrCode EvalValueImpl::getResultNoLock(IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    ErrCode err = checkParseAndResolve(false);
    OPENDAQ_RETURN_IF_FAILED(err);

    try
    {
        *obj = calc().addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }
    catch (...)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_CALCFAILED);
    };
}

template <typename T>
ErrCode EvalValueImpl::equalsValueInternal(const T value, Bool* equals)
{
    if (equals == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Equals output-parameter must not be null.");

    T thisValue;
    ErrCode errCode = getValueInternal<T>(thisValue);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    *equals = value == thisValue;
    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::Float_GetValue(Float* value)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    return getValueInternal<Float>(*value);
}

ErrCode EvalValueImpl::Float_EqualsValue(const Float value, Bool* equals)
{
    return equalsValueInternal<Float>(value, equals);
}

ErrCode EvalValueImpl::Integer_GetValue(Int* value)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    return getValueInternal<Int>(*value);
}

ErrCode EvalValueImpl::Integer_EqualsValue(const Int value, Bool* equals)
{
    return equalsValueInternal<Int>(value, equals);
}

ErrCode EvalValueImpl::Boolean_GetValue(Bool* value)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    return getValueInternal<Bool>(*value);
}

ErrCode EvalValueImpl::Boolean_EqualsValue(const Bool value, Bool* equals)
{
    return equalsValueInternal<Bool>(value, equals);
}

ErrCode EvalValueImpl::StringObject_GetCharPtr(ConstCharPtr* value)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    ErrCode errCode = getValueInternal<std::string>(strResult);
    *value = strResult.c_str();
    return errCode;
}

ErrCode EvalValueImpl::StringObject_GetLength(SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);

    ErrCode errCode = getValueInternal<std::string>(strResult);
    *size = strResult.length();
    return errCode;
}

template <typename T>
ErrCode EvalValueImpl::getValueInternal(T& value)
{
    auto err = checkParseAndResolve(false);
    OPENDAQ_RETURN_IF_FAILED(err);

    try
    {
        value = static_cast<T>(calc());
        return OPENDAQ_SUCCESS;
    }
    catch (...)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_CALCFAILED);
    };
}

ErrCode EvalValueImpl::toString(CharPtr* str)
{
    OPENDAQ_PARAM_NOT_NULL(str);

    ErrCode errCode = getValueInternal<std::string>(strResult);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    const auto len = std::strlen(strResult.c_str()) + 1;
    *str = static_cast<CharPtr>(daqAllocateMemory(len));
    OPENDAQ_PARAM_NOT_NULL(*str);

#if defined(__STDC_SECURE_LIB__) || defined(__STDC_LIB_EXT1__)
    strcpy_s(*str, len, strResult.c_str());
#else
    strncpy(*str, strResult.c_str(), len);
#endif
    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::toFloat(Float* val)
{
    OPENDAQ_PARAM_NOT_NULL(val);

    return getValueInternal<Float>(*val);
}

ErrCode EvalValueImpl::toInt(Int* val)
{
    OPENDAQ_PARAM_NOT_NULL(val);

    return getValueInternal<Int>(*val);
}

ErrCode EvalValueImpl::toBool(Bool* val)
{
    OPENDAQ_PARAM_NOT_NULL(val);

    return getValueInternal<Bool>(*val);
}

// IList

ErrCode EvalValueImpl::getItemAt(SizeT index, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    auto err = checkParseAndResolve(false);
    OPENDAQ_RETURN_IF_FAILED(err);

    ListPtr<IBaseObject> list = node->getResult();
    auto res = list.getItemAt(index);

    *obj = res.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::getCount(SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);

    auto err = checkParseAndResolve(false);
    OPENDAQ_RETURN_IF_FAILED(err);

    ListPtr<IBaseObject> list = node->getResult();

    *size = list.getCount();

    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::setItemAt(SizeT /*index*/, IBaseObject* /*obj*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::pushBack(IBaseObject* /*obj*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::moveBack(IBaseObject* /*obj*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::moveFront(IBaseObject* /*obj*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::pushFront(IBaseObject* /*obj*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::popBack(IBaseObject** /*obj*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::popFront(IBaseObject** /*obj*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::insertAt(SizeT /*index*/, IBaseObject* /*obj*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::removeAt(SizeT /*index*/, IBaseObject** /*obj*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::deleteAt(SizeT /*index*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::clear()
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::createStartIterator(IIterator** iterator)
{
    ErrCode errCode = checkParseAndResolve(false);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    ListPtr<IBaseObject> list;

    try
    {
        list = node->getResult();
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }

    errCode = list->createStartIterator(iterator);

    return errCode;
}

ErrCode EvalValueImpl::createEndIterator(IIterator** iterator)
{
    ErrCode errCode = checkParseAndResolve(false);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    ListPtr<IBaseObject> list = node->getResult();
    errCode = list->createEndIterator(iterator);

    return errCode;
}

ErrCode EvalValueImpl::getFloatValue(Float* value)
{
    return toFloat(value);
}

ErrCode EvalValueImpl::getIntValue(Int* value)
{
    return toInt(value);
}

ErrCode EvalValueImpl::Property_GetValueType(CoreType* /*type*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetKeyType(CoreType* /*type*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetItemType(CoreType* /*type*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetName(IString** /*name*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetDescription(IString** /*description*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetUnit(IUnit** /*unit*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetMinValue(INumber** /*min*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetMaxValue(INumber** /*max*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetDefaultValue(IBaseObject** /*value*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetSuggestedValues(IList** /*values*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetVisible(Bool* /*visible*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetReadOnly(Bool* /*readOnly*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetSelectionValues(IBaseObject** /*values*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetReferencedProperty(IProperty** /*property*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetIsReferenced(Bool* /*isReferenced*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetValidator(IValidator** /*validator*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetCoercer(ICoercer** /*coercer*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetCallableInfo(ICallableInfo** /*callable*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetStructType(IStructType** /*structType*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetOnPropertyValueWrite(IEvent** /*event*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetOnPropertyValueRead(IEvent** /*event*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_GetValue(IBaseObject** /*value*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::Property_SetValue(IBaseObject* /*value*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ACCESSDENIED);
}

ErrCode EvalValueImpl::UnitObject_GetId(Int* id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    UnitPtr unit;
    ErrCode err = getValueInternal<UnitPtr>(unit);
    OPENDAQ_RETURN_IF_FAILED(err);

    return daqTry([&]
    {
        *id = unit.getId();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode EvalValueImpl::UnitObject_GetSymbol(IString** symbol)
{
    OPENDAQ_PARAM_NOT_NULL(symbol);

    UnitPtr unit;
    ErrCode err = getValueInternal<UnitPtr>(unit);
    OPENDAQ_RETURN_IF_FAILED(err);

    return daqTry([&]
    {
        *symbol = unit.getSymbol().addRefAndReturn();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode EvalValueImpl::UnitObject_GetName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    UnitPtr unit;
    ErrCode err = getValueInternal<UnitPtr>(unit);
    OPENDAQ_RETURN_IF_FAILED(err);

    return daqTry([&]
    {
        *name = unit.getName().addRefAndReturn();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode EvalValueImpl::UnitObject_GetQuantity(IString** quantity)
{
    OPENDAQ_PARAM_NOT_NULL(quantity);

    UnitPtr unit;
    ErrCode err = getValueInternal<UnitPtr>(unit);
    OPENDAQ_RETURN_IF_FAILED(err);

    return daqTry([&]
    {
        *quantity = unit.getQuantity().addRefAndReturn();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode EvalValueImpl::StructObject_getStructType(IStructType** /*type*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
}

ErrCode EvalValueImpl::StructObject_getFieldNames(IList** /*names*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
}

ErrCode EvalValueImpl::StructObject_getFieldValues(IList** /*values*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
}

ErrCode EvalValueImpl::StructObject_get(IString* /*name*/, IBaseObject** /*field*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
}

ErrCode EvalValueImpl::StructObject_hasField(IString* /*name*/, Bool* /*contains*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
}

ErrCode EvalValueImpl::StructObject_getAsDictionary(IDict** /*dictionary*/)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTIMPLEMENTED);
}

// static
ConstCharPtr EvalValueImpl::SerializeId()
{
    return "EvalValue";
}

ErrCode EvalValueImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);

    SizeT size;
    eval->getLength(&size);

    ConstCharPtr str;
    eval->getCharPtr(&str);

    serializer->key("eval");
    serializer->writeString(str, size);
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::Deserialize(ISerializedObject* serialized,
                                   IBaseObject* /*context*/,
                                   IFunction* /*factoryCallback*/,
                                   IBaseObject** obj)
{
    StringPtr eval;
    ErrCode errCode = serialized->readString(String("eval"), &eval);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    IEvalValue* evalValue;
    errCode = createEvalValue(&evalValue, eval);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    *obj = evalValue;

    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::cloneWithOwner(IPropertyObject* newOwner, IEvalValue** clonedValue)
{
    OPENDAQ_PARAM_NOT_NULL(clonedValue);
    OPENDAQ_PARAM_NOT_NULL(newOwner);

    // OPENDAQ_TODO: properly handle error when parse failed
    assert(node != nullptr);

    EvalValueImpl* newEvalValue;
    if (useFunctionResolver && func.assigned())
        newEvalValue = new (std::nothrow) EvalValueImpl(*this, newOwner, func);
    else
        newEvalValue = new (std::nothrow) EvalValueImpl(*this, newOwner);

    if (newEvalValue == nullptr)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOMEMORY);
    }
    
    newEvalValue->addRef();
    *clonedValue = newEvalValue;

    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::getParseErrorCode()
{
    if (OPENDAQ_FAILED(parseErrCode))
        return DAQ_MAKE_ERROR_INFO(parseErrCode, parseErrMessage);

    return OPENDAQ_SUCCESS;
}


ErrCode EvalValueImpl::getPropertyReferences(IList** propertyReferences)
{
    OPENDAQ_PARAM_NOT_NULL(propertyReferences);

    if (OPENDAQ_FAILED(parseErrCode))
        return DAQ_MAKE_ERROR_INFO(parseErrCode, parseErrMessage);

    if (this->propertyReferences)
    {
        auto list = List<IString>();
        for (const auto& el : *this->propertyReferences)
            list.pushBack(el);
        *propertyReferences = list.detach();
    }
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
