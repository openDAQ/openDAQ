#include <coreobjects/validator_impl.h>
#include <coreobjects/eval_value_factory.h>

BEGIN_NAMESPACE_OPENDAQ


ValidatorImpl::ValidatorImpl(const StringPtr& evalStr)
{
    eval = evalStr;
    evalValue = EvalValueFunc(evalStr,
                  [this](const std::string& str) -> BaseObjectPtr
                  {
                      if (str == "value" || str == "val" || str == "Value" || str == "Val")
                      {
                          return this->value;
                      }
                      return nullptr;
                  });
}

ErrCode ValidatorImpl::validate(IBaseObject* propObj, IBaseObject* value)
{
    try
    {
        Bool validated = false;
        this->value = value;

        if (propObj != nullptr)
        {
            auto cloned = evalValue.cloneWithOwner(propObj);
            validated = cloned.getResult();
        }
        else
        {
            validated = evalValue.getResult();
        }

        this->value = nullptr;
        if (validated)
            return OPENDAQ_SUCCESS;
        
    }
    catch (...)
    {
    }

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_VALIDATE_FAILED);
}

ErrCode ValidatorImpl::validateNoLock(IBaseObject* propObj, IBaseObject* value)
{
        try
    {
        Bool validated = false;
        this->value = value;

        if (propObj != nullptr)
        {
            auto cloned = evalValue.cloneWithOwner(propObj);
            validated = cloned.getResultNoLock();
        }
        else
        {
            validated = evalValue.getResultNoLock();
        }

        this->value = nullptr;
        if (validated)
            return OPENDAQ_SUCCESS;
        
    }
    catch (...)
    {
    }

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_VALIDATE_FAILED);
}

ErrCode ValidatorImpl::getEval(IString** eval)
{
    OPENDAQ_PARAM_NOT_NULL(eval);

    *eval = this->eval.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ValidatorImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    {
        serializer->key("EvalStr");
        serializer->writeString(evalValue.getEval().getCharPtr(), evalValue.getEval().getLength());
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode ValidatorImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr ValidatorImpl::SerializeId()
{
    return "Validator";
}

ErrCode ValidatorImpl::Deserialize(ISerializedObject* serialized,
                                   IBaseObject* /*context*/,
                                   IFunction* /*factoryCallback*/,
                                   IBaseObject** obj)
{
    const SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);
    const StringPtr str = serializedObj.readString("EvalStr");
    
    ValidatorPtr validatorPtr;
    ErrCode errCode = createValidator(&validatorPtr, str);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    *obj = validatorPtr.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, Validator, IString*, eval)

END_NAMESPACE_OPENDAQ
