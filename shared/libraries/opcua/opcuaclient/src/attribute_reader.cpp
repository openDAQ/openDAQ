#include <opcuaclient/attribute_reader.h>
#include <iostream>
#include <cmath>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

AttributeReader::AttributeReader(const OpcUaClientPtr& client, size_t maxBatchSize)
    : client(client)
    , maxBatchSize(maxBatchSize)
{
}

void AttributeReader::addAttribute(const OpcUaAttribute& attribute)
{
    attributes.push_back(attribute);
}

OpcUaDataValuePtr AttributeReader::getValue(const OpcUaNodeId& nodeId, UA_AttributeId attributeId)
{
    if (resultMap.count(nodeId) == 0 || resultMap[nodeId].count(attributeId) == 0)
        throw OpcUaException(UA_STATUSCODE_BADNOTFOUND, "Attribute read result not found");

    return resultMap[nodeId][attributeId];
}

OpcUaDataValuePtr AttributeReader::getValue(const OpcUaAttribute& attribute)
{
    return getValue(attribute.nodeId, attribute.attributeId);
}

void AttributeReader::reset()
{
    attributes.clear();
    resultMap.clear();
    responses.clear();
}

void AttributeReader::read()
{
    resultMap.clear();
    responses.clear();

    if (attributes.empty())
        return;

    const size_t batchSize = (maxBatchSize > 0) ? maxBatchSize : attributes.size();
    const size_t numberOfBatches = std::ceil((double) attributes.size() / batchSize);
    size_t i = 0;
    responses.reserve(numberOfBatches);

    while (i < attributes.size())
        i += readBatch(i, batchSize);
}

const std::vector<OpcUaObject<UA_ReadResponse>>& AttributeReader::getResponses()
{
    return responses;
}

size_t AttributeReader::readBatch(size_t startIndex, size_t size)
{
    if (startIndex + size > attributes.size())
        size = attributes.size() - startIndex;

    OpcUaObject<UA_ReadRequest> request;
    request->nodesToReadSize = size;
    request->nodesToRead = (UA_ReadValueId*) UA_Array_new(attributes.size(), &UA_TYPES[UA_TYPES_READVALUEID]);

    for (size_t i = 0; i < size; i++)
    {
        const auto& attribute = attributes[startIndex + i];

        request->nodesToRead[i].nodeId = attribute.nodeId.copyAndGetDetachedValue();
        request->nodesToRead[i].attributeId = attribute.attributeId;
    }

    responses.emplace_back(UA_Client_Service_read(client->getLockedUaClient(), *request));

    const auto& response = responses.back();
    const auto status = response->responseHeader.serviceResult;

    if (status != UA_STATUSCODE_GOOD)
        throw OpcUaException(status, "Attribute read request failed");

    addBatchToResultMap(startIndex, response);
    return size;
}

void AttributeReader::addBatchToResultMap(size_t startIndex, const OpcUaObject<UA_ReadResponse>& response)
{
    for (size_t i = 0; i < response->resultsSize; i++)
    {
        const auto& attr = attributes[startIndex + i];

        const auto value = std::make_shared<OpcUaDataValue>(response->results + i);

        if (resultMap.count(attr.nodeId) == 0)
            resultMap[attr.nodeId] = {};

        resultMap[attr.nodeId][attr.attributeId] = value;
    }
}

END_NAMESPACE_OPENDAQ_OPCUA
