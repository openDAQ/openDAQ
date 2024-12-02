#include <coreobjects/callable_info_impl.h>
#include <coreobjects/callable_info_factory.h>
#include <coretypes/impl.h>
#include <utility>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr callableInfoStructType = CallableInfoStructType();
}

CallableInfoImpl::CallableInfoImpl(ListPtr<IArgumentInfo> arguments, CoreType returnType, Bool constFlag)
    : GenericStructImpl<daq::ICallableInfo, daq::IStruct>(
          detail::callableInfoStructType,
          Dict<IString, IBaseObject>(
              {{"Arguments", arguments}, {"ReturnType", static_cast<Int>(returnType)}, {"Const", constFlag}}))
{
    this->returnType = this->fields.get("ReturnType");
    this->arguments = this->fields.get("Arguments");
    this->constFlag = this->fields.get("Const");
}

ErrCode CallableInfoImpl::getReturnType(CoreType* type)
{
    if (type == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    *type = this->returnType;
    return OPENDAQ_SUCCESS;
}

ErrCode CallableInfoImpl::getArguments(IList** argumentInfo)
{
    if (argumentInfo == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    *argumentInfo = this->arguments.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC CallableInfoImpl::isConst(Bool* constFlag)
{
    OPENDAQ_PARAM_NOT_NULL(constFlag);

    *constFlag = this->constFlag;
    return OPENDAQ_SUCCESS;
}

ErrCode CallableInfoImpl::equals(IBaseObject* other, Bool* equal) const
{
    return daqTry([this, &other, &equal]() {
        if (equal == nullptr)
            return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

        *equal = false;
        if (!other)
            return OPENDAQ_SUCCESS;

        CallableInfoPtr callableInfo = BaseObjectPtr::Borrow(other).asPtrOrNull<ICallableInfo>();
        if (callableInfo == nullptr)
            return OPENDAQ_SUCCESS;

        if (returnType != callableInfo.getReturnType())
            return OPENDAQ_SUCCESS;

        if (arguments != callableInfo.getArguments())
            return OPENDAQ_SUCCESS;

        if (constFlag != callableInfo.isConst())
            return OPENDAQ_SUCCESS;

        *equal = true;
        return OPENDAQ_SUCCESS;
    });
}

ErrCode CallableInfoImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    const auto serializerPtr = SerializerPtr::Borrow(serializer);

    return daqTry(
        [this, &serializerPtr] {
            serializerPtr.startTaggedObject(borrowPtr<SerializablePtr>());
            {
                if (arguments.assigned() && !arguments.empty())
                {
                    serializerPtr.key("arguments");
                    arguments.serialize(serializerPtr);
                }

                serializerPtr.key("returnType");
                serializerPtr.writeInt(static_cast<Int>(returnType));

                serializerPtr.key("const");
                serializerPtr.writeBool(static_cast<Bool>(constFlag));
            }

            serializerPtr.endObject();
        });
}

ErrCode CallableInfoImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr CallableInfoImpl::SerializeId()
{
    return "CallableInfo";
}

ErrCode CallableInfoImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    const auto serializedObj = SerializedObjectPtr::Borrow(serialized);
    const auto contextPtr = BaseObjectPtr::Borrow(context);
    const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

    return daqTry(
        [&serializedObj, &contextPtr, factoryCallbackPtr, &obj]
        {
            ListPtr<IArgumentInfo> arguments;
            if(serializedObj.hasKey("arguments"))
                arguments = serializedObj.readObject("arguments", contextPtr, factoryCallbackPtr);

            const auto returnType = static_cast<CoreType>(serializedObj.readInt("returnType"));

            bool isConst = false;
            if (serializedObj.hasKey("const"))
                isConst = static_cast<Bool>(serializedObj.readBool("const"));

            *obj = createWithImplementation<ICallableInfo, CallableInfoImpl>(arguments, returnType, isConst).detach();
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY,
    CallableInfo,
    IList*,
    argumentInfo,
    CoreType,
    returnType,
    Bool,
    constFlag
    );

END_NAMESPACE_OPENDAQ
