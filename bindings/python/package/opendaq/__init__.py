from .opendaq import *
import os

OPENDAQ_MODULES_DIR = os.path.dirname(os.path.abspath(__file__))


def Instance(*args) -> IInstance:
    if len(args) == 0:
        module_manager = opendaq.ModuleManager(
            opendaq.String(OPENDAQ_MODULES_DIR))
        type_manager = opendaq.TypeManager()
        sinks = opendaq.List()
        sinks.append(opendaq.StdOutLoggerSink())
        logger = opendaq.Logger(sinks, opendaq.LogLevel.Default)
        scheduler = opendaq.Scheduler(logger, 0)
        context = opendaq.Context(
            scheduler, logger, type_manager, module_manager)
        return opendaq.Instance(context, None)
    else:
        return opendaq.Instance(*args)
