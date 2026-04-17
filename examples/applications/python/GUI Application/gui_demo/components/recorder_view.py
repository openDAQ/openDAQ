import tkinter as tk
from tkinter import ttk

import opendaq as daq
from .. import utils

from ..app_context import AppContext
from .metadata_fields_selector_dialog import MetadataFieldsSelectorDialog

class RecorderView(ttk.Frame):
    def __init__(self, parent: ttk.Frame, node=None, context: AppContext = None, **kwargs):
        ttk.Frame.__init__(self, parent, **kwargs)
        self.context = context

        self.node = daq.IRecorder.cast_from(node)

        header_frame = ttk.Frame(self)

        utils.make_banner(header_frame, 'Recording')
        self.pb = ttk.Progressbar(header_frame, mode='indeterminate', style="Striped.Horizontal.TProgressbar",maximum=10, value=0)
        self.pb.pack(side=tk.LEFT, anchor=tk.CENTER, padx=(10,0), pady=(10))
        
        self.startstop = ttk.Button(header_frame, text='Start/Stop',command=self.toggleRecording)
        self.startstop.pack(side=tk.RIGHT, anchor=tk.E, padx=(0,10),pady=(10))


        header_frame.pack(fill=tk.X)
        if self.node.is_recording:
            self.pb.start()
            self.startstop.configure(text='Stop')
        else:
            self.pb.stop()
            self.startstop.configure(text='Start')
    
    def toggleRecording(self):
        if self.node.is_recording:
            self.node.stop_recording()
        else:
            self.node.start_recording()

        if self.node.is_recording:
            self.pb.start()
            self.startstop.configure(text='Stop')
        else:
            self.pb.stop()
            self.startstop.configure(text='Start')
