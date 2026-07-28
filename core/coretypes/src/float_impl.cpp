#include <coretypes/float_impl.h>
#include <coretypes/impl.h>
#include <coretypes/object_pool.h>
#include <coretypes/pooled_object.h>

BEGIN_NAMESPACE_OPENDAQ

using namespace object_pool;

class PooledFloatImpl : public OrdinalPooledObject<Float, PooledFloatImpl, FloatImpl>
{
public:
    using OrdinalPooledObject<Float, PooledFloatImpl, FloatImpl>::OrdinalPooledObject;
};

class StaticFloats
{
public:
    IFloat* zeroValue;
    IFloat* oneValue;

#ifdef OPENDAQ_ENABLE_OBJECT_POOLS
    ObjectPool<PooledFloatImpl> floatPool{100};
#endif

    StaticFloats()
    {
        daq::createObject<IFloat, FloatImpl, const Float>(&zeroValue, 0.0);
        daq::createObject<IFloat, FloatImpl, const Float>(&oneValue, 1.0);
    }

    ~StaticFloats()
    {
        oneValue->releaseRef();
        zeroValue->releaseRef();
    }

} staticFloats;

ErrCode createFloatFromCache(IFloat** objTmp, const Float value)
{
    assert(objTmp != nullptr);

    if (value == 0.0)
    {
        staticFloats.zeroValue->addRef();
        *objTmp = staticFloats.zeroValue;
        return OPENDAQ_SUCCESS;
    }

    if (value == 1.0)
    {
        staticFloats.oneValue->addRef();
        *objTmp = staticFloats.oneValue;
        return OPENDAQ_SUCCESS;
    }

    return OPENDAQ_ERR_NOTFOUND;
}

ErrCode createFloatInternal(IFloat** objTmp, const Float value)
{
    OPENDAQ_PARAM_NOT_NULL(objTmp);

    auto errCode = createFloatFromCache(objTmp, value);
    if (errCode == OPENDAQ_ERR_NOTFOUND)
        errCode = daq::createObject<IFloat, FloatImpl, const Float>(objTmp, value);

    return errCode;
}

extern "C" daq::ErrCode LIBRARY_FACTORY createFloat(IFloat** objTmp, const Float value)
{
    return createFloatInternal(objTmp, value);
}

extern "C" daq::ErrCode LIBRARY_FACTORY createFloatObject(IFloat** objTmp, const Float value)
{
    return createFloatInternal(objTmp, value);
}

#ifdef OPENDAQ_ENABLE_OBJECT_POOLS

extern "C" daq::ErrCode LIBRARY_FACTORY createFloatFromPool(IFloat** objTmp, const Float value)
{
    OPENDAQ_PARAM_NOT_NULL(objTmp);

    auto errCode = createFloatFromCache(objTmp, value);
    if (errCode == OPENDAQ_ERR_NOTFOUND)
    {
        *objTmp = staticFloats.floatPool.get(value);
        (*objTmp)->addRef();
        errCode = OPENDAQ_SUCCESS;
    }
    return errCode;
}

#else

extern "C" daq::ErrCode LIBRARY_FACTORY createFloatFromPool(IFloat** objTmp, const Float value)
{
    return createFloat(objTmp, value);
}

#endif

END_NAMESPACE_OPENDAQ
