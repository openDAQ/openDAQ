from .opendaq import *
import os

OPENDAQ_MODULES_DIR = os.path.dirname(os.path.abspath(__file__))
OPENDAQ_CWD = os.getcwd()

# BUG: in mac os creating the project with `-> IInstance:` is failing`. 
# To build successfully, we need to remove the `-> IInstance:` from the code.

def Instance(*args) -> IInstance:
    if len(args) == 0:
        builder = opendaq.InstanceBuilder()
        builder.module_path = OPENDAQ_MODULES_DIR
        return opendaq.InstanceFromBuilder(builder)
    else:
        return opendaq.Instance(*args)

def InstanceBuilder() -> IInstanceBuilder:
    builder = opendaq.InstanceBuilder()
    builder.module_path = OPENDAQ_MODULES_DIR
    return builder