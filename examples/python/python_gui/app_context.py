import utils
import os
import opendaq

class AppContext(object):
    def __init__(self, params):
        self.show_invisible_components = False
        self.show_component_types = None

        self.icons = self.__load_icons(os.path.join(os.path.dirname(__file__), 'icons'))
        self.daq_instance = self.__init_opendaq(params)
        device = self.daq_instance.add_device("daqref://device0")
        device.add_function_block("ref_fb_module_statistics")

    def __load_icons(self, directory):
        images = {}
        for file in utils.get_files_in_directory(directory):
            image = utils.load_and_resize_image(os.path.join(directory, file))
            images[file.split('.')[0]] = image
        return images
    
    def __init_opendaq(self, params):
        return opendaq.Instance()