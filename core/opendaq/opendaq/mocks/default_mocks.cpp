#include <opendaq/mock/default_mocks.h>
#include <opendaq/channel_impl.h>
#include <opendaq/device_impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    INTERNAL_FACTORY, Channel,
    IChannel, createDefaultChannel,
    IFunctionBlockType*, fbType,
    IContext*, context,
    IComponent*, parent,
    IString*, localId
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    INTERNAL_FACTORY, FunctionBlock,
    IFunctionBlock, createDefaultFunctionBlock,
    IFunctionBlockType*, fbType,
    IContext*, context,
    IComponent*, parent,
    IString*, localId
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    INTERNAL_FACTORY, DefaultDevice,
    IDevice, createDefaultDevice,
    IContext*, context,
    IComponent*, parent,
    IString*, localId
)

END_NAMESPACE_OPENDAQ
