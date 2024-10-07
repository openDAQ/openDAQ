#include <coretypes/boolean_impl.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

class StaticBools
{
public:
    IBoolean* trueValue;
    IBoolean* falseValue;

    StaticBools()
    {
        daq::createObject<IBoolean, BooleanImpl, const Bool>(&trueValue, True);
        daq::createObject<IBoolean, BooleanImpl, const Bool>(&falseValue, False);
    }

    ~StaticBools()
    {
        trueValue->releaseRef();
        falseValue->releaseRef();
    }

} staticBools;

inline daq::ErrCode createBoolInternal(IBoolean**objTmp, const Bool value)
{
    if (!objTmp)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (value)
    {
        staticBools.trueValue->addRef();
        *objTmp = staticBools.trueValue;
    }
    else
    {
        staticBools.falseValue->addRef();
        *objTmp = staticBools.falseValue;
    }

    return OPENDAQ_SUCCESS;
}

extern "C" daq::ErrCode LIBRARY_FACTORY createBoolean(IBoolean** objTmp, const Bool value)
{
    return createBoolInternal(objTmp, value);
}

extern "C" daq::ErrCode LIBRARY_FACTORY createBoolObject(IBoolean** objTmp, const Bool value)
{
    return createBoolInternal(objTmp, value);
}

END_NAMESPACE_OPENDAQ
