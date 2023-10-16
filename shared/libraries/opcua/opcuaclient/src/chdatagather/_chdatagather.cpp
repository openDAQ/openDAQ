#include "opcuaclient/chdatagather/chdatagather.h"
#include "opcuaclient/opcuaclient.h"

BEGIN_NAMESPACE_OPCUA

ChDataGather::ChDataGather(OpcUaClient* client)
{
    this->client = client;
}

OpcUaClient* ChDataGather::getClient()
{
    return client;
}

void ChDataGather::start()
{
}

void ChDataGather::getData(RelativeTimeInSeconds seconds) //TODO align start... yes/no
{
    for (const auto& c : *getClient())
    {
        for (const auto& node : c->getAcqNodes())
        {
            node->getData(seconds);
        }
    }
}

void ChDataGather::stop()
{
}

END_NAMESPACE_OPCUA
