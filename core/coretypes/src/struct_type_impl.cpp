#include <coretypes/struct_type_impl.h>
#include <coretypes/baseobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static std::unordered_set<CoreType> structAcceptedCoreTypes = {
        ctBool, ctInt, ctFloat, ctString, ctList, ctDict, ctRatio, ctComplexNumber, ctStruct, ctUndefined};
}

StructTypeImpl::StructTypeImpl(StringPtr name, ListPtr<IString> names, ListPtr<IBaseObject> defaultValues, ListPtr<IType> types)
    : GenericTypeImpl(std::move(name), ctStruct)
    , names(std::move(names))
    , defaultValues(std::move(defaultValues))
    , types(std::move(types))
{
    this->names.freeze();
    this->defaultValues.freeze();
    this->types.freeze();

    if (this->names.getCount() != this->defaultValues.getCount() || this->names.getCount() != this->types.getCount())
        throw InvalidParameterException{"StructType parameters are of different sizes."};

    for (const TypePtr& type: this->types)
    {
        if (!detail::structAcceptedCoreTypes.count(type.getCoreType()))
            throw InvalidParameterException{"Struct fields cannot be ctObject, ctUndefined, ctFunc, ctProc, ctBinaryData"};
    }
}


StructTypeImpl::StructTypeImpl(StringPtr name, ListPtr<IString> names, ListPtr<IType> types)
    : GenericTypeImpl(std::move(name), ctStruct)
    , names(std::move(names))
    , types(std::move(types))
{
    if (this->names.assigned())
        this->names.freeze();
    if (this->types.assigned())
        this->types.freeze();

    if (this->names.getCount() != this->types.getCount())
        throw InvalidParameterException{"StructType parameters are of different sizes."};

    for (const TypePtr& type: this->types)
    {
        if (!detail::structAcceptedCoreTypes.count(type.getCoreType()))
            throw InvalidParameterException{"Struct fields cannot be ctObject, ctFunc, ctProc, ctBinaryData"};
    }
}

ErrCode StructTypeImpl::getFieldNames(IList** names)
{
    if (names == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *names = this->names.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode StructTypeImpl::getFieldDefaultValues(IList** defaultValues)
{
    if (defaultValues == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *defaultValues = this->defaultValues.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode StructTypeImpl::getFieldTypes(IList** types)
{
    if (types == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *types = this->types.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode StructTypeImpl::equals(IBaseObject* other, Bool* equal) const
{
    if (equal == nullptr)
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

    *equal = false;
    if (other == nullptr)
        return OPENDAQ_SUCCESS;

    const StructTypePtr typeOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IStructType>();
    if (typeOther == nullptr)
        return OPENDAQ_SUCCESS;

    *equal = typeOther.getFieldTypes() == this->types && typeOther.getFieldDefaultValues() == this->defaultValues &&
             typeOther.getFieldNames() == this->names;

    if (!*equal)
        return OPENDAQ_SUCCESS;
    
    return GenericTypeImpl<IStructType>::equals(other, equal);
}

ErrCode StructTypeImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    
    serializer->key("typeName");
    serializer->writeString(this->typeName.getCharPtr(), this->typeName.getLength());

    serializer->key("names");
    ISerializable* serializable;
    ErrCode errCode = this->names->borrowInterface(ISerializable::Id, reinterpret_cast<void**>(&serializable));

    if (errCode == OPENDAQ_ERR_NOINTERFACE)
        return OPENDAQ_ERR_NOT_SERIALIZABLE;

    if (OPENDAQ_FAILED(errCode))
        return errCode;

    errCode = serializable->serialize(serializer);

    if (OPENDAQ_FAILED(errCode))
        return errCode;

    if (defaultValues.assigned() && !defaultValues.empty())
    {
        serializer->key("defaultValues");
        errCode = this->defaultValues->borrowInterface(ISerializable::Id, reinterpret_cast<void**>(&serializable));

        if (errCode == OPENDAQ_ERR_NOINTERFACE)
            return OPENDAQ_ERR_NOT_SERIALIZABLE;

        if (OPENDAQ_FAILED(errCode))
            return errCode;

        errCode = serializable->serialize(serializer);

        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }

    serializer->key("types");
    errCode = this->types->borrowInterface(ISerializable::Id, reinterpret_cast<void**>(&serializable));

    if (errCode == OPENDAQ_ERR_NOINTERFACE)
        return OPENDAQ_ERR_NOT_SERIALIZABLE;

    if (OPENDAQ_FAILED(errCode))
        return errCode;

    errCode = serializable->serialize(serializer);

    if (OPENDAQ_FAILED(errCode))
        return errCode;
    
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode StructTypeImpl::getSerializeId(ConstCharPtr* id) const
{
    if (!id)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr StructTypeImpl::SerializeId()
{
    return "StructType";
}

ErrCode StructTypeImpl::Deserialize(ISerializedObject* ser, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    StringPtr typeName;
    ErrCode errCode = ser->readString("typeName"_daq, &typeName);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    BaseObjectPtr types;
    errCode = ser->readObject("types"_daq, context, factoryCallback, &types);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    BaseObjectPtr defaultValues;
    errCode = ser->readObject("defaultValues"_daq, context, factoryCallback, &defaultValues);
    if (OPENDAQ_FAILED(errCode) && errCode != OPENDAQ_ERR_NOTFOUND)
        return errCode;

    BaseObjectPtr names;
    errCode = ser->readObject("names"_daq, context, factoryCallback, &names);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    try
    {
        StructTypePtr structType;
        if (defaultValues.assigned())
            createStructType(&structType, typeName, names.asPtr<IList>(), defaultValues.asPtr<IList>(), types.asPtr<IList>());
        else
            createStructTypeNoDefaults(&structType, typeName, names.asPtr<IList>(), types.asPtr<IList>());
        *obj = structType.detach();
    }
    catch (const DaqException& e)
    {
        return e.getErrCode();
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, StructType, IString*, name, IList*, fieldNames, IList*, defaultValues, IList*, fieldTypes)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, StructType, IStructType, createStructTypeNoDefaults, IString*, name, IList*, fieldNames, IList*, fieldTypes)

END_NAMESPACE_OPENDAQ
