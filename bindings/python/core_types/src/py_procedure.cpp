#include "py_core_types/py_core_types.h"

PyDaqIntf<daq::IProcedure> declareIProcedure(pybind11::module_ m)
{
    return wrapInterface<daq::IProcedure>(m, "IProcedure");
}

void defineIProcedure(pybind11::module_ m, PyDaqIntf<daq::IProcedure> cls)
{
    cls.doc() = "Holds a callback function without return value.";

    m.def("Procedure", [](const py::object& object) { return PyProcedure_Create<PyConverter>(object); });

    cls.def("__call__",
        [](daq::IProcedure* procedure, const py::args& args)
        {
            const auto procObjPtr = daq::ProcedurePtr::Borrow(procedure);
            if (args.empty())
                procObjPtr.dispatch();
            else if (args.size() == 1)
            {
                const py::object param = args[0];
                procObjPtr.dispatch(pyObjectToBaseObject(param));
            }
            else
            {
                auto daqList = daq::List<daq::IBaseObject>();
                for (size_t i = 0; i < args.size(); i++)
                {
                    const py::object param = args[i];
                    daqList.pushBack(pyObjectToBaseObject(param));
                }
                procObjPtr.dispatch(daqList);
            }
        });
}
