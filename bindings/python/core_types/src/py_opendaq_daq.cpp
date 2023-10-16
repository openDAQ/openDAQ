#include "py_core_types/py_opendaq_daq.h"

py::handle blueberry_daq_module;
py::handle python_class_fraction;
std::unordered_map<daq::IntfID, std::function<py::object(const daq::ObjectPtr<daq::IBaseObject>&)>> daqInterfaceIdToClass;
