#include <coretypes/integer_impl.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

class StaticInts
{
public:
    IInteger* zeroValue;
    IInteger* oneValue;
    IInteger* twoValue;
    IInteger* threeValue;
    IInteger* fourValue;
    IInteger* fiveValue;
    IInteger* sixValue;
    IInteger* sevenValue;
    IInteger* minusOneValue;

    StaticInts()
    {
        daq::createObject<IInteger, IntegerImpl, const Int>(&zeroValue, 0);
        daq::createObject<IInteger, IntegerImpl, const Int>(&oneValue, 1);
        daq::createObject<IInteger, IntegerImpl, const Int>(&twoValue, 2);
        daq::createObject<IInteger, IntegerImpl, const Int>(&threeValue, 3);
        daq::createObject<IInteger, IntegerImpl, const Int>(&fourValue, 4);
        daq::createObject<IInteger, IntegerImpl, const Int>(&fiveValue, 5);
        daq::createObject<IInteger, IntegerImpl, const Int>(&sixValue, 6);
        daq::createObject<IInteger, IntegerImpl, const Int>(&sevenValue, 7);
        daq::createObject<IInteger, IntegerImpl, const Int>(&minusOneValue, -1);
    }

    ~StaticInts()
    {
        zeroValue->releaseRef();
        oneValue->releaseRef();
        twoValue->releaseRef();
        threeValue->releaseRef();
        fourValue->releaseRef();
        fiveValue->releaseRef();
        sixValue->releaseRef();
        minusOneValue->releaseRef();
        sevenValue->releaseRef();
    }

} staticInts;

extern "C" daq::ErrCode LIBRARY_FACTORY createInteger(IInteger** objTmp, const Int value)
{
    if (!objTmp)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    switch (value)
    {
        case -1:
            staticInts.minusOneValue->addRef();
            *objTmp = staticInts.minusOneValue;
            return OPENDAQ_SUCCESS;
        case 0:
            staticInts.zeroValue->addRef();
            *objTmp = staticInts.zeroValue;
            return OPENDAQ_SUCCESS;
        case 1:
            staticInts.oneValue->addRef();
            *objTmp = staticInts.oneValue;
            return OPENDAQ_SUCCESS;
        case 2:
            staticInts.twoValue->addRef();
            *objTmp = staticInts.twoValue;
            return OPENDAQ_SUCCESS;
        case 3:
            staticInts.threeValue->addRef();
            *objTmp = staticInts.threeValue;
            return OPENDAQ_SUCCESS;
        case 4:
            staticInts.fourValue->addRef();
            *objTmp = staticInts.fourValue;
            return OPENDAQ_SUCCESS;
        case 5:
            staticInts.fiveValue->addRef();
            *objTmp = staticInts.fiveValue;
            return OPENDAQ_SUCCESS;
        case 6:
            staticInts.sixValue->addRef();
            *objTmp = staticInts.sixValue;
            return OPENDAQ_SUCCESS;
        case 7:
            staticInts.sevenValue->addRef();
            *objTmp = staticInts.sevenValue;
            return OPENDAQ_SUCCESS;
        default:
            return daq::createObject<IInteger, IntegerImpl, const Int>(objTmp, value);
    }
}

END_NAMESPACE_OPENDAQ
