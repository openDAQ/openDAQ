class EventPort:
    def __init__(self, parent, event_name='<<App_Refresh>>', event_callback=None):
        self.parent = parent
        self.event_name = event_name
        self.event_callback = event_callback
        if parent and event_callback:
            parent.bind(self.event_name, self.event_callback)

    def emit(self, event_name=None):
        event_name = event_name or self.event_name
        if self.parent and event_name:
            self.parent.event_generate(event_name, when='tail')
