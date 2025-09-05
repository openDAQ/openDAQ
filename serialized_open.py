#!/usr/bin/env python3
#
#

import opendaq as daq
from opendaq.opendaq import Instance

if __name__ == "__main__":
    instance: daq.IInstance = Instance()
    with open('serialized_config.json', 'r', encoding='utf-8') as f:
        config_str = f.read()
        params = daq.UpdateParameters()
        params.set_property_value('ReAddDevices', True)
        instance.load_configuration(config_str, params)

    input('Press Enter to exit...')
