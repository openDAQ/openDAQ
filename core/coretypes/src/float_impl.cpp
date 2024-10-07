#include <coretypes/float_impl.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

class StaticFloats
{
public:
    IFloat* zeroValue;

    StaticFloats()
    {
        daq::createObject<IFloat, FloatImpl, const Float>(&zeroValue, 0.0);
    }

    ~StaticFloats()
    {
        zeroValue->releaseRef();
    }

} staticFloats;

inline daq::ErrCode createFloatInternal(IFloat** objTmp, const Float value)
{
    if (!objTmp)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (value == 0.0)
    {
        staticFloats.zeroValue->addRef();
        *objTmp = staticFloats.zeroValue;
        return OPENDAQ_SUCCESS;
    }

    return daq::createObject<IFloat, FloatImpl, const Float>(objTmp, value);
}

extern "C" daq::ErrCode LIBRARY_FACTORY createFloat(IFloat** objTmp, const Float value)
{
    return createFloatInternal(objTmp, value);
}

extern "C" daq::ErrCode LIBRARY_FACTORY createFloatObject(IFloat** objTmp, const Float value)
{
    return createFloatInternal(objTmp, value);
}

END_NAMESPACE_OPENDAQ
