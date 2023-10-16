#include <testutils/bb_memcheck_listener.h>
#include <coretypes/baseobject.h>
#include <coretypes/impl.h>
#include <coretypes/objectptr.h>

DECLARE_OPENDAQ_INTERFACE(ITest, daq::IBaseObject)
{
};

class TestImpl : public daq::ImplementationOf<ITest>
{
public:
    TestImpl()
    {
        internalAddRef();
    }
};

using namespace daq;

using InternalAddRefTest = testing::Test;

TEST_F(InternalAddRefTest, Create)
{
    ObjectPtr<ITest> intf;
    checkErrorInfo(createObject<ITest, TestImpl>(&intf));
}
