#include "opcuaclient/browser/opcuabrowser.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaBrowser::OpcUaBrowser(const OpcUaNodeId& nodeId, const OpcUaClientPtr& client)
    : request(OpcUaObject<UA_BrowseRequest>())
    , client(client)
{
    request->requestedMaxReferencesPerNode = 0;
    request->nodesToBrowse = UA_BrowseDescription_new();
    request->nodesToBrowseSize = 1;
    request->nodesToBrowse[0].nodeId = nodeId.copyAndGetDetachedValue();
    request->nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
}

OpcUaBrowser::OpcUaBrowser(const OpcUaObject<UA_BrowseRequest>& request, const OpcUaClientPtr& client)
    : request(request)
    , client(client)
{
}

std::vector<UA_ReferenceDescription>& OpcUaBrowser::browse()
{
    transactions.clear();
    references.clear();

    OpcUaBrowseTransactionPtr tr = browseTransaction();

    while (tr->getResultsSize() > 0)
    {
        validateTransactionStatus(tr);
        transactions.push_back(tr);

        for (size_t i = 0; i < tr->getResultsSize(); i++)
        {
            UA_BrowseResult& result = tr->getResults()[i];

            if (result.statusCode == UA_STATUSCODE_BADUSERACCESSDENIED)
                continue;
            if (result.statusCode == UA_STATUSCODE_BADNODEIDUNKNOWN)
                continue;

            CheckStatusCodeException(result.statusCode, "Browse result error");
            references.insert(references.end(), &result.references[0], &result.references[result.referencesSize]);
        }

        UA_ByteString* contPoint = &tr->getResults()[0].continuationPoint;
        if (contPoint->length == 0)
            break;

        tr = this->browseNextTransaction(contPoint);
    }

    return references;
}

tsl::ordered_map<OpcUaNodeId, OpcUaObject<UA_ReferenceDescription>> OpcUaBrowser::referencesByNodeId()
{
    auto refMap = tsl::ordered_map<OpcUaNodeId, OpcUaObject<UA_ReferenceDescription>>();

    for (const auto& ref : references)
    {
        OpcUaNodeId nodeId = ref.nodeId.nodeId;
        refMap.insert({nodeId, OpcUaObject<UA_ReferenceDescription>(ref)});
    }

    return refMap;
}

tsl::ordered_map<std::string, OpcUaObject<UA_ReferenceDescription>> OpcUaBrowser::referencesByBrowseName()
{
    auto refMap = tsl::ordered_map<std::string, OpcUaObject<UA_ReferenceDescription>>();

    for (const auto& ref : references)
    {
        std::string name = utils::ToStdString(ref.displayName.text);
        refMap.insert({name, OpcUaObject<UA_ReferenceDescription>(ref)});
    }

    return refMap;
}

OpcUaBrowseTransactionPtr OpcUaBrowser::browseTransaction()
{
    auto transaction = std::make_shared<OpcUaBrowseTransaction_First>();
    transaction->response = UA_Client_Service_browse(client->getLockedUaClient(), *request);

    return transaction;
}

OpcUaBrowseTransactionPtr OpcUaBrowser::browseNextTransaction(UA_ByteString* contPoint)
{
    auto nextRequest = prepareNextRequest(contPoint);

    auto transaction = std::make_shared<OpcUaBrowseTransaction_Next>();
    transaction->response = UA_Client_Service_browseNext(client->getLockedUaClient(), *nextRequest);

    return transaction;
}

OpcUaObject<UA_BrowseNextRequest> OpcUaBrowser::prepareNextRequest(UA_ByteString* contPoint)
{
    OpcUaObject<UA_BrowseNextRequest> nextRequest;

    nextRequest->releaseContinuationPoints = UA_FALSE;
    nextRequest->continuationPointsSize = 1u;
    nextRequest->continuationPoints = contPoint;

    return nextRequest;
}

void OpcUaBrowser::validateTransactionStatus(const OpcUaBrowseTransactionPtr& transaction)
{
    CheckStatusCodeException(transaction->getResponseHeader().serviceResult,
                             "Browse transaction error, transaction:" + std::to_string(transactions.size() + 1));
}

END_NAMESPACE_OPENDAQ_OPCUA
