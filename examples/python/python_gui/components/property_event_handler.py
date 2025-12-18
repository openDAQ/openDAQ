"""Handler for property change events"""
import opendaq


class PropertyEventHandler:
    """Manages property value change events from DAQ core events"""

    def __init__(self, context, property_object, property_path=""):
        self.context = context
        self.property_object = property_object
        self.property_path = property_path
        self._nested_property_objects = {}  # path -> property_object
        self._update_callbacks = []  # List of (prop_path, callback) tuples
        self._core_event_handler = None

        # Track root property object
        self._nested_property_objects[property_path] = property_object

        # Subscribe to events
        self._setup_listeners()

    def add_nested_property_object(self, path: str, prop_obj):
        """Track a nested property object"""
        self._nested_property_objects[path] = prop_obj

    def register_update_callback(self, prop_path: str, callback):
        """Register callback for property updates: callback(prop_name, new_value)"""
        self._update_callbacks.append((prop_path, callback))

    def unregister_update_callback(self, prop_path: str, callback):
        """Unregister a callback"""
        self._update_callbacks = [
            (path, cb) for path, cb in self._update_callbacks
            if not (path == prop_path and cb == callback)
        ]

    def _setup_listeners(self):
        """Subscribe to core events from DAQ instance context"""
        try:
            daq_context = self.context.daq_instance.context
            self._core_event_handler = opendaq.QueuedEventHandler(self._on_core_event)
            daq_context.on_core_event + self._core_event_handler
        except Exception as e:
            print(f"Failed to subscribe to core events: {e}")

    def _on_core_event(self, sender, args):
        """Handle PropertyValueChanged core events"""
        try:
            if args.event_name != "PropertyValueChanged":
                return

            core_args = opendaq.ICoreEventArgs.cast_from(args)
            owner = core_args.parameters["Owner"]

            # Find which property object this belongs to
            owner_path = None
            for path, prop_obj in self._nested_property_objects.items():
                if owner == prop_obj:
                    owner_path = path
                    break

            if owner_path is None:
                return

            prop_name = core_args.parameters["Name"]
            new_value = core_args.parameters["Value"]
            path = core_args.parameters["Path"]

            # Build full property path
            if path:
                full_prop_path = f"{path}.{prop_name}"
            else:
                full_prop_path = f"{owner_path}.{prop_name}" if owner_path else prop_name

            # Notify all registered callbacks
            for cb_path, callback in self._update_callbacks:
                if cb_path == full_prop_path or cb_path == "":  # "" means all properties
                    try:
                        callback(full_prop_path, new_value)
                    except Exception as e:
                        print(f"Error in update callback: {e}")

        except Exception as e:
            print(f"Error handling core event: {e}")

    def cleanup(self):
        """Clean up event subscriptions"""
        try:
            if self._core_event_handler:
                daq_context = self.context.daq_instance.context
                daq_context.on_core_event - self._core_event_handler
        except Exception as e:
            print(f"Error unsubscribing from core events: {e}")
