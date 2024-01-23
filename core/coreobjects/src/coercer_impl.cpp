#include <coreobjects/coercer_impl.h>
#include <coreobjects/eval_value_factory.h>

BEGIN_NAMESPACE_OPENDAQ

CoercerImpl::CoercerImpl(const StringPtr& evalStr)
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

ErrCode CoercerImpl::coerce(IBaseObject* propObj, IBaseObject* value, IBaseObject** result)
{
    try
    {
        this->value = value;

        if (propObj != nullptr)
        {
            auto cloned = evalValue.cloneWithOwner(propObj);
            *result = cloned.getResult().detach();
        }
        else
        {
            *result = evalValue.getResult().detach();
        }

        this->value = nullptr;
    }
    catch (...)
    {
        return OPENDAQ_ERR_COERCE_FAILED;
    }

    return OPENDAQ_SUCCESS;
}

ErrCode CoercerImpl::getEval(IString** eval)
{
    if (eval == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *eval = this->eval.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode CoercerImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    {
        serializer->key("EvalStr");
        serializer->writeString(evalValue.getEval().getCharPtr(), evalValue.getEval().getLength());
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode CoercerImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr CoercerImpl::SerializeId()
{
    return "Coercer";
}

ErrCode CoercerImpl::Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj)
{
    const SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);
    const StringPtr str = serializedObj.readString("EvalStr");

    CoercerPtr coercerPtr;
    ErrCode errCode = createCoercer(&coercerPtr, str);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    *obj = coercerPtr.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, Coercer, IString*, eval)

END_NAMESPACE_OPENDAQ
