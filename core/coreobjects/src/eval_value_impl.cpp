#include <coreobjects/eval_value_impl.h>
#include <coreobjects/eval_value_parser.h>
#include <functional>
#include <coreobjects/eval_value_ptr.h>

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
    node = ev.node->clone([this](const std::string& str, RefType refType, int argIndex, std::string& postRef)
    {
        return getReference(str, refType, argIndex, postRef);
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
    node = ev.node->clone([this](const std::string& str, RefType refType, int argIndex, std::string& postRef)
    {
        return getReference(str, refType, argIndex, postRef);
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
        [this](const std::string& str, RefType refType, int argIndex, std::string& postRef)
        {
            return getReference(str, refType, argIndex, postRef);
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

    parseErrCode = parsed ? OPENDAQ_SUCCESS :  OPENDAQ_ERR_PARSEFAILED;
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

BaseObjectPtr EvalValueImpl::getReferenceFromPrefix(const PropertyObjectPtr& propObject, const std::string& str, RefType refType) const
{
    BaseObjectPtr value;

    if (refType == RefType::Property)
    {
        value = propObject.getProperty(str);
    }
    else if (refType == RefType::Value)
    {
        value = propObject.getPropertyValue(str);
        checkForEvalValue(value);
    }
    else if (refType == RefType::SelectedValue)
    {
        value = propObject.getPropertySelectionValue(str);
        checkForEvalValue(value);
    }

    return value;
}

BaseObjectPtr EvalValueImpl::getReference(const std::string& str, RefType refType, int argIndex, std::string& postRef) const
{
    if (argIndex > -1)
    {
        if (!arguments.assigned() || argIndex > int(arguments.getCount()))
        {
            return nullptr;
        }

        return getReferenceFromPrefix(arguments[argIndex], str, refType);
    }

    if (refType == RefType::Func)
        return func.call(String(str));

    if (!owner.assigned())
        return nullptr;

    auto pos = str.find(':');

    PropertyObjectPtr ownerRef = owner.getRef();
    if (pos == std::string::npos)
        return getReferenceFromPrefix(ownerRef, str, refType);

    std::string prefix = str.substr(0, pos);
    postRef = str.substr(pos + 1, std::wstring::npos);
#if defined(_WIN32)
    if (_stricmp("value", postRef.c_str()) == 0)
#else
    if (strcasecmp("value", postRef.c_str()) == 0)
#endif
        return getReferenceFromPrefix(ownerRef, prefix, RefType::Value);
#if defined(_WIN32)
    if (_stricmp("selectedvalue", postRef.c_str()) == 0)
#else
    if (strcasecmp("selectedvalue", postRef.c_str()) == 0)
#endif
        return getReferenceFromPrefix(ownerRef, prefix, RefType::SelectedValue);
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

int EvalValueImpl::resolveReferences()
{
    assert(node);

    int r = node->visit([](BaseNode* input)
    {
        return input->resolveReference();
    });

    resolveStatus = r == 0 ? ResolveStatus::Resolved
                           : ResolveStatus::Failed;
    return r;
}

ErrCode EvalValueImpl::checkParseAndResolve()
{
    if (OPENDAQ_FAILED(parseErrCode))
    {
        return parseErrCode;
    }

    int r = resolveReferences();
    if (r != 0)
        return OPENDAQ_ERR_RESOLVEFAILED;

    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::getCoreType(CoreType* coreType)
{
    if (coreType == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    ErrCode err = checkParseAndResolve();
    if (OPENDAQ_FAILED(err))
        return err;

    try
    {
        *coreType = calc().getCoreType();
        return OPENDAQ_SUCCESS;
    }
    catch (...)
    {
        return OPENDAQ_ERR_CALCFAILED;
    };
}

ErrCode EvalValueImpl::getEval(IString** evalString)
{
    if (evalString == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

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
    if (obj == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    ErrCode err = checkParseAndResolve();
    if (OPENDAQ_FAILED(err))
        return err;

    try
    {
        *obj = calc().addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }
    catch (...)
    {
        return OPENDAQ_ERR_CALCFAILED;
    };
}

template <typename T>
ErrCode EvalValueImpl::equalsValueInternal(const T value, Bool* equals)
{
    if (equals == nullptr)
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals output-parameter must not be null.");

    T thisValue;
    ErrCode errCode = getValueInternal<T>(thisValue);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    *equals = value == thisValue;
    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::Float_GetValue(Float* value)
{
    if (value == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return getValueInternal<Float>(*value);
}

ErrCode EvalValueImpl::Float_EqualsValue(const Float value, Bool* equals)
{
    return equalsValueInternal<Float>(value, equals);
}

ErrCode EvalValueImpl::Integer_GetValue(Int* value)
{
    if (value == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return getValueInternal<Int>(*value);
}

ErrCode EvalValueImpl::Integer_EqualsValue(const Int value, Bool* equals)
{
    return equalsValueInternal<Int>(value, equals);
}

ErrCode EvalValueImpl::Boolean_GetValue(Bool* value)
{
    if (value == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return getValueInternal<Bool>(*value);
}

ErrCode EvalValueImpl::Boolean_EqualsValue(const Bool value, Bool* equals)
{
    return equalsValueInternal<Bool>(value, equals);
}

ErrCode EvalValueImpl::StringObject_GetCharPtr(ConstCharPtr* value)
{
    if (value == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    ErrCode errCode = getValueInternal<std::string>(strResult);
    *value = strResult.c_str();
    return errCode;
}

ErrCode EvalValueImpl::StringObject_GetLength(SizeT* size)
{
    if (size == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    ErrCode errCode = getValueInternal<std::string>(strResult);
    *size = strResult.length();
    return errCode;
}

template <typename T>
ErrCode EvalValueImpl::getValueInternal(T& value)
{
    auto err = checkParseAndResolve();
    if (OPENDAQ_FAILED(err))
        return err;

    try
    {
        value = static_cast<T>(calc());
        return OPENDAQ_SUCCESS;
    }
    catch (...)
    {
        return OPENDAQ_ERR_CALCFAILED;
    };
}

ErrCode EvalValueImpl::toString(CharPtr* str)
{
    if (str == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    ErrCode errCode = getValueInternal<std::string>(strResult);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    const auto len = std::strlen(strResult.c_str()) + 1;
    *str = static_cast<CharPtr>(daqAllocateMemory(len));
    if (*str == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

#if defined(__STDC_SECURE_LIB__) || defined(__STDC_LIB_EXT1__)
    strcpy_s(*str, len, strResult.c_str());
#else
    strncpy(*str, strResult.c_str(), len);
#endif
    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::toFloat(Float* val)
{
    if (val == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return getValueInternal<Float>(*val);
}

ErrCode EvalValueImpl::toInt(Int* val)
{
    if (val == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return getValueInternal<Int>(*val);
}

ErrCode EvalValueImpl::toBool(Bool* val)
{
    if (val == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return getValueInternal<Bool>(*val);
}

// IList

ErrCode EvalValueImpl::getItemAt(SizeT index, IBaseObject** obj)
{
    if (obj == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    auto err = checkParseAndResolve();
    if (OPENDAQ_FAILED(err))
        return err;

    ListPtr<IBaseObject> list = node->getResult();
    auto res = list.getItemAt(index);

    *obj = res.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::getCount(SizeT* size)
{
    if (size == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    auto err = checkParseAndResolve();
    if (OPENDAQ_FAILED(err))
        return err;

    ListPtr<IBaseObject> list = node->getResult();

    *size = list.getCount();

    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::setItemAt(SizeT /*index*/, IBaseObject* /*obj*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::pushBack(IBaseObject* /*obj*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::moveBack(IBaseObject* /*obj*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::moveFront(IBaseObject* /*obj*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::pushFront(IBaseObject* /*obj*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::popBack(IBaseObject** /*obj*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::popFront(IBaseObject** /*obj*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::insertAt(SizeT /*index*/, IBaseObject* /*obj*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::removeAt(SizeT /*index*/, IBaseObject** /*obj*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::deleteAt(SizeT /*index*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::clear()
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::createStartIterator(IIterator** iterator)
{
    ErrCode errCode = checkParseAndResolve();
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    ListPtr<IBaseObject> list;

    try
    {
        list = node->getResult();
    }
    catch (const DaqException& e)
    {
        return e.getErrCode();
    }

    errCode = list->createStartIterator(iterator);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::createEndIterator(IIterator** iterator)
{
    ErrCode errCode = checkParseAndResolve();
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    ListPtr<IBaseObject> list = node->getResult();
    errCode = list->createEndIterator(iterator);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    return OPENDAQ_SUCCESS;
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
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetKeyType(CoreType* /*type*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetItemType(CoreType* /*type*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetName(IString** /*name*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetDescription(IString** /*description*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetUnit(IUnit** /*unit*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetMinValue(INumber** /*min*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetMaxValue(INumber** /*max*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetDefaultValue(IBaseObject** /*value*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetSuggestedValues(IList** /*values*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetVisible(Bool* /*visible*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetReadOnly(Bool* /*readOnly*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetSelectionValues(IBaseObject** /*values*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetReferencedProperty(IProperty** /*property*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetIsReferenced(Bool* /*isReferenced*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetValidator(IValidator** /*validator*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetCoercer(ICoercer** /*coercer*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetCallableInfo(ICallableInfo** /*callable*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetStructType(IStructType** /*structType*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetOnPropertyValueWrite(IEvent** /*event*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::Property_GetOnPropertyValueRead(IEvent** /*event*/)
{
    return OPENDAQ_ERR_ACCESSDENIED;
}

ErrCode EvalValueImpl::UnitObject_GetId(Int* id)
{
    if (id == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    UnitPtr unit;
    ErrCode err = getValueInternal<UnitPtr>(unit);
    if (OPENDAQ_FAILED(err))
        return err;

    return daqTry(
        [&]()
        {
            *id = unit.getId();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode EvalValueImpl::UnitObject_GetSymbol(IString** symbol)
{
    if (symbol == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    UnitPtr unit;
    ErrCode err = getValueInternal<UnitPtr>(unit);
    if (OPENDAQ_FAILED(err))
        return err;

    return daqTry(
        [&]()
        {
            *symbol = unit.getSymbol().addRefAndReturn();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode EvalValueImpl::UnitObject_GetName(IString** name)
{
    if (name == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    UnitPtr unit;
    ErrCode err = getValueInternal<UnitPtr>(unit);
    if (OPENDAQ_FAILED(err))
        return err;

    return daqTry(
        [&]()
        {
            *name = unit.getName().addRefAndReturn();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode EvalValueImpl::UnitObject_GetQuantity(IString** quantity)
{
    if (quantity == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    UnitPtr unit;
    ErrCode err = getValueInternal<UnitPtr>(unit);
    if (OPENDAQ_FAILED(err))
        return err;

    return daqTry(
        [&]()
        {
            *quantity = unit.getQuantity().addRefAndReturn();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode EvalValueImpl::StructObject_getStructType(IStructType** /*type*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode EvalValueImpl::StructObject_getFieldNames(IList** /*names*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode EvalValueImpl::StructObject_getFieldValues(IList** /*values*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode EvalValueImpl::StructObject_get(IString* /*name*/, IBaseObject** /*field*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode EvalValueImpl::StructObject_hasField(IString* /*name*/, Bool* /*contains*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode EvalValueImpl::StructObject_getAsDictionary(IDict** /*dictionary*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
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
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    IEvalValue* evalValue;
    errCode = createEvalValue(&evalValue, eval);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    *obj = evalValue;

    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::cloneWithOwner(IPropertyObject* newOwner, IEvalValue** clonedValue)
{
    if (newOwner == nullptr || clonedValue == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    // OPENDAQ_TODO: properly handle error when parse failed
    assert(node != nullptr);

    EvalValueImpl* newEvalValue;
    if (useFunctionResolver && func.assigned())
        newEvalValue = new (std::nothrow) EvalValueImpl(*this, newOwner, func);
    else
        newEvalValue = new (std::nothrow) EvalValueImpl(*this, newOwner);

    if (newEvalValue == nullptr)
    {
        return OPENDAQ_ERR_NOMEMORY;
    }
    
    newEvalValue->addRef();
    *clonedValue = newEvalValue;

    return OPENDAQ_SUCCESS;
}

ErrCode EvalValueImpl::getParseErrorCode()
{
    if (OPENDAQ_FAILED(parseErrCode))
        return makeErrorInfo(parseErrCode, parseErrMessage);

    return OPENDAQ_SUCCESS;
}


ErrCode EvalValueImpl::getPropertyReferences(IList** propertyReferences)
{
    if (propertyReferences == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (OPENDAQ_FAILED(parseErrCode))
        return makeErrorInfo(parseErrCode, parseErrMessage);

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
