#include <testutils/testutils.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/eval_value_factory.h>

using namespace daq;

class SelectionPropertyTest : public testing::Test
{
public:
    void SetUp() override
    {
        obj = PropertyObject();

        obj.addProperty(SparseSelectionProperty("SparseSelectionInt", Dict<IInteger, IInteger>({{0, 10}, {5, 20}}), 5));
        obj.addProperty(SparseSelectionProperty("SparseSelectionString", Dict<IInteger, IString>({{0, "foo"}, {10, "bar"}}), 0));

        obj.addProperty(SelectionProperty("IndexSelectionInt", List<IInteger>(10, 20, 30), 1));
        obj.addProperty(SelectionProperty("IndexSelectionString", List<IString>("foo", "bar"), 0));
		
        // Non-index/sparse selections
		obj.addProperty(StringPropertyBuilder("StringSelection", "foo").setSelectionValues(List<IString>("foo", "bar")).build());
		obj.addProperty(IntPropertyBuilder("IntSelection", 10).setSelectionValues(List<IInteger>(0, 6, 15, 10)).setIsValueSelectionProperty(true).build());
		obj.addProperty(FloatPropertyBuilder("FloatSelection", 5.12).setSelectionValues(List<IFloat>(0.12, -5.2, 5.12, 10.2)).build());
        
        // Non-index/sparse selections w/ references
        obj.addProperty(ListProperty("StringList", List<IString>("foo", "bar"), false));
        obj.addProperty(ListProperty("IntList", List<IInteger>(0, 6, 15, 10), false));
        obj.addProperty(ListProperty("FloatList", List<IFloat>(0.12, -5.2, 5.12, 10.2), false));

        //obj.addProperty(StringPropertyBuilder("StringSelectionRef", "foo").setSelectionValues(EvalValue("StringList")).build());
		//obj.addProperty(IntPropertyBuilder("IntSelectionRef", 10).setSelectionValues(EvalValue("IntList")).setIsValueSelectionProperty(true).build());
		//obj.addProperty(FloatPropertyBuilder("FloatSelectionRef", 5.12).setSelectionValues(EvalValue("FloatList")).build());

        obj.addProperty(IntProperty("CallCount", 0));
        
        sparseSelectionInt = obj.getProperty("SparseSelectionInt");
	    sparseSelectionString = obj.getProperty("SparseSelectionString");

	    indexSelectionInt = obj.getProperty("IndexSelectionInt");
	    indexSelectionString = obj.getProperty("IndexSelectionString");

	    stringSelection = obj.getProperty("StringSelection");
	    intSelection = obj.getProperty("IntSelection");
	    floatSelection = obj.getProperty("FloatSelection");

	    // stringSelectionRef = obj.getProperty("StringSelectionRef");
	    // intSelectionRef = obj.getProperty("IntSelectionRef");
	    // floatSelectionRef = obj.getProperty("FloatSelectionRef");
    }

    PropertyObjectPtr obj;

    PropertyPtr sparseSelectionInt;
    PropertyPtr sparseSelectionString;

	PropertyPtr indexSelectionInt;
    PropertyPtr indexSelectionString;

	PropertyPtr stringSelection;
    PropertyPtr intSelection;
    PropertyPtr floatSelection;

	// PropertyPtr stringSelectionRef;
    // PropertyPtr intSelectionRef;
    // PropertyPtr floatSelectionRef;
};

TEST_F(SelectionPropertyTest, SelectionTypes)
{
    ASSERT_EQ(sparseSelectionInt.getPropertyType(), PropertyType::SparseSelection);
    ASSERT_EQ(sparseSelectionString.getPropertyType(), PropertyType::SparseSelection);
    ASSERT_EQ(indexSelectionInt.getPropertyType(), PropertyType::IndexSelection);
    ASSERT_EQ(indexSelectionString.getPropertyType(), PropertyType::IndexSelection);
    ASSERT_EQ(stringSelection.getPropertyType(), PropertyType::Selection);
    ASSERT_EQ(intSelection.getPropertyType(), PropertyType::Selection);
    ASSERT_EQ(floatSelection.getPropertyType(), PropertyType::Selection);
    // ASSERT_EQ(stringSelectionRef.getPropertyType(), PropertyType::Selection);
    // ASSERT_EQ(intSelectionRef.getPropertyType(), PropertyType::Selection);
    // ASSERT_EQ(floatSelectionRef.getPropertyType(), PropertyType::Selection);
}

