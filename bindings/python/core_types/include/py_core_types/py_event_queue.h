#pragma once
#include <coretypes/objectptr.h>
#include <coretypes/common.h>
#include <pybind11/pybind11.h>

BEGIN_NAMESPACE_OPENDAQ

class PyQueuedEventHandler;

void enqueuePythonEvent(daq::PyQueuedEventHandler* eventHandler, daq::ObjectPtr<daq::IBaseObject>sender, daq::ObjectPtr<daq::IEventArgs> eventArgs);
void processPythonEventFromQueue();
void clearPythonEventQueue();

END_NAMESPACE_OPENDAQ