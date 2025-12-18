import utils
import os
import opendaq


class EditorState:
    """State of the active property editor"""
    def __init__(self):
        self.editor = None
        self.item_id = None
        self.view = None

    def set(self, editor, item_id, view):
        self.editor = editor
        self.item_id = item_id
        self.view = view

    def clear(self):
        self.editor = None
        self.item_id = None
        self.view = None

    def is_active(self) -> bool:
        return self.editor is not None


class AppContext(object):
    def __init__(self, params):
        self.__show_invisible_components = False
        self.__show_invisible_components_changed_callbacks = []
        self.show_component_types = None
        self.editor_state = EditorState()

        self.icons = self.__load_icons(os.path.join(os.path.dirname(__file__), 'icons'))
        self.daq_instance = self.__init_opendaq(params)

    def set_show_invisible_components(self, show: bool):
        if self.__show_invisible_components == show:
            return

        self.__show_invisible_components = show
        for callback in self.__show_invisible_components_changed_callbacks:
            callback(show)

    @property
    def show_invisible_components(self) -> bool:
        return self.__show_invisible_components

    def set_show_invisible_components_changed_callback(self, callback):
        self.__show_invisible_components_changed_callbacks.append(callback)

    def remove_show_invisible_components_changed_callback(self, callback):
        if callback in self.__show_invisible_components_changed_callbacks:
            self.__show_invisible_components_changed_callbacks.remove(callback)

    def __load_icons(self, directory):
        images = {}
        for file in utils.get_files_in_directory(directory):
            image = utils.load_and_resize_image(os.path.join(directory, file))
            images[file.split('.')[0]] = image
        return images
    
    def __init_opendaq(self, params):
        builder = opendaq.InstanceBuilder()
        builder.using_scheduler_main_loop = True
        instance = builder.build()
        device = instance.add_device("daqref://device0")
        device.add_function_block("ref_fb_module_statistics")
        device.add_device("daq://SonyUK_Denis")

        # Add test dict property
        try:
            # Create dict property for testing
            test_dict = opendaq.Dict()
            test_dict["key1"] = "value1"
            test_dict["key2"] = "value2"
            test_dict["number"] = "123"

            # Add property to instance (visible=True)
            instance.add_property(opendaq.DictProperty("TestDictionary", test_dict, True))
        except Exception as e:
            print(f"Failed to add test dict property: {e}")

        # Add test list property
        try:
            # Create list property for testing
            test_list = opendaq.List()
            test_list.append("item1")
            test_list.append("item2")
            test_list.append("item3")

            # Add property to instance (visible=True)
            instance.add_property(opendaq.ListProperty("TestList", test_list, True))
        except Exception as e:
            print(f"Failed to add test list property: {e}")

        return instance