TEST_F(SelectionPropertyTest, SelectionValueValueTypes)
{
    ASSERT_EQ(sparseSelectionInt.getValueType(), CoreType::ctInt);
	ASSERT_EQ(sparseSelectionString.getValueType(), CoreType::ctInt);
	ASSERT_EQ(indexSelectionInt.getValueType(), CoreType::ctInt);
	ASSERT_EQ(indexSelectionString.getValueType(), CoreType::ctInt);
	ASSERT_EQ(stringSelection.getValueType(), CoreType::ctString);
	ASSERT_EQ(intSelection.getValueType(), CoreType::ctInt);
	ASSERT_EQ(floatSelection.getValueType(), CoreType::ctFloat);
	// ASSERT_EQ(stringSelectionRef.getValueType(), CoreType::ctString);
	// ASSERT_EQ(intSelectionRef.getValueType(), CoreType::ctInt);
	// ASSERT_EQ(floatSelectionRef.getValueType(), CoreType::ctFloat);
}

TEST_F(SelectionPropertyTest, SelectionValueKeyTypes)
{
    ASSERT_EQ(sparseSelectionInt.getKeyType(), CoreType::ctUndefined);
	ASSERT_EQ(sparseSelectionString.getKeyType(), CoreType::ctUndefined);
	ASSERT_EQ(indexSelectionInt.getKeyType(), CoreType::ctUndefined);
	ASSERT_EQ(indexSelectionString.getKeyType(), CoreType::ctUndefined);
	ASSERT_EQ(stringSelection.getKeyType(), CoreType::ctUndefined);
	ASSERT_EQ(intSelection.getKeyType(), CoreType::ctUndefined);
	ASSERT_EQ(floatSelection.getKeyType(), CoreType::ctUndefined);
    // ASSERT_EQ(stringSelectionRef.getKeyType(), CoreType::ctUndefined);
    // ASSERT_EQ(intSelectionRef.getKeyType(), CoreType::ctUndefined);
    // ASSERT_EQ(floatSelectionRef.getKeyType(), CoreType::ctUndefined);
}

TEST_F(SelectionPropertyTest, SelectionValueItemTypes)
{
    ASSERT_EQ(sparseSelectionInt.getItemType(), CoreType::ctInt);
	ASSERT_EQ(sparseSelectionString.getItemType(), CoreType::ctString);
	ASSERT_EQ(indexSelectionInt.getItemType(), CoreType::ctInt);
	ASSERT_EQ(indexSelectionString.getItemType(), CoreType::ctString);
	ASSERT_EQ(stringSelection.getItemType(), CoreType::ctString);
	ASSERT_EQ(intSelection.getItemType(), CoreType::ctInt);
	ASSERT_EQ(floatSelection.getItemType(), CoreType::ctFloat);
    // ASSERT_EQ(stringSelectionRef.getItemType(), CoreType::ctUndefined);
    // ASSERT_EQ(intSelectionRef.getItemType(), CoreType::ctUndefined);
    // ASSERT_EQ(floatSelectionRef.getItemType(), CoreType::ctUndefined);
}

TEST_F(SelectionPropertyTest, ValidWriteIndexSparseSelection)
{
    ASSERT_NO_THROW(sparseSelectionInt.setValue(0));
    ASSERT_EQ(sparseSelectionInt.getValue(), 0);
    ASSERT_NO_THROW(obj.setPropertyValue("SparseSelectionInt", 5));
    ASSERT_EQ(obj.getPropertySelectionValue("SparseSelectionInt"), 20u);

    ASSERT_NO_THROW(sparseSelectionString.setValue(10));
    ASSERT_EQ(sparseSelectionString.getValue(), 10u);
    ASSERT_NO_THROW(obj.setPropertyValue("SparseSelectionString", 0));
    ASSERT_EQ(obj.getPropertySelectionValue("SparseSelectionString"), "foo");

    ASSERT_NO_THROW(indexSelectionInt.setValue(0));
    ASSERT_EQ(indexSelectionInt.getValue(), 0u);
    ASSERT_NO_THROW(obj.setPropertyValue("IndexSelectionInt", 1));
    ASSERT_EQ(obj.getPropertySelectionValue("IndexSelectionInt"), 20u);

    ASSERT_NO_THROW(indexSelectionString.setValue(1));
    ASSERT_EQ(indexSelectionString.getValue(), 1);
    ASSERT_NO_THROW(obj.setPropertyValue("IndexSelectionString", 0));
    ASSERT_EQ(obj.getPropertySelectionValue("IndexSelectionString"), "foo");
}

