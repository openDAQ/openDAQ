#include <gtest/gtest.h>
#include <opcuashared/opcuadatatypearraylist.h>
#include <open62541/types_generated.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

typedef struct {
    UA_Float x;
    UA_Float y;
} Struct1;
static UA_DataTypeMember struct1_members[2] = {
    /* a */
    {
        UA_TYPENAME("a")                // .typeName
        &UA_TYPES[UA_TYPES_FLOAT],      // .memberType
        0,                              // .padding
        false,                          // .isArray
        false                           // .isOptional
    },
    /* b */
    {
        UA_TYPENAME("b")                // .typeName
        &UA_TYPES[UA_TYPES_FLOAT],      // .memberType
        0,                              // .padding
        false,                          // .isArray
        false                           // .isOptional
    }
};
static const UA_DataType struct1Type = {
    UA_TYPENAME("Struct1")              // .typeName
    {1, UA_NODEIDTYPE_NUMERIC, {4242}}, // .typeId
    {1, UA_NODEIDTYPE_NUMERIC, {1}},    // .binaryEncodingId
    sizeof(Struct1),                    // .memSize
    UA_DATATYPEKIND_STRUCTURE,          // .typeKind
    true,                               // .pointerFree
    false,                              // .overlayable
    2,                                  // .membersize
    struct1_members                     // .members
};

typedef struct {
    UA_Float x;
    UA_Float y;
    UA_Float z;
} Struct2;
static UA_DataTypeMember struct2_members[3] = {
    /* x */
    {
        UA_TYPENAME("x")                // .typeName
        &UA_TYPES[UA_TYPES_FLOAT],      // .memberType
        0,                              // .padding
        false,                          // .isArray
        false                           // .isOptional
    },
    /* y */
    {
        UA_TYPENAME("y")                // .typeName
        &UA_TYPES[UA_TYPES_FLOAT],      // .memberType
        0,                              // .padding
        false,                          // .isArray
        false                           // .isOptional
    },
    /* z */
    {
        UA_TYPENAME("z")                // .typeName
        &UA_TYPES[UA_TYPES_FLOAT],      // .memberType
        0,                              // .padding
        false,                          // .isArray
        false                           // .isOptional
    },
};
static const UA_DataType struct2Type = {
    UA_TYPENAME("Struct2")              // .typeName
    {1, UA_NODEIDTYPE_NUMERIC, {4243}}, // .typeId
    {1, UA_NODEIDTYPE_NUMERIC, {2}},    // .binaryEncodingId
    sizeof(Struct2),                    // .memSize
    UA_DATATYPEKIND_STRUCTURE,          // .typeKind
    true,                               // .pointerFree
    false,                              // .overlayable
    3,                                  // .membersize
    struct2_members                     // .members
};

TEST(OpcUaDataTypeArrayListTest, EmptyList)
{
    OpcUaDataTypeArrayList arrList;
    ASSERT_EQ(arrList.getCustomDataTypes(), nullptr);
}

TEST(OpcUaDataTypeArrayListTest, SingleListElement)
{
    UA_DataType types[1];
    types[0] = struct1Type;

    OpcUaDataTypeArrayList arrList;

    arrList.add(1, types);
    const UA_DataTypeArray* dataType = arrList.getCustomDataTypes();

    ASSERT_EQ(dataType->next, nullptr);
    ASSERT_EQ(dataType->typesSize, 1u);
}

TEST(OpcUaDataTypeArrayListTest, MultipleListElements)
{
    UA_DataType types1[1];
    types1[0] = struct1Type;
    UA_DataType types2[1];
    types2[0] = struct2Type;

    OpcUaDataTypeArrayList arrList;
    arrList.add(1, types1);
    arrList.add(1, types2);

    const UA_DataTypeArray* dataType = arrList.getCustomDataTypes();
    int count = 0;
    while (dataType != NULL)
    {
        ++count;
        dataType = dataType->next;
    }

    ASSERT_EQ(count, 2);
}

TEST(OpcUaDataTypeArrayListTest, LargerTypeCount)
{
    UA_DataType types[2];
    types[0] = struct1Type;
    types[1] = struct2Type;

    OpcUaDataTypeArrayList arrList;
    arrList.add(2, types);

    const UA_DataTypeArray* dataType = arrList.getCustomDataTypes();

    ASSERT_EQ(dataType->typesSize, 2u);
}

END_NAMESPACE_OPENDAQ_OPCUA
