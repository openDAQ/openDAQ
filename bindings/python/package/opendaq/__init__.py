from .opendaq import *
import os

OPENDAQ_MODULES_DIR = os.path.dirname(os.path.abspath(__file__))
OPENDAQ_CWD = os.getcwd()


def Instance(*args) -> IInstance:
    if len(args) == 0:
        builder = opendaq.InstanceBuilder()
        builder.add_module_path(OPENDAQ_MODULES_DIR)
        builder.add_module_path(OPENDAQ_CWD)
        builder.add_logger_sink(opendaq.StdOutLoggerSink())
        builder.scheduler_worker_num = 0
        return opendaq.InstanceFromBuilder(builder)
    else:
        return opendaq.Instance(*args)