TEST_F(SelectionPropertyTest, InvalidWriteIndexSparseSelection)
{
    ASSERT_THROW(sparseSelectionInt.setValue(3), NotFoundException);
    ASSERT_THROW(obj.setPropertyValue("SparseSelectionInt", -3), NotFoundException);

    ASSERT_THROW(sparseSelectionString.setValue(20), NotFoundException);
    ASSERT_THROW(obj.setPropertyValue("SparseSelectionString", -20), NotFoundException);

    ASSERT_THROW(indexSelectionInt.setValue(3), NotFoundException);
    ASSERT_THROW(obj.setPropertyValue("IndexSelectionInt", -3), NotFoundException);

    ASSERT_THROW(indexSelectionString.setValue(2), NotFoundException);
    ASSERT_THROW(obj.setPropertyValue("IndexSelectionString", -2), NotFoundException);
}

TEST_F(SelectionPropertyTest, ValidWriteValueBasedSelection)
{
    ASSERT_NO_THROW(stringSelection.setValue("bar"));
    ASSERT_EQ(stringSelection.getValue(), "bar");
    ASSERT_NO_THROW(obj.setPropertyValue("StringSelection", "foo"));
    ASSERT_EQ(obj.getPropertyValue("StringSelection"), "foo");

    ASSERT_NO_THROW(intSelection.setValue(6));
    ASSERT_EQ(intSelection.getValue(), 6u);
    ASSERT_NO_THROW(obj.setPropertyValue("IntSelection", 10));
    ASSERT_EQ(obj.getPropertyValue("IntSelection"), 10u);

    ASSERT_NO_THROW(floatSelection.setValue(10.2));
    ASSERT_DOUBLE_EQ(floatSelection.getValue(), 10.2);
    ASSERT_NO_THROW(obj.setPropertyValue("FloatSelection", -5.2));
    ASSERT_DOUBLE_EQ(obj.getPropertyValue("FloatSelection"), -5.2);
}

TEST_F(SelectionPropertyTest, InvalidWriteValueBasedSelection)
{
    ASSERT_THROW(stringSelection.setValue("foobar"), NotFoundException);
    ASSERT_THROW(obj.setPropertyValue("StringSelection", "foobar"), NotFoundException);
    ASSERT_THROW(intSelection.setValue(5), NotFoundException);
    ASSERT_THROW(obj.setPropertyValue("IntSelection", 5), NotFoundException);
    ASSERT_THROW(floatSelection.setValue(0.5), NotFoundException);
    ASSERT_THROW(obj.setPropertyValue("FloatSelection", 0.5), NotFoundException);
}

TEST_F(SelectionPropertyTest, ReadSelectionValue)
{
    // TODO: What does this throw?
    ASSERT_EQ(obj.getPropertySelectionValue("StringSelection"), "foo");
    ASSERT_EQ(obj.getPropertySelectionValue("IntSelection"), 10u);
    ASSERT_EQ(obj.getPropertySelectionValue("FloatSelection"), 5.12f);
}

// TEST_F(SelectionPropertyTest, ReferenceBasedSelectionInvalidRead)
// {
//     ASSERT_EQ(stringSelectionRef.getValue(), "foo");
//     ASSERT_EQ(intSelectionRef.getValue(), 10u);
//     ASSERT_DOUBLE_EQ(floatSelectionRef.getValue(), 5.12);

//     // Modify list of selection values to point to invalid selection values
//     obj.setPropertyValue("StringList", List<IString>("apple", "banana"));
//     obj.setPropertyValue("IntList", List<IInteger>(1, 2, 3));
//     obj.setPropertyValue("FloatList", List<IFloat>(0.1, 0.2, 0.3));

//     ASSERT_THROW(stringSelectionRef.getValue(), NotFoundException);
//     ASSERT_THROW(intSelectionRef.getValue(), NotFoundException);
//     ASSERT_THROW(floatSelectionRef.getValue(), NotFoundException);
// }

