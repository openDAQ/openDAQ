#include <coretypes/validation.h>
#include <opendaq/reference_domain_info_builder_impl.h>
#include <opendaq/reference_domain_info_builder_ptr.h>
#include <opendaq/reference_domain_info_factory.h>
#include <opendaq/reference_domain_info_impl.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
static const StructTypePtr referenceDomainInfoStructType = ReferenceDomainInfoStructType();
}

DictPtr<IString, IBaseObject> ReferenceDomainInfoImpl::PackBuilder(IReferenceDomainInfoBuilder* referenceDomainInfoBuilder)
{
    const auto builderPtr = ReferenceDomainInfoBuilderPtr::Borrow(referenceDomainInfoBuilder);
    auto params = Dict<IString, IBaseObject>();
    params.set("ReferenceDomainId", builderPtr.getReferenceDomainId());
    params.set("ReferenceDomainOffset", builderPtr.getReferenceDomainOffset());
    params.set("ReferenceTimeProtocol", static_cast<Int>(builderPtr.getReferenceTimeProtocol()));
    params.set("UsesOffset", static_cast<Int>(builderPtr.getUsesOffset()));
    return params;
}

ReferenceDomainInfoImpl::ReferenceDomainInfoImpl(IReferenceDomainInfoBuilder* referenceDomainInfoBuilder)
    : Super(detail::referenceDomainInfoStructType, PackBuilder(referenceDomainInfoBuilder))
{
    const auto dataDescriptorBuilderPtr = ReferenceDomainInfoBuilderPtr(referenceDomainInfoBuilder);
    this->referenceDomainId = dataDescriptorBuilderPtr.getReferenceDomainId();
    this->referenceDomainOffset = dataDescriptorBuilderPtr.getReferenceDomainOffset();
    this->referenceTimeProtocol = dataDescriptorBuilderPtr.getReferenceTimeProtocol();
    this->usesOffset = dataDescriptorBuilderPtr.getUsesOffset();
}

ErrCode INTERFACE_FUNC ReferenceDomainInfoImpl::getReferenceDomainId(IString** referenceDomainId)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainId);

    *referenceDomainId = this->referenceDomainId.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ReferenceDomainInfoImpl::getReferenceDomainOffset(IInteger** referenceDomainOffset)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainOffset);

    *referenceDomainOffset = this->referenceDomainOffset.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ReferenceDomainInfoImpl::getReferenceTimeProtocol(TimeProtocol* referenceTimeProtocol)
{
    OPENDAQ_PARAM_NOT_NULL(referenceTimeProtocol);

    *referenceTimeProtocol = this->referenceTimeProtocol;

    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ReferenceDomainInfoImpl::getUsesOffset(UsesOffset* usesOffset)
{
    OPENDAQ_PARAM_NOT_NULL(usesOffset);

    *usesOffset = this->usesOffset;

    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ReferenceDomainInfoImpl::equals(IBaseObject* other, Bool* equals) const
{
    const ErrCode errCode = daqTry([this, &other, &equals]()
    {
        if (equals == nullptr)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

        *equals = false;
        if (!other)
            return OPENDAQ_SUCCESS;

        ReferenceDomainInfoPtr info = BaseObjectPtr::Borrow(other).asPtrOrNull<IReferenceDomainInfo>();
        if (info == nullptr)
            return OPENDAQ_SUCCESS;

        if (!BaseObjectPtr::Equals(referenceDomainId, info.getReferenceDomainId()))
            return OPENDAQ_SUCCESS;
        if (!BaseObjectPtr::Equals(referenceDomainOffset, info.getReferenceDomainOffset()))
            return OPENDAQ_SUCCESS;
        if (referenceTimeProtocol != info.getReferenceTimeProtocol())
            return OPENDAQ_SUCCESS;
        if (usesOffset != info.getUsesOffset())
            return OPENDAQ_SUCCESS;

        *equals = true;
        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

ErrCode ReferenceDomainInfoImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    {
        if (referenceDomainId.assigned())
        {
            serializer->key("referenceDomainId");
            serializer->writeString(referenceDomainId.getCharPtr(), referenceDomainId.getLength());
        }

        if (referenceDomainOffset.assigned())
        {
            serializer->key("referenceDomainOffset");
            serializer->writeInt(referenceDomainOffset);
        }

        serializer->key("referenceTimeProtocol");
        serializer->writeInt(static_cast<Int>(referenceTimeProtocol));

        serializer->key("usesOffset");
        serializer->writeInt(static_cast<Int>(usesOffset));
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode ReferenceDomainInfoImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr ReferenceDomainInfoImpl::SerializeId()
{
    return "ReferenceDomainInfo";
}

ErrCode ReferenceDomainInfoImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    ReferenceDomainInfoBuilderPtr dataDescriptor;
    auto errCode = createObject<IReferenceDomainInfoBuilder, ReferenceDomainInfoBuilderImpl>(&dataDescriptor);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);

    if (serializedObj.hasKey("referenceDomainId"))
    {
        auto referenceDomainId = serializedObj.readString("referenceDomainId");
        dataDescriptor.setReferenceDomainId(referenceDomainId);
    }

    if (serializedObj.hasKey("referenceDomainOffset"))
    {
        auto referenceDomainOffset = serializedObj.readInt("referenceDomainOffset");
        dataDescriptor.setReferenceDomainOffset(referenceDomainOffset);
    }

    if (serializedObj.hasKey("referenceTimeProtocol"))
    {
        auto referenceTimeProtocol = static_cast<TimeProtocol>(serializedObj.readInt("referenceTimeProtocol"));
        dataDescriptor.setReferenceTimeProtocol(referenceTimeProtocol);
    }

    if (serializedObj.hasKey("usesOffset"))
    {
        auto usesOffset = static_cast<UsesOffset>(serializedObj.readInt("usesOffset"));
        dataDescriptor.setUsesOffset(usesOffset);
    }

    *obj = dataDescriptor.build().as<IBaseObject>();

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, ReferenceDomainInfo, IReferenceDomainInfo, createReferenceDomainInfoFromBuilder, IReferenceDomainInfoBuilder*, builder)

END_NAMESPACE_OPENDAQ
