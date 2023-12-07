#include <coretypes/struct_builder_impl.h>
#include <coretypes/validation.h>
#include <coretypes/struct_factory.h>

BEGIN_NAMESPACE_OPENDAQ
    StructBuilderImpl::StructBuilderImpl(const StringPtr& name, const TypeManagerPtr& typeManager)
    : fields(Dict<IString, IBaseObject>())
{
    structType = typeManager.getType(name);

    const auto defaultValues = structType.getFieldDefaultValues();
    const auto names = structType.getFieldNames();
    
    for (size_t i = 0 ; i < names.getCount(); ++i)
    {
        if (defaultValues.assigned())
            fields.set(names[i], defaultValues[i]);
        else
            fields.set(names[i], nullptr);
    }
}

StructBuilderImpl::StructBuilderImpl(const StructPtr& struct_)
    : fields(Dict<IString, IBaseObject>())
{
    structType = struct_.getStructType();
    for(const auto& field : struct_.getAsDictionary())
        fields.set(field.first, field.second);
}

ErrCode StructBuilderImpl::build(IStruct** struct_)
{
    OPENDAQ_PARAM_NOT_NULL(struct_);

    const auto builderPtr = this->borrowPtr<StructBuilderPtr>();

    return daqTry([&]()
    {
        *struct_ = StructFromBuilder(builderPtr).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode StructBuilderImpl::setFieldValues(IList* values)
{
    OPENDAQ_PARAM_NOT_NULL(values);

    const auto& valuesPtr = ListPtr<IString>::Borrow(values);
    if (valuesPtr.getCount() != fields.getCount())
        return OPENDAQ_ERR_INVALIDPARAMETER;

    const auto types = structType.getFieldTypes();
    for (size_t i = 0; i < valuesPtr.getCount(); ++i)
        if (valuesPtr.assigned() && SimpleType(valuesPtr[i].getCoreType()) != types[i])
            return OPENDAQ_ERR_INVALIDPARAMETER;

    DictPtr<IString, IBaseObject> newDict = Dict<IString, IBaseObject>();
    const auto keyList = fields.getKeyList();
    for (size_t i = 0; i < valuesPtr.getCount(); ++i)
        newDict.set(keyList[i], valuesPtr[i]);

    fields = newDict.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode StructBuilderImpl::set(IString* name, IBaseObject* field)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    if (!fields.hasKey(name))
        return OPENDAQ_ERR_NOTFOUND;

    const auto namePtr = StringPtr::Borrow(name);
    auto fieldPtr = BaseObjectPtr::Borrow(field);
    const auto names = structType.getFieldNames();

    if (!fieldPtr.assigned())
    {
        const auto defaultValues = structType.getFieldDefaultValues();
        if (defaultValues.assigned())
        {
            for (size_t i = 0; i < names.getCount(); ++i)
            {
                if (names[i] == namePtr)
                {
                    fieldPtr = defaultValues[i];
                    break;
                }
            }
        }
    }
    else
    {
        for (size_t i = 0; i < names.getCount(); ++i)
            if (names[i] == namePtr && SimpleType(fieldPtr.getCoreType())!= structType.getFieldTypes()[i])
                return OPENDAQ_ERR_INVALIDPARAMETER;
    }

    fields.set(name, field);
    return OPENDAQ_SUCCESS;
}

ErrCode StructBuilderImpl::getStructType(IStructType** type)
{
    if (!type)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *type = this->structType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode StructBuilderImpl::getFieldNames(IList** names)
{
    if (!names)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *names = this->fields.getKeyList().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode StructBuilderImpl::getFieldValues(IList** values)
{
    if (!values)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *values = this->fields.getValueList().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode StructBuilderImpl::get(IString* name, IBaseObject** field)
{
    if (!name)
    {
        *field = nullptr;
        return OPENDAQ_SUCCESS;
    }

    if (!field)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    const auto nameObj = StringPtr::Borrow(name);
    if (this->fields.hasKey(name))
        *field = this->fields.get(name).addRefAndReturn();
    else
        *field = nullptr;

    return OPENDAQ_SUCCESS;
}

ErrCode StructBuilderImpl::getAsDictionary(IDict** dictionary)
{
    if (!dictionary)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *dictionary = this->fields.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode StructBuilderImpl::hasField(IString* name, Bool* contains)
{
    if (!contains)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *contains = false;
    if (!name)
        return OPENDAQ_SUCCESS;

    const auto nameObj = StringPtr::Borrow(name);
    if (this->fields.hasKey(name))
        *contains = true;

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, StructBuilder, IString*, name, ITypeManager*, typeManager)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY, StructBuilder, IStructBuilder, createStructBuilderFromStruct, IStruct*, struct_)


END_NAMESPACE_OPENDAQ