static void onWriteSelection(PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
{
    int callCount = obj.getPropertyValue("CallCount");
    obj.setPropertyValue("CallCount", callCount + 1);

    if (args.getPropertyEventType() == PropertyEventType::Clear)
        return;

    auto newVal = args.getValue();
    if (args.getProperty().getValueType() == ctInt)
    {
        ASSERT_EQ(newVal, 6u);
    }
    else if (args.getProperty().getValueType() == ctString)
    {
        ASSERT_EQ(newVal, "bar");
    }
    else if (args.getProperty().getValueType() == ctFloat)
    {
        ASSERT_DOUBLE_EQ(newVal, 10.2);
    }
}

TEST_F(SelectionPropertyTest, ValidWriteEventSelection)
{
    stringSelection.getOnPropertyValueWrite() += onWriteSelection;
    intSelection.getOnPropertyValueWrite() += onWriteSelection;
    floatSelection.getOnPropertyValueWrite() += onWriteSelection;

    stringSelection.setValue("bar");
    intSelection.setValue(6);
    floatSelection.setValue(10.2);

    ASSERT_EQ(obj.getPropertyValue("CallCount"), 3u);
}

static void onWriteSelectionCoerce(PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
{
    int callCount = obj.getPropertyValue("CallCount");
    obj.setPropertyValue("CallCount", callCount + 1);

    auto newVal = args.getValue();
    ListPtr<IBaseObject> vals = args.getProperty().getSelectionValues();
    for (const auto& val : vals)
    {
        if (args.getProperty().getValueType() == ctFloat)
        {
            // Floating point comparison
            if (std::abs(static_cast<double>(newVal) - static_cast<double>(val)) < 1e-6)
                return;
        }
        else if (val == newVal)
        {
            return;
        }
    }
    
    if (vals.getCount())
        args.setValue(vals[0]);
}

TEST_F(SelectionPropertyTest, InvalidWriteEventCoerce)
{
    stringSelection.getOnPropertyValueWrite() += onWriteSelectionCoerce;
    intSelection.getOnPropertyValueWrite() += onWriteSelectionCoerce;
    floatSelection.getOnPropertyValueWrite() += onWriteSelectionCoerce;

    ASSERT_ANY_THROW(stringSelection.setValue("foobar"));
    ASSERT_EQ(stringSelection.getValue(), "foo");

    ASSERT_ANY_THROW(intSelection.setValue(5));
    ASSERT_EQ(intSelection.getValue(), 10u);

    ASSERT_ANY_THROW(floatSelection.setValue(0.5));
    ASSERT_DOUBLE_EQ(floatSelection.getValue(), 5.12);

    // checking is happening before calling on write event, so call count should be 0
    ASSERT_EQ(obj.getPropertyValue("CallCount"), 0u);
}

TEST_F(SelectionPropertyTest, EndUpdateOnWriteEvent)
{
        
    stringSelection.getOnPropertyValueWrite() += onWriteSelectionCoerce;
    intSelection.getOnPropertyValueWrite() += onWriteSelectionCoerce;
    floatSelection.getOnPropertyValueWrite() += onWriteSelectionCoerce;

    obj.beginUpdate();

    // this is invalid but should not throw until end update is called
    ASSERT_NO_THROW(stringSelection.setValue("foobar"));
    ASSERT_NO_THROW(intSelection.setValue(5));
    ASSERT_NO_THROW(floatSelection.setValue(0.5));

    ASSERT_ANY_THROW(obj.endUpdate());

    ASSERT_EQ(stringSelection.getValue(), "foo");
    ASSERT_EQ(intSelection.getValue(), 10u);
    ASSERT_DOUBLE_EQ(floatSelection.getValue(), 5.12);

    ASSERT_EQ(obj.getPropertyValue("CallCount"), 0u);
}

TEST_F(SelectionPropertyTest, UpdatableOnWriteEvent)
{
    stringSelection.getOnPropertyValueWrite() += onWriteSelection;
    intSelection.getOnPropertyValueWrite() += onWriteSelection;
    floatSelection.getOnPropertyValueWrite() += onWriteSelection;

    stringSelection.setValue("bar");
    intSelection.setValue(6);
    floatSelection.setValue(10.2);

    auto ser = JsonSerializer();
    obj.serialize(ser);

    obj.clearPropertyValue("StringSelection");
    obj.clearPropertyValue("IntSelection");
    obj.clearPropertyValue("FloatSelection");

    ASSERT_EQ(stringSelection.getValue(), "foo");
    ASSERT_EQ(intSelection.getValue(), 10);
    ASSERT_DOUBLE_EQ(floatSelection.getValue(), 5.12);

    auto deser = JsonDeserializer();
    deser.update(obj, ser.getOutput());

    ASSERT_EQ(stringSelection.getValue(), "bar");
    ASSERT_EQ(intSelection.getValue(), 6u);
    ASSERT_DOUBLE_EQ(floatSelection.getValue(), 10.2);
}
