#include <coretypes/function_custom_impl.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY,
    CustomFunctionImpl<FuncCall>,
    IFunction,
    createFunction,
    FuncCall, func
)

END_NAMESPACE_OPENDAQ
