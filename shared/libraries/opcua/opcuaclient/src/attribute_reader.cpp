#include <opcuaclient/attribute_reader.h>
#include <iostream>
#include <cmath>
#include <iostream>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

AttributeReader::AttributeReader(const OpcUaClientPtr& client, size_t maxBatchSize)
    : client(client)
    , maxBatchSize(maxBatchSize)
{
}

void AttributeReader::setAttibutes(const tsl::ordered_set<OpcUaAttribute>& attributes)
{
    this->attributes = attributes;
}

void AttributeReader::addAttribute(const OpcUaAttribute& attribute)
{
    attributes.insert(attribute);
}

OpcUaVariant AttributeReader::getValue(const OpcUaNodeId& nodeId, UA_AttributeId attributeId)
{
    if (resultMap.count(nodeId) == 0 || resultMap[nodeId].count(attributeId) == 0)
        throw OpcUaException(UA_STATUSCODE_BADNOTFOUND, "Attribute read result not found");

    return resultMap[nodeId][attributeId];
}

OpcUaVariant AttributeReader::getValue(const OpcUaAttribute& attribute)
{
    return getValue(attribute.nodeId, attribute.attributeId);
}

bool AttributeReader::hasAnyValue(const OpcUaNodeId& nodeId)
{
    return resultMap.count(nodeId) > 0;
}

void AttributeReader::clearResults()
{
    resultMap.clear();
}

void AttributeReader::clearAttributes()
{
    attributes.clear();
}

void AttributeReader::read()
{
    if (attributes.empty())
        return;

    const size_t batchSize = (maxBatchSize > 0) ? maxBatchSize : attributes.size();
    size_t read = 0;
    size_t toRead = batchSize;
    auto attrIterator = attributes.begin();

    while (read < attributes.size())
    {
        toRead = batchSize;
        if (read + toRead > attributes.size())
            toRead = attributes.size() - read;

        readBatch(attrIterator, toRead);
        read += toRead;
    }
}

void AttributeReader::readBatch(tsl::ordered_set<OpcUaAttribute>::iterator& attrIterator, size_t size)
{
    assert(size > 0);
    auto batchStartIterator = attrIterator;

    OpcUaObject<UA_ReadRequest> request;
    request->nodesToReadSize = size;
    request->nodesToRead = (UA_ReadValueId*) UA_Array_new(attributes.size(), &UA_TYPES[UA_TYPES_READVALUEID]);

    for (size_t i = 0; i < size; i++)
    {
        const auto& attribute = *attrIterator;
        request->nodesToRead[i].nodeId = attribute.nodeId.copyAndGetDetachedValue();
        request->nodesToRead[i].attributeId = attribute.attributeId;
        attrIterator++;
    }

    OpcUaObject<UA_ReadResponse> response = UA_Client_Service_read(client->getLockedUaClient(), *request);
    const auto status = response->responseHeader.serviceResult;

    if (status != UA_STATUSCODE_GOOD)
        throw OpcUaException(status, "Attribute read request failed");
    if (response->resultsSize != size)
        throw OpcUaException(UA_STATUSCODE_BADINVALIDSTATE, "Read request returned incorrect number of results");

    addBatchToResultMap(batchStartIterator, response);
}

void AttributeReader::addBatchToResultMap(tsl::ordered_set<OpcUaAttribute>::iterator attrIterator,
                                          const OpcUaObject<UA_ReadResponse>& response)
{
    for (size_t i = 0; i < response->resultsSize; i++)
    {
        const auto& attr = *attrIterator;

        if (resultMap.count(attr.nodeId) == 0)
            resultMap[attr.nodeId] = {};

        resultMap[attr.nodeId][attr.attributeId] = OpcUaVariant(response->results[i].value);
        attrIterator++;
    }
}

END_NAMESPACE_OPENDAQ_OPCUA
