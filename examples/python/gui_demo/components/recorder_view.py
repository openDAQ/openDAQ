import tkinter as tk
from tkinter import ttk

import opendaq as daq

from ..app_context import AppContext
from .metadata_fields_selector_dialog import MetadataFieldsSelectorDialog

class RecorderView(ttk.Frame):
    def __init__(self, parent: ttk.Frame, node=None, context: AppContext = None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.context = context
        self.configure(padding=(10, 5))

        self.node = daq.IRecorder.cast_from(node)

        header_frame = ttk.Frame(self)

        ttk.Label(header_frame, text='Recording').pack(side=tk.LEFT, padx=5, pady=5)
        self.pb = ttk.Progressbar(header_frame, mode='indeterminate', style="Striped.Horizontal.TProgressbar",maximum=10, value=0)
        self.pb.pack(side=tk.LEFT, anchor=tk.CENTER)
        ttk.Button(header_frame, text='Start/Stop',
                  command=self.toggleRecording
                  ).pack(side=tk.RIGHT, anchor=tk.E)


        header_frame.pack(fill=tk.X)

    def toggleRecording(self):
<<<<<<< HEAD
        if self.node.is_recording:
            self.node.stop_recording()
        else:
            self.node.start_recording()

        if self.node.is_recording:
            self.pb.start()
        else:
            self.pb.stop()
=======
        if self.node.is_recording():
            self.node.stop()
        else:
            self.node.start()

        if self.node.is_recording():
            self.pb.start()
        else:
            self.pb.stop()
>>>>>>> 41414ca09fc3c273b4e20530137d74c6904756c2
