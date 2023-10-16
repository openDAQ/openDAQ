#include "py_core_types/py_function.h"
#include "py_core_types/py_core_types.h"

PyDaqIntf<daq::IFunction> declareIFunction(pybind11::module_ m)
{
    return wrapInterface<daq::IFunction>(m, "IFunction");
}

void defineIFunction(pybind11::module_ m, PyDaqIntf<daq::IFunction> cls)
{
    cls.doc() = "Holds a callback function with return value.";

    m.def("Function", [](const py::object& object) { return PyFunction_Create<PyConverter>(object); });

    cls.def("__call__",
        [](daq::IFunction* function, const py::args& args)
        {
            const auto funcObjPtr = daq::FunctionPtr::Borrow(function);
            daq::BaseObjectPtr result;
            if (args.empty())
                daq::checkErrorInfo(funcObjPtr->call(nullptr, &result));
            else if (args.size() == 1)
            {
                const py::object param = args[0];
                daq::checkErrorInfo(funcObjPtr->call(pyObjectToBaseObject(param), &result));
            }
            else
            {
                auto daqList = daq::List<daq::IBaseObject>();
                for (size_t i = 0; i < args.size(); i++)
                {
                    const py::object param = args[i];
                    daqList.pushBack(pyObjectToBaseObject(param));
                }
                daq::checkErrorInfo(funcObjPtr->call(daqList, &result));
            }
                
            return PyConverter::ToPyObject(result);
        });
}
