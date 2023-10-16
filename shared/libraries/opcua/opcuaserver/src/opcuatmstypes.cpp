
#include <open62541/namespace_di_generated.h>
#include <open62541/di_nodeids.h>
#ifdef NAMESPACE_TMSBT
    #include <open62541/namespace_tmsbt_generated.h>
    #include <open62541/tmsbt_nodeids.h>
#endif
#ifdef NAMESPACE_TMSBSP
    #include <open62541/namespace_tmsbsp_generated.h>
    #include <open62541/tmsbsp_nodeids.h>
#endif
#ifdef NAMESPACE_TMSDEVICE
    #include <open62541/namespace_tmsdevice_generated.h>
    #include <open62541/tmsdevice_nodeids.h>
#endif
#ifdef NAMESPACE_TMSESP
    #include <open62541/namespace_tmsesp_generated.h>
    #include <open62541/tmsesp_nodeids.h>
#endif
#include "opcuashared/opcuaexception.h"
#include "opcuashared/opcualog.h"
#include "opcuaserver/opcuatmstypes.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

void addTmsTypes(UA_Server *server)
{
    UA_StatusCode uaStatus = namespace_di_generated(server);
    CheckStatusCodeException(uaStatus, "Failed to add OPC-UA for devices nodeset.");
    LOGD << "OPC-UA for devices nodeSet was added successfully.";

#ifdef NAMESPACE_TMSBT
    uaStatus = namespace_tmsbt_generated(server);
    CheckStatusCodeException(uaStatus, "Failed to add TMS BT nodeset.");
    LOGD << "TMS BT nodeset was added successfully.";
#endif

#ifdef NAMESPACE_TMSBSP
    uaStatus = namespace_tmsbsp_generated(server);
    CheckStatusCodeException(uaStatus, "Failed to add TMS BSP nodeset.");
    LOGD << "TMS BSP nodeset was added successfully.";
#endif

#ifdef NAMESPACE_TMSDEVICE
    uaStatus = namespace_tmsdevice_generated(server);
    CheckStatusCodeException(uaStatus, "Failed to add TMS DEVICE nodeset.");
    LOGD << "TMS DEVICE nodeset was added successfully.";
#endif

#ifdef NAMESPACE_TMSESP
    uaStatus = namespace_tmsesp_generated(server);
    CheckStatusCodeException(uaStatus, "Failed to add TMS ESP nodeset.");
    LOGD << "TMS ESP nodeset was added successfully.";
#endif
}

END_NAMESPACE_OPENDAQ_OPCUA
