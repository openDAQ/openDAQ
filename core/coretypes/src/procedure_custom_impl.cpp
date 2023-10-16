#include <coretypes/procedure_custom_impl.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY,
    CustomProcedureImpl<ProcCall>,
    IProcedure,
    createProcedure,
    ProcCall, proc
)

END_NAMESPACE_OPENDAQ
