
#include <open62541/namespace_di_generated.h>
#include <open62541/di_nodeids.h>
#ifdef NAMESPACE_DAQBT
    #include <open62541/namespace_daqbt_generated.h>
    #include <open62541/daqbt_nodeids.h>
#endif
#ifdef NAMESPACE_DAQBSP
    #include <open62541/namespace_daqbsp_generated.h>
    #include <open62541/daqbsp_nodeids.h>
#endif
#ifdef NAMESPACE_DAQDEVICE
    #include <open62541/namespace_daqdevice_generated.h>
    #include <open62541/daqdevice_nodeids.h>
#endif
#ifdef NAMESPACE_DAQESP
    #include <open62541/namespace_daqesp_generated.h>
    #include <open62541/daqesp_nodeids.h>
#endif
#include "opcuashared/opcuaexception.h"
#include "opcuashared/opcualog.h"
#include "opcuaserver/opcuatmstypes.h"
#include <open62541/namespace_daqhbk_generated.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

void addTmsTypes(UA_Server *server)
{
    UA_StatusCode uaStatus = namespace_di_generated(server);
    CheckStatusCodeException(uaStatus, "Failed to add OPC-UA for devices nodeset.");
    LOGD << "OPC-UA for devices nodeSet was added successfully.";

#ifdef NAMESPACE_DAQBT
    uaStatus = namespace_daqbt_generated(server);
    CheckStatusCodeException(uaStatus, "Failed to add TMS BT nodeset.");
    LOGD << "TMS BT nodeset was added successfully.";
#endif

#ifdef NAMESPACE_DAQBSP
    uaStatus = namespace_daqbsp_generated(server);
    CheckStatusCodeException(uaStatus, "Failed to add TMS BSP nodeset.");
    LOGD << "TMS BSP nodeset was added successfully.";
#endif

#ifdef NAMESPACE_DAQDEVICE
    uaStatus = namespace_daqdevice_generated(server);
    CheckStatusCodeException(uaStatus, "Failed to add TMS DEVICE nodeset.");
    LOGD << "TMS DEVICE nodeset was added successfully.";
#endif

#ifdef NAMESPACE_DAQESP
    uaStatus = namespace_daqesp_generated(server);
    CheckStatusCodeException(uaStatus, "Failed to add TMS ESP nodeset.");
    LOGD << "TMS ESP nodeset was added successfully.";
#endif
    
    uaStatus = namespace_daqhbk_generated(server);
    CheckStatusCodeException(uaStatus, "Failed to add TMS HBK nodeset.");
    LOGD << "TMS HBK nodeset was added successfully.";
}

END_NAMESPACE_OPENDAQ_OPCUA
