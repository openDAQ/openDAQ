#include <testutils/daq_memcheck_listener.h>
#include <coretypes/baseobject.h>
#include <coretypes/impl.h>
#include <coretypes/objectptr.h>

using namespace daq;

// unity-safe namespace: keeps this file's test types file-local
namespace test_internaladdref
{

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


using InternalAddRefTest = testing::Test;

TEST_F(InternalAddRefTest, Create)
{
    ObjectPtr<ITest> intf;
    checkErrorInfo(createObject<ITest, TestImpl>(&intf));
}
}
// namespace test_internaladdref
