#include <numeric>
#include "opcuaclient/chdatagather/alignsyncchdatagather.h"
#include "opcuaclient/opcuaclient.h"

BEGIN_NAMESPACE_OPCUA

AlignSyncChannelDataGather::AlignSyncChannelDataGather(OpcUaClient* client)
    : ChDataGather(client)
    , previousSamplesAcquired(0)
{
}

void AlignSyncChannelDataGather::fillSyncChannelList()
{
    syncChannelList.clear();

    for (const auto& clientBase : *getClient())
    {
        if (clientBase->getClockMode() == ClockMode::MasterClock &&
            clientBase->getConnection()->getEndpoint().getServerType() == OpcUaServerType::Dewesoft)
        {
            for (const auto& node : clientBase->getAcqNodes())
            {
                if (node->getChannelType() == ChannelType::Sync)
                    syncChannelList.insert(node);
            }
        }
    }
}

void AlignSyncChannelDataGather::start()
{
    fillSyncChannelList();

    maxSyncSampleRate = OpcUaRatio();

    for (const auto& node : syncChannelList)
    {
        OpcUaRatio rSR = node->getRemoteSampleRate();
        if (!maxSyncSampleRate.isValid() || maxSyncSampleRate < rSR)
            maxSyncSampleRate = rSR;
    }

    srLCMDiv = 1;

    for (const auto& node : syncChannelList)
    {
        OpcUaRatio rSR = node->getRemoteSampleRate();
        OpcUaRatio div = maxSyncSampleRate / rSR;

        if ((div.numerator % div.denominator) != 0)
        {
            throw std::runtime_error("Can't align channels with sample rate " + rSR.toString() + " and " + div.toString());
        }

        srLCMDiv = std::lcm(srLCMDiv, div.intVal());
    }
}

void AlignSyncChannelDataGather::getData(RelativeTimeInSeconds seconds)
{
    auto samplesAcq = (getSamplesAcquired() / srLCMDiv) * srLCMDiv;

    for (const auto& c : *client)
    {
        for (const auto& node : c->getAcqNodes())
        {
            if (syncChannelList.find(node) != syncChannelList.end())
            {
                if (node->getTiming() && !node->getTiming()->masterClockDetails.isBufferOverrun())  // check if data are incomplete
                {
                    OpcUaRatio rSR = node->getRemoteSampleRate();
                    int64_t divider = (maxSyncSampleRate / rSR).intVal();
                    int64_t totalSyncSamples = node->getChannel()->getBufferProxy()->getTotalSyncSamples();
                    auto samplesAcqNode = samplesAcq / divider - totalSyncSamples;
                    node->getData(seconds, (size_t) samplesAcqNode);

                    assert(totalSyncSamples + samplesAcqNode == node->getChannel()->getBufferProxy()->getTotalSyncSamples());
                }
            }
            else
                node->getData(seconds);
        }
    }
    previousSamplesAcquired = samplesAcq;
}

int64_t AlignSyncChannelDataGather::getSamplesAcquired()
{
    int64_t samplesCountSync = INT64_MAX;

    for (const auto& node : syncChannelList)
    {
        if (node->getTiming())
        {
            OpcUaRatio rSR = node->getRemoteSampleRate();
            int64_t divider = (maxSyncSampleRate / rSR).intVal();
            int64_t samplesCnt = node->getTiming()->masterClockDetails.getSamplesAcquired() * divider;
            samplesCnt =
                std::min<int64_t>(samplesCnt,
                                  previousSamplesAcquired +
                                      (node->getChannel()->getBufferProxy()->getDBBufSize() - 1) *
                                          divider);  // limit to channel buffer size, so you move datalost error handling only to daqthread
            if (samplesCnt < samplesCountSync)
                samplesCountSync = samplesCnt;
        }
        else
        {
            samplesCountSync = 0;
            break;
        }
    }

    return (samplesCountSync < INT64_MAX) ? samplesCountSync : 0;
}

END_NAMESPACE_OPCUA
