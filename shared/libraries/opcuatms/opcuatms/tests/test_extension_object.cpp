#include "gtest/gtest.h"
#include "opcuatms/extension_object.h"
#include <opendaq/range_factory.h>
#include "opcuashared/opcuaobject.h"
#include "opcuatms/converters/variant_converter.h"

using ExtensionObjectTest = testing::Test;

using namespace daq;
using namespace daq::opcua;
using namespace daq::opcua::tms;
using namespace daq::opcua;

TEST_F(ExtensionObjectTest, IsDecoded)
{
    const RangePtr range = Range(0, 10);
    const auto variant = VariantConverter<IRange>::ToVariant(range);

    ExtensionObject eo(variant);
    ASSERT_TRUE(eo.isDecoded());
}

TEST_F(ExtensionObjectTest, IsType)
{
    const RangePtr range = Range(0, 10);
    auto variant = VariantConverter<IRange>::ToVariant(range);

    ExtensionObject eo(variant);
    ASSERT_TRUE(eo.isType<UA_Range>());
    ASSERT_FALSE(eo.isType<UA_KeyValuePair>());
}

TEST_F(ExtensionObjectTest, VariantConversion)
{
    const RangePtr range = Range(0, 10);
    const RangePtr rangeWrong = Range(10, 20);
    const auto variant = VariantConverter<IRange>::ToVariant(range);

    auto eo = ExtensionObject();
    eo.setFromVariant(variant);
    const auto variantOut = eo.getAsVariant();
    const RangePtr rangeOut = VariantConverter<IRange>::ToDaqObject(variantOut);

    ASSERT_TRUE(rangeOut.equals(range));
    ASSERT_FALSE(rangeOut.equals(rangeWrong));
}

TEST_F(ExtensionObjectTest, ExtensionObjectConstructor)
{
    UA_Range tmsRangeSrc = {0, 10};
    RangePtr rangeSrc = Range(0, 10);
    RangePtr rangeWrong = Range(10, 20);

    OpcUaObject<UA_ExtensionObject> eoSrc;
    UA_ExtensionObject_setValueCopy(eoSrc.get(), &tmsRangeSrc, &UA_TYPES[UA_TYPES_RANGE]);

    ExtensionObject eo(eoSrc);
    const auto variantOut = eo.getAsVariant();
    const auto rangeOut = VariantConverter<IRange>::ToDaqObject(variantOut);

    ASSERT_TRUE(rangeOut.equals(rangeSrc));
    ASSERT_FALSE(rangeOut.equals(rangeWrong));
}
