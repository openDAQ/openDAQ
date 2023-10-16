#include "opcuashared/opcuadatatypearraylist.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaDataTypeArrayList::OpcUaDataTypeArrayList(const OpcUaDataTypeArrayList& copy)
    : std::list<UA_DataTypeArray>()
{
    operator=(copy);
}

OpcUaDataTypeArrayList& OpcUaDataTypeArrayList::operator=(const OpcUaDataTypeArrayList& other)
{
    if (this != &other)
    {
        clear();
        for (const auto& cur : other)
            add(cur.typesSize, cur.types);
    }
    return *this;
}

void OpcUaDataTypeArrayList::add(const size_t typesSize, const UA_DataType* types)
{
    UA_DataTypeArray dataTypeArray{nullptr, typesSize, types};

    const UA_DataTypeArray* nextElement = nullptr;
    if (!empty())
        nextElement = &front();

    UA_DataTypeArray newDataTypeArray = {nextElement, typesSize, types};
    push_front(newDataTypeArray);
}

const UA_DataTypeArray* OpcUaDataTypeArrayList::getCustomDataTypes() const
{
    if (empty())
        return nullptr;

    return &(*begin());
}

END_NAMESPACE_OPENDAQ_OPCUA
