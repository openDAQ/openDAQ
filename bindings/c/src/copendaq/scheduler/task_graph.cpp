//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.7.0) on 03.06.2025 22:07:34.
// </auto-generated>
//------------------------------------------------------------------------------

#include <copendaq/scheduler/task_graph.h>

#include <opendaq/opendaq.h>

#include <copendaq_private.h>

const daqIntfID DAQ_TASK_GRAPH_INTF_ID = { daq::ITaskGraph::Id.Data1, daq::ITaskGraph::Id.Data2, daq::ITaskGraph::Id.Data3, daq::ITaskGraph::Id.Data4_UInt64 };

daqErrCode daqTaskGraph_createTaskGraph(daqTaskGraph** obj, daqProcedure* work, daqString* name)
{
    daq::ITaskGraph* ptr = nullptr;
    daqErrCode err = daq::createTaskGraph(&ptr, reinterpret_cast<daq::IProcedure*>(work), reinterpret_cast<daq::IString*>(name));
    *obj = reinterpret_cast<daqTaskGraph*>(ptr);
    return err;
}
