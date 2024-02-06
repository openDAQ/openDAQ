#include "opcuatms_client/objects/tms_client_tags_impl.h"
#include "opendaq/tags_factory.h"
#include "opendaq/custom_log.h"
#include "opcuatms/errors.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

TmsClientTagsImpl::TmsClientTagsImpl(const ContextPtr& ctx, const TmsClientContextPtr& clientContext, const opcua::OpcUaNodeId& nodeId)
    : TmsClientObjectImpl(ctx, clientContext, nodeId)
    , TagsImpl()
    , loggerComponent(ctx.getLogger().getOrAddComponent("OpcUaClient"))
{
}

ErrCode TmsClientTagsImpl::getList(IList** value)
{
    refreshTags();
    return TagsImpl::getList(value);
}

ErrCode TmsClientTagsImpl::add(IString* name)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
}

ErrCode TmsClientTagsImpl::set(IList* tags)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
}

ErrCode TmsClientTagsImpl::remove(IString* name)
{
    return OPENDAQ_ERR_OPCUA_CLIENT_CALL_NOT_AVAILABLE;
}

ErrCode TmsClientTagsImpl::contains(IString* name, Bool* value)
{
    refreshTags();
    return TagsImpl::contains(name, value);
}

ErrCode TmsClientTagsImpl::query(IString* query, Bool* value)
{
    refreshTags();
    return TagsImpl::query(query, value);
}

void TmsClientTagsImpl::refreshTags()
{
    try
    {
        const ListPtr<IString> tagValues = VariantConverter<IString>::ToDaqList(client->readValue(nodeId));
        this->tags.clear();
        for (const auto& tag : tagValues)
            this->tags.insert(tag);
    }
    catch([[maybe_unused]] const std::exception& e)
    {
        LOG_D("OPC UA failed to fetch tags: {}", e.what())
    }
    catch(...)
    {
        LOG_D("OPC UA failed to fetch tags.")
    }
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
