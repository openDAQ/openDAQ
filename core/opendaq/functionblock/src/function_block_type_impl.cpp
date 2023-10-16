#include <opendaq/function_block_type_impl.h>
#include <opendaq/function_block_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr functionBlockTypeStructType = FunctionBlockTypeStructType();
}

FunctionBlockTypeImpl::FunctionBlockTypeImpl(const StringPtr& id,
                                             const StringPtr& name,
                                             const StringPtr& description,
                                             const FunctionPtr& createDefaultConfigCallback)
    : Super(detail::functionBlockTypeStructType, id, name, description, createDefaultConfigCallback)
{
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, FunctionBlockType,
    IString*, id,
    IString*, name,
    IString*, description,
    IFunction*, createDefaultConfigCallback
)

END_NAMESPACE_OPENDAQ
