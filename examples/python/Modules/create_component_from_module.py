##
# Advanced example that showcases how components are created using modules. These methods are usually not called
# by end-users, as they instead use the requisite device methods (eg. `add_function_block` or `add_device`) that
# in turn call the correct module methods. How said module methods are used is shown in this example.
##

import opendaq as daq
import sys
sys.path.append("..")
import daq_utils

if __name__ == "__main__":
    instance = daq.Instance()

    module_manager = instance.module_manager
    manager = instance.module_manager
    modules = manager.modules

    # Find trigger function block type
    trigger_fb_type = None
    fb_module = None
    for module in modules:
        if module.module_info.id == 'ReferenceFunctionBlockModule':
            type_obj = module.available_function_block_types['RefFBModuleTrigger']
            # Need to cast to appropriate type; Python function object type is IStruct
            trigger_fb_type = daq.IComponentType.cast_from(type_obj)
            fb_module = module
            break

    # Each type has a default configuration that can be passed to the module manager
    # on object creation
    config = trigger_fb_type.create_default_config()
    print('Trigger config properties:')
    daq_utils.print_property_object(config, 1)

    # We set the "UseMultiThreadedScheduler" config parameter to 'False'
    config.set_property_value('UseMultiThreadedScheduler', False)

    # When creating component directly through the module object, its parent folder must be
    # specified and the component must be added as said parent's child. When creating a module
    # object through the instance, the wrapper code automatically does the above.

    # We find the "FB" child folder of our root device (instance) and specify it as the parent
    fb_folder = instance.get_item('FB')
    fb = fb_module.create_function_block(trigger_fb_type.id, fb_folder, 'trigger', config)

    # The function block must be added as the child of our folder
    fb_folder_config = daq.IFolderConfig.cast_from(fb_folder)
    fb_folder_config.add_item(fb)

    for item in fb_folder_config.items:
        print(item.local_id)