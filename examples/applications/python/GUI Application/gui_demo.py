#!/usr/bin/env python

import argparse
import os
import enum
import gc
import sys
import platform

import tkinter as tk
from tkinter import ttk
import tkinter.font as tkfont
from tkinter.filedialog import asksaveasfile
from tkinter.filedialog import askopenfile
from tkinter.filedialog import askopenfilename
import opendaq as daq
from tkinter import messagebox

try:
    from ctypes import windll
    windll.shcore.SetProcessDpiAwareness(1)
except Exception:
    pass

try:
    from gui_demo.components.block_view import BlockView
    from gui_demo.components.properties_view import PropertiesView
    from gui_demo.components.add_device_dialog import AddDeviceDialog
    from gui_demo.components.add_server_dialog import AddServerDialog
    from gui_demo.components.add_function_block_dialog import AddFunctionBlockDialog
    from gui_demo.components.load_instance_config_dialog import LoadInstanceConfigDialog
    from gui_demo.components.configure_instance_dialog import ConfigureInstanceDialog
    from gui_demo.components.logs_window import LogsWindow
    from gui_demo.app_context import AppContext
    from gui_demo import utils
    from gui_demo.event_port import EventPort
except Exception as e:
    from opendaq.gui_demo.components.block_view import BlockView
    from opendaq.gui_demo.components.properties_view import PropertiesView
    from opendaq.gui_demo.components.add_device_dialog import AddDeviceDialog
    from opendaq.gui_demo.components.add_server_dialog import AddServerDialog
    from opendaq.gui_demo.components.add_function_block_dialog import AddFunctionBlockDialog
    from opendaq.gui_demo.components.load_instance_config_dialog import LoadInstanceConfigDialog
    from opendaq.gui_demo.components.configure_instance_dialog import ConfigureInstanceDialog
    from opendaq.gui_demo.components.logs_window import LogsWindow
    from opendaq.gui_demo.app_context import AppContext
    from opendaq.gui_demo import utils
    from opendaq.gui_demo.event_port import EventPort


class DisplayType(enum.Enum):
    SYSTEM_OVERVIEW = 0
    SIGNALS = 1
    CHANNELS = 2
    FUNCTION_BLOCKS = 3
    TOPOLOGY = 4
    TOPOLOGY_CUSTOM_COMPONENTS = 5
    MODULES = 6
    UNSPECIFIED = 99

    def from_tab_index(index):
        if index == 0:
            return DisplayType.SYSTEM_OVERVIEW
        elif index == 1:
            return DisplayType.SIGNALS
        elif index == 2:
            return DisplayType.CHANNELS
        elif index == 3:
            return DisplayType.FUNCTION_BLOCKS
        elif index == 4:
            return DisplayType.TOPOLOGY
        elif index == 5:
            return DisplayType.MODULES
        return DisplayType.UNSPECIFIED

class ContextParams:
    module_path: str = ''
    discovery_servers: list = None

class App(tk.Tk):

    # MARK: -- INIT
    def __init__(self, args):
        super().__init__()

        context_params = ContextParams()

        try:
            if args.module_path != '':
                context_params.module_path = args.module_path
            else:
                context_params.module_path = None
        except ValueError:
            context_params.module_path = None
            
        if args.discovery_server:
            context_params.discovery_servers = [
                s.strip() for s in args.discovery_server.split(',') if s.strip()
            ]
        else:
            context_params.discovery_servers = []

        self.context = AppContext(context_params)
        self.event_port = EventPort(self, event_callback=self.on_refresh_event)

        self.context.ui_scaling_factor = int(args.scale)
        self.context.include_reference_devices = bool(args.demo)
        try:
            if args.connection_string != '':
                self.context.connection_string = args.connection_string
            else:
                self.context.connection_string = None
        except ValueError:
            self.context.connection_string = None

        self.modules_map = {}
        self._nested_fb_indicators = {}
        self._tree_action_buttons = {}
        self._tree_action_pos = {}
        # row colour each floating button currently sits on, and which one is
        # being held; together they decide its hover and pressed shade
        self._tree_action_base_bg = {}
        self._tree_action_pressed = None
        self._shade_cache = {}
        self._tree_hover_row = None
        self._tree_font_cache = None
        # tree search / filter state
        self._tree_all_items = []
        self._nested_fb_types_cache = {}
        # true while the search box is showing its hint as its own text
        self._search_placeholder = False
        self._indicator_click = False
        self._floating_dialogs = {}
        self._logs_window = None

        self.title('openDAQ demo')
        self.geometry('{}x{}'.format(
            int(1500 * self.context.ui_scaling_factor * self.context.dpi_factor),
            int(800 * self.context.ui_scaling_factor * self.context.dpi_factor)))

        # icons load first: menus, the refresh button and the tree action
        # buttons all reference them
        self.context.load_icons(os.path.join(
            os.path.dirname(__file__), 'gui_demo', 'icons'))

        self.menu_bar_create()

        main_frame_bottom = ttk.Frame(self)
        main_frame_bottom.pack(fill=tk.BOTH, expand=True)

        # the tab strip and the refresh button share a row, so refresh sits at
        # the right of the tabs rather than looking like part of the search box
        tab_row = ttk.Frame(main_frame_bottom)
        tab_row.pack(fill=tk.X)

        nb = ttk.Notebook(tab_row)
        nb.add(ttk.Frame(nb), text='System Overview')
        nb.add(ttk.Frame(nb), text='Signals')
        nb.add(ttk.Frame(nb), text='Channels')
        nb.add(ttk.Frame(nb), text='Function blocks')
        nb.add(ttk.Frame(nb), text='Full Topology')
        nb.add(ttk.Frame(nb), text='Modules')
        nb.bind('<<NotebookTabChanged>>', self.on_tab_change)
        nb.pack(side=tk.LEFT, fill=tk.X, expand=True)
        self.nb = nb

        self._tab_refresh_button = self._flat_icon_button(
            tab_row, self.handle_refresh_button_clicked,
            image=self.context.icons.get('refresh'),
            text=None if self.context.icons.get('refresh') else '↻')
        self._tab_refresh_button.pack(side=tk.RIGHT, padx=(6, 6))
        utils.attach_tooltip(
            self._tab_refresh_button,
            'Rebuild the tree from the instance\n'
            '(it is not the search box\'s reset - that is the ✕)')

        # packed after refresh, so side=RIGHT puts it to refresh's left
        self._tab_logs_button = self._flat_icon_button(
            tab_row, self.handle_logs_button_clicked,
            image=self.context.icons.get('logs'),
            text=None if self.context.icons.get('logs') else 'Logs')
        self._tab_logs_button.pack(side=tk.RIGHT)
        utils.attach_tooltip(self._tab_logs_button,
                             'Show the instance log')

        main_frame_navigator = ttk.PanedWindow(
            main_frame_bottom, orient=tk.HORIZONTAL)
        main_frame_navigator.pack_propagate(0)

        frame_navigator_for_properties = ttk.Frame(
            main_frame_navigator)

        self.tree_widget_create(main_frame_navigator)

        main_frame_navigator.add(frame_navigator_for_properties)

        main_frame_navigator.pack(side=tk.LEFT, expand=1, fill=tk.BOTH)

        self.frame_navigator_for_properties = frame_navigator_for_properties

        self.right_side_panel_create(frame_navigator_for_properties)

        # High DPI workaround for now
        style = ttk.Style()
        treeview_rowheight = max(20, int(round(
            30 * self.context.ui_scaling_factor * self.context.dpi_factor)))
        style.configure('Treeview', rowheight=treeview_rowheight)

        style.configure('Treeview.Heading', font='Arial 10 bold')
        style.configure('Treeview.Column', padding=(
            5 * self.context.ui_scaling_factor))

        # Style for status text labels
        style.configure("StatusOk.TLabel",
                foreground="green",
                font=("TkDefaultFont", 10, "bold"))
        style.configure("StatusError.TLabel",
                    foreground="red",
                    font=("TkDefaultFont", 10, "bold"))
        style.configure("StatusWarning.TLabel",
                    foreground="goldenrod",
                    font=("TkDefaultFont", 10, "bold"))

        default_font = tkfont.nametofont('TkDefaultFont')
        default_font.configure(size=9 * self.context.ui_scaling_factor)

        # hover look for nested FB indicators: strong contrast marks the row
        # as clickable (a single click adds the function block)
        self.tree.tag_configure('nested_fb_hover',
                                foreground='#1f1f1f', background='#e4e4e4')

        self.instance_create()
        self.init_opendaq()

        if args.config != '':
            self._load_config(args.config)

        self.poll_opendaq_events()

    # MARK: - Instance lifecycle
    def instance_create(self):
        try:
            self.context.create_instance()
        except Exception as e:
            print('Instance creation failed:', e, file=sys.stderr)
            utils.show_error('Instance creation failed',
                             f'{str(e)}\nStarting with default settings instead.', self)
            self.context.module_path = None
            self.context.create_instance()

    def handle_reconfigure_instance_clicked(self):
        dialog = ConfigureInstanceDialog(self, self.context)
        dialog.show()
        if dialog.confirmed:
            self.instance_recreate()

    # recreates the instance with the current context settings; the running
    # setup (devices, function blocks, servers) is saved first and loaded
    # into the new instance
    def instance_recreate(self):
        config_string = None
        try:
            config_string = self.context.instance.save_configuration()
        except Exception as e:
            print('Saving configuration failed:', e, file=sys.stderr)

        # drop everything that references the old instance
        for dialog in self._floating_dialogs.values():
            if dialog.winfo_exists():
                dialog.destroy()
        self._floating_dialogs = {}
        self.tree.delete(*self.tree.get_children())
        self.right_side_panel_clear()
        self._tree_hover_set(None)
        self.context.selected_node = None
        self.context.nodes = {}
        self.context.signals = {}
        self.context.custom_component_ids = set()
        # per-component tree state learned about the old instance
        self.context.forget_nested_fb_refusals()
        self.context.nested_fb_hidden = set()
        self.context.instance = None
        gc.collect()  # release the old instance before creating the new one

        # the old sink keeps its log file open; the new instance gets a
        # fresh one and the logs window follows it automatically
        self.context.next_log_file()
        self.instance_create()

        if config_string is not None:
            try:
                self.context.instance.load_configuration(
                    config_string, daq.UpdateParameters())
            except Exception as e:
                print('Restoring configuration failed:', e, file=sys.stderr)
                utils.show_error(
                    'Reconfigure instance',
                    f'New instance created, but restoring devices and '
                    f'function blocks failed: {str(e)}', self)

        self.tree_update()

    def poll_opendaq_events(self):
        if self.context.instance is None:
            self.after(50, self.poll_opendaq_events)
            return

        try:
            daq.event_queue.process_events()
        except Exception as e:
            print("Callback processing error:", e)

        if self.context.needs_refresh:
            self.on_refresh_event(None)
            self.context.needs_refresh = False

        try:
            self.context.instance.context.scheduler.run_main_loop_iteration()
        except Exception as e:
            print("Scheduler processing error:", e)

        # safety net keeping the floating row-action buttons glued to their rows
        self._tree_overlays_update()

        self.after(50, self.poll_opendaq_events)

    def init_opendaq(self):

        # add the first device if connection string is provided once on start
        if self.context.connection_string is not None:
            # adds the device only; the tree is built once below, so a caller
            # cannot skip that on the assumption this refreshed anything
            self.context.add_first_available_device()

        self.tree_update()

    # MARK: - Menu bar
    def menu_bar_create(self):
        menu_bar = tk.Menu(self)
        self.config(menu=menu_bar)

        icons = self.context.icons

        file_menu = tk.Menu(menu_bar, tearoff=0)
        menu_bar.add_cascade(label='File', menu=file_menu)
        file_menu.add_command(label='Load configuration',
                              image=icons['load_config'], compound=tk.LEFT,
                              command=self.handle_load_config_button_clicked)
        file_menu.add_command(label='Save configuration',
                              image=icons['save_config'], compound=tk.LEFT,
                              command=self.handle_save_config_button_clicked)
        file_menu.add_command(label='Load module',
                              image=icons['load_module'], compound=tk.LEFT,
                              command=self.handle_load_modules_button_clicked)
        file_menu.add_command(label='Reconfigure instance…',
                              image=icons['settings'], compound=tk.LEFT,
                              command=self.handle_reconfigure_instance_clicked)
        file_menu.add_separator()
        file_menu.add_command(label='Exit', image=icons['exit_app'],
                              compound=tk.LEFT, command=self.quit)

        view_menu = tk.Menu(menu_bar, tearoff=0)
        menu_bar.add_cascade(label='View', menu=view_menu)
        view_menu.add_checkbutton(label='Show hidden components',command=self.handle_view_show_hidden_components)

        self._signal_preview_var = tk.BooleanVar(value=self.context.view_signal_preview)
        view_menu.add_checkbutton(label='Signal preview',variable=self._signal_preview_var,command=self.handle_view_signal_preview_toggled)

        self._nested_fb_var = tk.BooleanVar(value=self.context.view_nested_fb)
        view_menu.add_checkbutton(
            label='Nested function block buttons',
            variable=self._nested_fb_var,
            command=self.handle_view_nested_fb_toggled)

        view_menu.add_separator()
        view_menu.add_command(label='Show logs', image=icons['logs'],
                              compound=tk.LEFT,
                              command=self.handle_logs_button_clicked)

    def handle_view_show_hidden_components(self):
        self.context.view_hidden_components = not self.context.view_hidden_components
        self.tree_update()

    def handle_view_signal_preview_toggled(self):
        self.context.view_signal_preview = self._signal_preview_var.get()
        if self.context.selected_node is not None:
            self.right_side_panel_clear()
            self.right_side_panel_draw_node(self.context.selected_node)

    # Turning the placeholders back on should show them everywhere, so the
    # per-row opt-outs collected while they were on do not survive the switch.
    def handle_view_nested_fb_toggled(self):
        self.context.view_nested_fb = self._nested_fb_var.get()
        if self.context.view_nested_fb:
            self.context.nested_fb_hidden = set()
        self.tree_update(self.context.selected_node)
            
    TREE_ICON_WIDTH = 24
    TREE_TEXT_PADDING = 28

    def _tree_indent(self):
        try:
            return int(str(self.tk.call(
                'ttk::style', 'lookup', 'Treeview', '-indent')))
        except Exception:
            return 20

    # Cached: the overlay placement measures text on every mouse motion.
    def _tree_font(self):
        if self._tree_font_cache is None:
            self._tree_font_cache = tkfont.Font(
                font=ttk.Style().lookup('Treeview', 'font') or 'TkDefaultFont')
        return self._tree_font_cache

    # Width the deepest visible row needs. Tk never sizes the tree column to its
    # contents, so without this a nested row is clipped instead of scrollable.
    def _tree_content_width(self):
        font = self._tree_font()
        indent = self._tree_indent()
        widest = 0

        def walk(parent, depth):
            nonlocal widest
            for iid in self.tree.get_children(parent):
                needed = (depth * indent + self.TREE_ICON_WIDTH
                          + font.measure(self.tree.item(iid, 'text')))
                widest = max(widest, needed)
                if self.tree.item(iid, 'open'):
                    walk(iid, depth + 1)

        walk('', 1)
        return widest + self.TREE_TEXT_PADDING

    def _tree_autosize_column(self):
        visible = self.tree.winfo_width()
        if visible <= 1:
            return
        # never narrower than the pane, so a shallow tree shows no scrollbar
        wanted = max(self._tree_content_width(), visible - 4)
        if abs(wanted - self.tree.column('#0', 'width')) > 2:
            self.tree.column('#0', width=wanted)

    # Flat, borderless icon button matching the tree action buttons. The
    # background tracks its row so transparent icon corners blend in.
    def _flat_icon_button(self, parent, handler, image=None, text=None, bg=None):
        if bg is None:
            bg = ttk.Style().lookup('TFrame', 'background') or \
                self.cget('background')
        kwargs = {'bd': 0, 'bg': bg, 'cursor': 'hand2'}
        if image is not None:
            kwargs['image'] = image
        if text is not None:
            kwargs['text'] = text
            kwargs['font'] = ('TkDefaultFont', 11)
            kwargs['fg'] = '#555555'
            kwargs['padx'] = 3
        button = tk.Label(parent, **kwargs)
        button.bind('<Button-1>', lambda e: handler())
        button.bind('<Enter>', lambda e: button.configure(bg='#e4e4e4'))
        button.bind('<Leave>', lambda e: button.configure(bg=bg))
        return button

    # MARK: - Tree view
    def tree_widget_create(self, parent_frame):
        frame = ttk.Frame(parent_frame)

        # shares this frame with the tree, so whatever is packed here sets the
        # pane's minimum width
        self._tree_search_row_create(frame)

        tree = ttk.Treeview(frame, columns=('name', 'hash'), displaycolumns=(
            'name'), show='tree', selectmode=tk.BROWSE)

        # #0 must not stretch: a stretched column is always exactly the widget
        # width, so there would be nothing for xview to scroll over and deeply
        # indented rows would simply be clipped.
        tree.column('#0', stretch=False,
                    width=int(350 * self.context.ui_scaling_factor * self.context.dpi_factor))
        # 'hash' carries the global id for lookups and is never displayed
        tree.column('#1', width=0, minwidth=0, stretch=False)

        tree.bind('<<TreeviewSelect>>', self.handle_tree_select)
        tree.bind('<ButtonRelease-3>', self.handle_tree_right_button_release)
        tree.bind('<Button-3>', self.handle_tree_right_button)
        tree.bind('<Button-1>', self.handle_tree_click)
        tree.bind('<Double-1>', self._block_indicator_double_click)

        scroll_bar = ttk.Scrollbar(
            frame, orient=tk.VERTICAL, command=tree.yview)

        def tree_yscroll(first, last):
            scroll_bar.set(first, last)
            self._tree_overlays_update()  # immediate sync while scrolling
        tree.configure(yscroll=tree_yscroll)

        scroll_bar_x = ttk.Scrollbar(
            frame, orient=tk.HORIZONTAL, command=tree.xview)

        def tree_xscroll(first, last):
            scroll_bar_x.set(first, last)
            self._tree_overlays_update()
        tree.configure(xscroll=tree_xscroll)
        self._tree_scroll_x = scroll_bar_x

        # Packed before the tree: the tree claims a side=LEFT slab, so anything
        # packed after it lands in the strip beside it rather than under it.
        scroll_bar_x.pack(fill=tk.X, side=tk.BOTTOM)
        scroll_bar.pack(fill=tk.Y, side=tk.RIGHT)
        tree.pack(fill=tk.BOTH, expand=True, side=tk.LEFT)

        parent_frame.add(frame)
        tree.tag_configure('warning', foreground=utils.StatusColor.WARNING)
        tree.tag_configure('error', foreground=utils.StatusColor.ERROR)
        tree.tag_configure('inactive', foreground='gray')
        tree.tag_configure('nested_fb', foreground='gray')
        tree.bind('<Configure>', lambda e: self._tree_overlays_update(), add='+')
        # expanding or collapsing changes which rows are visible, so the width
        # the column needs changes with it
        tree.bind('<Configure>', lambda e: self._tree_autosize_column(), add='+')
        tree.bind('<<TreeviewOpen>>',
                  lambda e: self.after_idle(self._tree_autosize_column), add='+')
        tree.bind('<<TreeviewClose>>',
                  lambda e: self.after_idle(self._tree_autosize_column), add='+')
        # the filter restores the snapshot's expansion state when it clears, so
        # the snapshot has to follow what the user does to the unfiltered tree
        tree.bind('<<TreeviewOpen>>',
                  lambda e: self.after_idle(self._tree_resnapshot_open_state),
                  add='+')
        tree.bind('<<TreeviewClose>>',
                  lambda e: self.after_idle(self._tree_resnapshot_open_state),
                  add='+')
        tree.bind('<Motion>', self._handle_tree_motion)
        tree.bind('<Leave>', self._handle_tree_leave)
        style = ttk.Style()
        self._tree_field_bg = style.lookup(
            'Treeview', 'fieldbackground') or 'white'
        # selection color, used to blend the floating buttons into a
        # selected row (child widgets cannot be transparent in tk)
        self._tree_selected_bg = '#0078d7'
        for state_spec, color in style.map('Treeview', 'background'):
            if 'selected' in state_spec:
                self._tree_selected_bg = color
                break
        self.tree = tree
        self.tree_action_buttons_create()

    # background the given row is currently painted with
    def _row_background(self, iid):
        if iid and self.tree.exists(iid):
            if iid in self.tree.selection():
                return self._tree_selected_bg
            if 'nested_fb_hover' in self.tree.item(iid, 'tags'):
                return '#e4e4e4'
        return self._tree_field_bg

    # How far the button background is pushed towards black when the pointer is
    # over it and while it is held. tk has no alpha on a widget background, so
    # this is a blend rather than an overlay - which is what keeps it working on
    # a selected row, where the row underneath is the selection colour.
    ACTION_BUTTON_HOVER_SHADE = 0.12
    ACTION_BUTTON_PRESSED_SHADE = 0.26

    # blend a colour towards black. winfo_rgb resolves names and system colours
    # too, so this works whatever the theme hands back for the row background
    def _shade(self, color, amount):
        key = (color, amount)
        cached = self._shade_cache.get(key)
        if cached is not None:
            return cached
        try:
            r, g, b = (c // 257 for c in self.winfo_rgb(color))
        except tk.TclError:
            return color
        f = 1.0 - amount
        shaded = '#{:02x}{:02x}{:02x}'.format(
            int(r * f), int(g * f), int(b * f))
        self._shade_cache[key] = shaded
        return shaded

    def _widget_has_pointer(self, widget):
        try:
            if not widget.winfo_ismapped():
                return False
            px, py = widget.winfo_pointerxy()
            x, y = widget.winfo_rootx(), widget.winfo_rooty()
            return (x <= px < x + widget.winfo_width()
                    and y <= py < y + widget.winfo_height())
        except Exception:
            return False

    # Repaints the placed buttons for their current state. Driven by the pointer
    # position rather than by the last <Enter> seen: these buttons are moved
    # under and away from a resting pointer as the hovered row changes, so Tk's
    # enter and leave events do not describe where the pointer actually is.
    def _tree_action_buttons_restyle(self):
        for key, btn in self._tree_action_buttons.items():
            base = self._tree_action_base_bg.get(key)
            if base is None or self._tree_action_pos.get(key) is None:
                continue
            if self._tree_action_pressed == key and self._widget_has_pointer(btn):
                bg = self._shade(base, self.ACTION_BUTTON_PRESSED_SHADE)
            elif self._widget_has_pointer(btn):
                bg = self._shade(base, self.ACTION_BUTTON_HOVER_SHADE)
            else:
                bg = base
            if str(btn.cget('bg')) != bg:
                btn.configure(bg=bg)

    def _handle_action_button_press(self, key):
        self._tree_action_pressed = key
        self._tree_action_buttons_restyle()

    def _handle_action_button_release(self, key):
        self._tree_action_pressed = None
        self._tree_action_buttons_restyle()

    # floating per-row action buttons: add pinned to the instance row, the
    # rest shown on the hovered row
    def tree_action_buttons_create(self):
        def make_button(key, icon_key, handler, tooltip=None):
            btn = tk.Label(self.tree, image=self.context.icons[icon_key],
                           bg=self._tree_field_bg, bd=0)
            # Tooltip first: Tk runs same-sequence bindings in the order they
            # were added, and its handlers have to go before these two. The tip
            # sits just below the button, over the rows the pointer crosses on
            # its way out, so it must be gone before _handle_action_button_leave
            # asks what is under the pointer - and gone before tk_popup below
            # blocks in the menu loop with a tip still scheduled.
            if tooltip is not None:
                # icon-only buttons, two of which swap image per row, so the text
                # is resolved when the tip is shown, not when it is attached
                utils.attach_tooltip(btn, tooltip)
            # Press styling before the handler: an add button opens a menu whose
            # grab blocks here, so the held look has to be on screen by then. The
            # release lands after the menu closes, which is when it should clear.
            btn.bind('<Button-1>',
                     lambda e, k=key: self._handle_action_button_press(k),
                     add='+')
            btn.bind('<Button-1>', handler, add='+')
            btn.bind('<ButtonRelease-1>',
                     lambda e, k=key: self._handle_action_button_release(k),
                     add='+')
            btn.bind('<Enter>',
                     lambda e: self._tree_action_buttons_restyle(), add='+')
            btn.bind('<Leave>', self._handle_action_button_leave, add='+')
            btn.bind('<Leave>',
                     lambda e: self._tree_action_buttons_restyle(), add='+')
            return btn

        # logs lives in the tab row, not on a tree row: it is not about any one
        # component
        self._tree_action_buttons = {
            # no tooltip on either '+': the menu it opens names its own options,
            # so the tip only got in the way of the thing it was describing
            'add': make_button('add', 'plus', self.handle_tree_add_clicked),
            'remove': make_button('remove', 'trash',
                                  self.handle_tree_remove_clicked,
                                  self._remove_tooltip),
            'hover_add': make_button('hover_add', 'plus',
                                     self.handle_tree_hover_add_clicked),
            # 'lock' and 'nested_fb' swap image per row, and both show the
            # action rather than the current state: a lock-shaped icon sitting
            # on a locked row reads as a status badge, and gets clicked by
            # someone trying to unlock. The tooltip names the same action.
            'lock': make_button('lock', 'lock', self.handle_tree_lock_clicked,
                                self._lock_tooltip),
            'nested_fb': make_button('nested_fb', 'add_fb',
                                     self.handle_tree_nested_fb_clicked,
                                     self._nested_fb_tooltip),
        }

    # ---- tooltips for the hovered row's buttons: what they do depends on the
    # row they are floating over, so each is resolved on demand. Returning None
    # when nothing is hovered any more suppresses the tip. ----
    def _remove_tooltip(self):
        component = self.context.nodes.get(self._tree_hover_row)
        if component is None:
            return None
        return 'Remove this device' if daq.IDevice.can_cast_from(component) \
            else 'Remove this function block'

    def _hover_add_tooltip(self):
        component = self.context.nodes.get(self._tree_hover_row)
        if component is None:
            return None
        return 'Add a device or function block to this device' \
            if daq.IDevice.can_cast_from(component) \
            else 'Add a nested function block'

    def _lock_tooltip(self):
        if self._tree_hover_row is None:
            return None
        return 'Unlock this device' if self._device_locked(
            self._tree_hover_row) else 'Lock this device'

    def _nested_fb_tooltip(self):
        if self._tree_hover_row is None:
            return None
        return 'Show the nested function blocks this can take' \
            if self._tree_hover_row in self.context.nested_fb_hidden \
            else 'Hide the nested function blocks this can take'

    def handle_tree_add_clicked(self, event):
        icons = self.context.icons
        menu = tk.Menu(self.tree, tearoff=0)
        menu.add_command(label='Add device', image=icons['device'],
                         compound=tk.LEFT,
                         command=self.handle_add_device_button_clicked)
        menu.add_command(label='Add function block', image=icons['function_block'],
                         compound=tk.LEFT,
                         command=self.handle_add_function_block_button_clicked)
        menu.add_command(label='Add server', image=icons['server'],
                         compound=tk.LEFT,
                         command=self.handle_add_server_button_clicked)
        try:
            menu.tk_popup(event.widget.winfo_rootx(),
                          event.widget.winfo_rooty() + event.widget.winfo_height())
        finally:
            menu.grab_release()

    def handle_tree_remove_clicked(self, event):
        iid = self._tree_hover_row
        component = self.context.nodes.get(iid) if iid else None
        if component is None:
            return
        if daq.IFunctionBlock.can_cast_from(
                component) and not daq.IChannel.can_cast_from(component):
            self.handle_tree_menu_remove_function_block(
                daq.IFunctionBlock.cast_from(component))
        elif daq.IDevice.can_cast_from(component):
            self.handle_tree_menu_remove_device(
                daq.IDevice.cast_from(component))

    # lock button on a hovered device row: toggles that device's lock
    def handle_tree_lock_clicked(self, event):
        iid = self._tree_hover_row
        if not self._device_lockable(iid):
            return
        if self._device_locked(iid):
            self.unlock_device_node(iid)
        else:
            self.lock_device_node(iid)

    # a row only offers the hide/show toggle when it has placeholders to hide.
    # A block that already refused every type it offers has none, so it gets no
    # button - otherwise the toggle would flip with nothing to show for it.
    def _nested_fb_togglable(self, iid):
        if not self.context.view_nested_fb:
            return False
        fb_types = self._nested_fb_types(iid)
        if not fb_types:
            return False
        return any(self._nested_fb_offered(iid, fb_type_id)
                   for fb_type_id in fb_types.keys())

    # Whether this row still has somewhere to put another block of this type:
    # it has not been refused one, and it is not already at the declared limit.
    def _nested_fb_offered(self, iid, fb_type_id):
        fb_type_id = str(fb_type_id)
        if (iid, fb_type_id) in self.context.nested_fb_full:
            return False
        component = self.context.nodes.get(iid)
        if component is None:
            return True
        return not self.context.nested_fb_at_limit(component, fb_type_id)

    # hide or bring back this row's nested-function-block placeholders. Scoped
    # to the one row: a block is finished being configured on its own schedule.
    def handle_tree_nested_fb_clicked(self, event):
        iid = self._tree_hover_row
        if not self._nested_fb_togglable(iid):
            return
        hidden = self.context.nested_fb_hidden
        if iid in hidden:
            hidden.discard(iid)
        else:
            hidden.add(iid)
        self.tree_update(self.context.selected_node)

    # plus button on a hovered row: the same offers its right-click menu makes,
    # so a block that can take a nested function block gets the button too
    def handle_tree_hover_add_clicked(self, event):
        iid = self._tree_hover_row
        component = self.context.nodes.get(iid) if iid else None
        if component is None:
            return

        icons = self.context.icons
        menu = tk.Menu(self.tree, tearoff=0)

        if daq.IDevice.can_cast_from(component):
            device = daq.IDevice.cast_from(component)
            menu.add_command(label='Add device', image=icons['device'],
                             compound=tk.LEFT,
                             command=lambda: self.add_device_dialog_show(device))
            target = device
        else:
            target = component

        if self._nested_fb_types(iid) is not None or \
                self._device_fb_types(iid) is not None:
            menu.add_command(
                label='Add function block', image=icons['function_block'],
                compound=tk.LEFT,
                command=lambda: self.add_function_block_dialog_show(target))

        # index(END) is None only for a menu with no entries, and only because
        # tearoff=0 above: a tearoff entry would make it 0 and this guard would
        # stop firing, posting an empty menu on every row that offers nothing.
        if menu.index(tk.END) is None:
            return
        try:
            menu.tk_popup(event.widget.winfo_rootx(),
                          event.widget.winfo_rooty() + event.widget.winfo_height())
        finally:
            menu.grab_release()

    # the function block types a device row offers, or None
    def _device_fb_types(self, iid):
        component = self.context.nodes.get(iid)
        if component is None or not daq.IDevice.can_cast_from(component):
            return None
        try:
            fb_types = daq.IDevice.cast_from(
                component).available_function_block_types
        except Exception:
            return None
        return fb_types if fb_types else None

    # True when the hovered row has anything to add: a device always does, a
    # function block or channel only when it accepts nested blocks
    def _row_addable(self, iid):
        component = self.context.nodes.get(iid)
        if component is None:
            return False
        if daq.IDevice.can_cast_from(component):
            return True
        return self._nested_fb_types(iid) is not None

    # the client instance itself is excluded: it is the local root, not
    # something a lock protects against another client
    def _device_lockable(self, iid):
        component = self.context.nodes.get(iid) if iid else None
        if component is None or not daq.IDevice.can_cast_from(component):
            return False
        if self.context.instance is not None and \
                component.global_id == self.context.instance.global_id:
            return False
        return True

    def _device_locked(self, iid):
        component = self.context.nodes.get(iid) if iid else None
        if component is None or not daq.IDevice.can_cast_from(component):
            return False
        try:
            return bool(daq.IDevice.cast_from(component).locked)
        except Exception:
            return False

    def _component_removable(self, iid):
        component = self.context.nodes.get(iid)
        if component is None:
            return False
        if self.context.instance is not None and \
                component.global_id == self.context.instance.global_id:
            return False
        if daq.IChannel.can_cast_from(component):
            return False
        return daq.IFunctionBlock.can_cast_from(
            component) or daq.IDevice.can_cast_from(component)

    # MARK: - Tree search / filter
    def _tree_search_row_create(self, parent):
        self._search_var = tk.StringVar()

        row = ttk.Frame(parent)
        row.pack(side=tk.TOP, fill=tk.X, padx=(0, 2), pady=2)

        # flat, borderless icon buttons matching the tree action buttons; the
        # background tracks the row so the transparent icon corners blend in
        row_bg = ttk.Style().lookup('TFrame', 'background') or \
            self.cget('background')
        self._search_row_bg = row_bg

        def flat_button(handler, image=None, text=None):
            return self._flat_icon_button(row, handler, image=image, text=text,
                                          bg=row_bg)

        entry = ttk.Entry(row, textvariable=self._search_var, width=53)
        entry.pack(side=tk.LEFT)
        self._search_entry = entry
        self._search_entry_fg = str(entry.cget('foreground')) or ''

        clear_btn = flat_button(self._clear_search, text='✕')
        clear_btn.pack(side=tk.LEFT, padx=(4, 0))

        # placeholder first, then the trace: this row is built before the tree
        # exists, and filtering on the initial write would reach for it
        self._search_placeholder_show()
        self._search_var.trace_add(
            'write', lambda *a: self._on_search_changed())
        entry.bind('<Escape>', lambda e: self._clear_search())
        entry.bind('<FocusIn>', lambda e: self._search_placeholder_hide())
        entry.bind('<FocusOut>', lambda e: self._search_placeholder_show())

    # ttk has no placeholder, so the box holds the hint as its own text while
    # it is empty and unfocused. Nothing else may read the variable directly:
    # _search_query is what tells a query from the hint.
    SEARCH_HINT = 'filter by name, tag or local id'

    def _search_placeholder_show(self):
        if self._search_placeholder or self._search_var.get():
            return
        self._search_placeholder = True
        self._search_entry.configure(foreground='#808080')
        self._search_var.set(self.SEARCH_HINT)

    def _search_placeholder_hide(self):
        if not self._search_placeholder:
            return
        self._search_placeholder = False
        self._search_entry.configure(foreground=self._search_entry_fg)
        self._search_var.set('')

    def _search_query(self):
        if getattr(self, '_search_placeholder', False):
            return ''
        return self._search_var.get().strip()

    def _on_search_changed(self):
        self._apply_tree_filter()
        self._tree_autosize_column()

    def _clear_search(self):
        if self._search_placeholder:
            return  # nothing was typed; the box is showing the hint
        if self._search_var.get():
            self._search_var.set('')
        # clearing with the caret still in the box leaves it empty: the hint
        # would sit behind the caret and read like typed text
        if self._search_entry.focus_get() is not self._search_entry:
            self._search_placeholder_show()

    @staticmethod
    def _component_tags(comp):
        return utils.component_tags(comp)

    # (display_name, global_id, lowercase haystack) for a tree row. Searching
    # covers name, tags and local id - not the whole global id, so a query
    # cannot match on a path fragment of some unrelated ancestor.
    def _row_search_fields(self, iid):
        comp = self.context.nodes.get(iid)
        if comp is None:
            name = self.tree.item(iid, 'text').strip()
            return name, iid, name.lower()

        name = self.get_component_tree_name(comp)
        local_id = getattr(comp, 'local_id', '') or ''
        terms = [name, str(local_id)] + self._component_tags(comp)
        display = name.split(' | ')[0].strip()  # drop operation-mode suffix
        return display, iid, ' '.join(terms).lower()

    # snapshot of the fully built tree, so the filter can restore it before
    # re-applying without a full rebuild (keeps selection while typing)
    def _tree_capture_structure(self):
        self._tree_all_items = []

        def walk(parent):
            for index, iid in enumerate(self.tree.get_children(parent)):
                self._tree_all_items.append(
                    (iid, parent, index, bool(self.tree.item(iid, 'open'))))
                walk(iid)
        walk('')

    # Only meaningful with no filter applied: while one is, rows are detached
    # and a fresh walk would drop them from the snapshot entirely.
    def _tree_resnapshot_open_state(self):
        if self._search_query():
            return
        self._tree_capture_structure()

    def _tree_descendants(self, iid):
        out = []
        for child in self.tree.get_children(iid):
            out.append(child)
            out.extend(self._tree_descendants(child))
        return out

    # shows rows matching the search text plus their parent chain (and the
    # subtree of a matched row); an empty search restores the full view
    def _apply_tree_filter(self):
        query = self._search_query().lower()

        # Filtering works from the fully built tree every time, so the first
        # step is always to put back what the last query moved or detached.
        for iid, parent, index, is_open in self._tree_all_items:
            if self.tree.exists(iid):
                self.tree.move(iid, parent, index)
                self.tree.item(iid, open=is_open)

        if not query:
            self._tree_overlays_update()
            return

        keep = set()
        matches = []
        for iid, parent, index, is_open in self._tree_all_items:
            if iid in self._nested_fb_indicators:
                continue  # skip the "add function block" placeholder rows
            name, gid, hay = self._row_search_fields(iid)
            if query in hay:
                matches.append((iid, name, gid))
                keep.add(iid)
                ancestor = self.tree.parent(iid)
                while ancestor:
                    keep.add(ancestor)
                    ancestor = self.tree.parent(ancestor)

        for iid, parent, index, is_open in self._tree_all_items:
            if iid not in keep and self.tree.exists(iid):
                self.tree.detach(iid)

        # collapse redundant single default folders in the filtered view
        self._filter_splice_single_folders()
        self._open_to_reveal([iid for iid, _name, _gid in matches])

        self._tree_overlays_update()

    # like tree_splice_single_folders but hides (detach) instead of deleting,
    # so the full tree can be restored when the filter changes
    def _filter_splice_single_folders(self, parent_iid=''):
        for iid in self.tree.get_children(parent_iid):
            self._filter_splice_single_folders(iid)
        if not parent_iid:
            return
        children = self.tree.get_children(parent_iid)
        if len(children) != 1 or not self._is_default_folder(children[0]):
            return
        folder_iid = children[0]
        for index, child in enumerate(self.tree.get_children(folder_iid)):
            self.tree.move(child, parent_iid, index)
        self.tree.detach(folder_iid)

    # expands only the parent chains that lead to a match, so filtering never
    # unfolds a match's own subtree. Everything else keeps the expansion the
    # user left it at, which is what the restore pass above put back.
    def _open_to_reveal(self, iids):
        for iid in iids:
            ancestor = self.tree.parent(iid)
            while ancestor:
                self.tree.item(ancestor, open=True)
                ancestor = self.tree.parent(ancestor)

    def tree_update(self, new_selected_node=None):
        # The filter detaches non-matching rows instead of deleting them, so that
        # clearing it restores the tree without a rebuild and without losing the
        # selection. The price is paid here: a detached row still belongs to the
        # widget and keeps its id reserved, while having no parent makes it
        # unreachable through get_children, so a rebuild walks into
        # "Item ... already exists". Deleting by id first keeps the cheap restore;
        # deleting on filter instead would have thrown it away.
        for iid, _parent, _index, _is_open in self._tree_all_items:
            if self.tree.exists(iid):
                self.tree.delete(iid)
        self._tree_all_items = []

        self.tree.delete(*self.tree.get_children())
        self.right_side_panel_clear()
        self._nested_fb_indicators = {}
        self._nested_fb_types_cache = {}
        self._tree_hover_set(None)

        self.context.selected_node = new_selected_node

        if self.current_tab() == DisplayType.MODULES:
            self.modules_map = {}
            for mod in self.context.instance.module_manager.modules:
                info = mod.module_info
                
                if info is None:
                    continue
                
                mod_id = str(info.id)
                if not mod_id:
                    mod_id = f'__module_{len(self.modules_map)}__'

                display_name = str(info.name) if info.name else mod_id

                self.tree.insert('', tk.END, iid=mod_id,
                                 text=self._format_tree_item_text(display_name), open=False)
                self.modules_map[mod_id] = mod
            self._tree_capture_structure()
            self._apply_tree_filter()
            self._tree_autosize_column()
            return

        self.tree_traverse_components_recursive(
            self.context.instance, self.current_tab())
        self.tree_splice_single_folders()
        self.tree_insert_nested_fb_indicators()
        self._tree_capture_structure()
        self._apply_tree_filter()
        self.tree_restore_selection(
            self.context.selected_node)  # reset in case the selected node outdates
        self.set_node_update_status()
        self.set_node_lock_status()
        self.set_node_active_status()

    def tree_traverse_components_recursive(
            self, component, display_type=DisplayType.UNSPECIFIED, tree_parent_id=None):
        if component is None:
            return

        self.context.nodes[component.global_id] = component

        folder = daq.IFolder.cast_from(
            component) if component and daq.IFolder.can_cast_from(component) else None
        device = daq.IDevice.cast_from(
            component) if component and daq.IDevice.can_cast_from(component) else None
        items = folder.get_items(daq.AnySearchFilter(
        ) if self.context.view_hidden_components else None) if folder else []

        # Where this row hangs. The flat tabs (Signals, and the top level of
        # Channels and Function blocks) drop the ancestry on purpose so every
        # match is a root row; tree_parent_id is how a recursion opts back into
        # nesting, for the contents of a matched block.
        if tree_parent_id is not None:
            parent_id = tree_parent_id
        elif display_type not in (
                DisplayType.UNSPECIFIED, DisplayType.TOPOLOGY, DisplayType.SYSTEM_OVERVIEW,
                DisplayType.TOPOLOGY_CUSTOM_COMPONENTS, None) or component.parent is None:
            parent_id = ''
        else:
            parent_id = component.parent.global_id
        
        is_fb = daq.IFunctionBlock.can_cast_from(component)
        is_channel = daq.IChannel.can_cast_from(component)
            
        if folder is None or items or display_type == DisplayType.TOPOLOGY_CUSTOM_COMPONENTS:
            if display_type in (DisplayType.UNSPECIFIED, DisplayType.TOPOLOGY,
                                DisplayType.TOPOLOGY_CUSTOM_COMPONENTS, None):
                self.tree_add_component(
                    parent_id, component, display_type == DisplayType.TOPOLOGY_CUSTOM_COMPONENTS)
            elif display_type == DisplayType.SYSTEM_OVERVIEW:
                if not (daq.IInputPort.can_cast_from(component) or daq.ISignal.can_cast_from(
                        component) or daq.IServer.can_cast_from(component)):
                    if not (daq.IFolder.can_cast_from(component)
                            and component.name in ('IP', 'Sig', 'Srv')):
                        self.tree_add_component(parent_id, component)
            elif display_type == DisplayType.SIGNALS and daq.ISignal.can_cast_from(component):
                self.tree_add_component(
                    parent_id, daq.ISignal.cast_from(component))
            elif display_type == DisplayType.CHANNELS:
                if daq.IChannel.can_cast_from(component):
                    self.tree_add_component(
                        parent_id, daq.IChannel.cast_from(component))
                elif tree_parent_id is not None and is_fb:
                    self.tree_add_component(parent_id, daq.IFunctionBlock.cast_from(component))
            elif display_type == DisplayType.FUNCTION_BLOCKS:
                if daq.IFunctionBlock.can_cast_from(
                        component) and not daq.IChannel.can_cast_from(component):
                    self.tree_add_component(
                        parent_id, daq.IFunctionBlock.cast_from(component))

        if folder is not None and (self.context.view_hidden_components or folder.visible):
            if display_type == DisplayType.FUNCTION_BLOCKS and is_fb and not is_channel:
                for item in items:
                    self.tree_traverse_components_recursive(
                        item, display_type=display_type, tree_parent_id=component.global_id)
            elif display_type == DisplayType.CHANNELS and (is_channel or (tree_parent_id is not None and is_fb)):
                for item in items:
                    self.tree_traverse_components_recursive(
                        item, display_type=display_type, tree_parent_id=component.global_id)
            elif not (is_fb and display_type == DisplayType.FUNCTION_BLOCKS):
                for item in items:
                    self.tree_traverse_components_recursive(
                        item, display_type=display_type, tree_parent_id=tree_parent_id)

        if device is not None and display_type == DisplayType.TOPOLOGY:
            custom_components = device.custom_components
            for item in custom_components:
                if item.visible or self.context.view_hidden_components:
                    self.context.custom_component_ids.add(item.global_id)
                    self.tree_traverse_components_recursive(
                        item, display_type=DisplayType.TOPOLOGY_CUSTOM_COMPONENTS)

    def tree_add_component(self, parent_node_id,
                           component, show_unknown=False):
        component_node_id = component.global_id
        component_name = self.get_component_tree_name(component)
        icon = self.context.icons['circle']
        skip = not self.context.view_hidden_components and not component.visible

        if daq.IChannel.can_cast_from(component):
            icon = self.context.icons['channel']
        elif daq.ISignal.can_cast_from(component):
            icon = self.context.icons['signal']
        elif daq.IFunctionBlock.can_cast_from(component):
            icon = self.context.icons['function_block']
        elif daq.IInputPort.can_cast_from(component):
            icon = self.context.icons['input_port']
        elif daq.IDevice.can_cast_from(component):
            icon = self.context.icons['device']
        elif daq.IServer.can_cast_from(component):
            icon = self.context.icons['server']
        elif daq.IFolder.can_cast_from(component):
            icon = self.context.icons['folder']
            component_name = self.get_component_tree_name(component)
        elif daq.ISyncComponent.can_cast_from(component):
            icon = self.context.icons['link']
        else:  # skipping unknown type components
            skip = not show_unknown

        if not skip:
            status_string = None
            try:
                status = component.status_container.get_status('ComponentStatus')
                if status == daq.Enumeration(daq.String('ComponentStatusType'), daq.String('Warning'), component.context.type_manager):
                    status_string = 'warning'
                elif status == daq.Enumeration(daq.String('ComponentStatusType'), daq.String('Error'), component.context.type_manager):
                    status_string = 'error'
            except:
                pass
            
            is_open = not daq.IFunctionBlock.can_cast_from(component)
            
            self.tree.insert(parent_node_id, tk.END, iid=component_node_id, image=icon,
                             text=self._format_tree_item_text(component_name), open=is_open, values=(component_node_id,), tags=(status_string,))

    # a default folder that is the only child of its parent adds a level
    # without information; hoist its children up and drop the folder row
    def tree_splice_single_folders(self, parent_iid=''):
        for iid in self.tree.get_children(parent_iid):
            self.tree_splice_single_folders(iid)
        if not parent_iid:
            return
        children = self.tree.get_children(parent_iid)
        if len(children) != 1 or not self._is_default_folder(children[0]):
            return
        folder_iid = children[0]
        for index, child in enumerate(self.tree.get_children(folder_iid)):
            self.tree.move(child, parent_iid, index)
        self.tree.delete(folder_iid)

    # MARK: - Nested function block indicators

    # The nested function block types a tree row offers, or None when the row is
    # not a function block or offers none. Cached for the life of the built tree:
    # on a remote block this is an RPC, and the hover overlays ask twice per pass
    # while scrolling and resizing run that pass repeatedly. The cache is dropped
    # in tree_update, which is also the only thing that acts on the answer.
    def _nested_fb_types(self, iid):
        if iid in self._nested_fb_types_cache:
            return self._nested_fb_types_cache[iid]
        types = self._nested_fb_types_uncached(iid)
        self._nested_fb_types_cache[iid] = types
        return types

    def _nested_fb_types_uncached(self, iid):
        component = self.context.nodes.get(iid)
        if component is None or not daq.IFunctionBlock.can_cast_from(component):
            return None
        fb = daq.IFunctionBlock.cast_from(component)
        try:
            fb_types = fb.available_function_block_types
        except Exception:
            # Deliberately broader than RuntimeError. Asking a remote block for
            # its nested types needs config protocol 9, and the SDK reports the
            # refusal in more ways than one; anything escaping here aborts the
            # whole indicator pass, so every later row silently loses its
            # placeholders too.
            return None
        return fb_types if fb_types else None

    # appends a grayed-out indicator row for every nested function block
    # type a function block or channel offers; a single click on the row
    # adds that function block directly
    def tree_insert_nested_fb_indicators(self, parent_iid=''):
        if not self.context.view_nested_fb:
            return

        for iid in self.tree.get_children(parent_iid):
            self.tree_insert_nested_fb_indicators(iid)

            # hidden rows still report types: the button that unhides them is
            # only offered where there is something to unhide
            fb_types = self._nested_fb_types(iid)
            if not fb_types or iid in self.context.nested_fb_hidden:
                continue

            # Every offered type keeps its indicator - nesting is not on/off,
            # many blocks accept more than one instance - except where this
            # block is full: it either said no once, or it is a type declared
            # to take a fixed number and already holds them.
            for fb_type_id in fb_types.keys():
                fb_type_id = str(fb_type_id)
                if not self._nested_fb_offered(iid, fb_type_id):
                    continue
                try:
                    display_name = daq.IComponentType.cast_from(
                        fb_types[fb_type_id]).name or fb_type_id
                except RuntimeError:
                    display_name = fb_type_id

                indicator_iid = f'__nested_fb__|{iid}|{fb_type_id}'
                if self.tree.exists(indicator_iid):
                    continue
                self.tree.insert(iid, tk.END, iid=indicator_iid,
                                 image=self.context.icons['add_fb'],
                                 text=self._format_tree_item_text(display_name),
                                 tags=('nested_fb',))
                self._nested_fb_indicators[indicator_iid] = (iid, fb_type_id)

                # expand the ancestor chain so the indicator is visible
                ancestor = iid
                while ancestor:
                    self.tree.item(ancestor, open=True)
                    ancestor = self.tree.parent(ancestor)

    # places the floating row-action buttons: add pinned to the instance row,
    # and on the hovered row whichever of add / remove / nested / lock apply
    def _tree_overlays_update(self):
        if not self._tree_action_buttons:
            return

        pad = int(6 * self.context.ui_scaling_factor * self.context.dpi_factor)
        width = self.tree.winfo_width()
        placements = {}
        backgrounds = {}

        # lays buttons out right to left along a row, pinned to the visible
        # right edge of the tree.
        def place_row(iid, wanted):
            bbox = self.tree.bbox(iid)
            if not bbox:
                return
            x = width - pad
            for key, show in wanted:
                if not show:
                    continue
                btn = self._tree_action_buttons[key]
                x -= btn.winfo_reqwidth()
                # Keying the left limit off the end of the row's own text used
                # to drop the buttons entirely on any row wide enough to run
                # past the visible edge, which is the deeply nested rows and so
                # exactly the ones with the most to add or remove. A long name
                # is readable by scrolling; a button that was never placed is
                # not reachable at all. Only running out of widget stops them.
                if x < 0:
                    break
                y = bbox[1] + (bbox[3] - btn.winfo_reqheight()) // 2
                placements[key] = (x, y)
                backgrounds[key] = self._row_background(iid)
                x -= pad

        root_iid = self.context.instance.global_id \
            if self.context.instance is not None else None
        if root_iid and self.tree.exists(root_iid):
            place_row(root_iid, (('add', True),))

        hover = self._tree_hover_row
        if hover and hover != root_iid and self.tree.exists(hover):
            lockable = self._device_lockable(hover)
            if lockable:
                self._tree_action_buttons['lock'].configure(
                    image=self.context.icons[
                        'unlock' if self._device_locked(hover) else 'lock'])
            nested = self._nested_fb_togglable(hover)
            if nested:
                self._tree_action_buttons['nested_fb'].configure(
                    image=self.context.icons[
                        'add_fb' if hover in self.context.nested_fb_hidden
                        else 'right'])
            # keep the '+' (add) rightmost, then remove, then the nested
            # placeholder toggle beside it, then lock
            place_row(hover, (('hover_add', self._row_addable(hover)),
                              ('remove', self._component_removable(hover)),
                              ('nested_fb', nested),
                              ('lock', lockable)))

        # These buttons follow the hovered row, so they get moved out from under
        # the pointer and back under it as it travels. Tk reports that as <Enter>
        # with no matching <Leave>, which left a tooltip trailing the mouse down
        # the tree; a button that just moved has not been pointed at.
        for key, btn in self._tree_action_buttons.items():
            pos = placements.get(key)
            if pos is None:
                if self._tree_action_pos.get(key) is not None:
                    utils.tooltip_dismiss(btn)
                    btn.place_forget()
                    self._tree_action_pos[key] = None
                    self._tree_action_base_bg[key] = None
                continue
            # the row it floats over is what it blends into when idle, and what
            # the hover and pressed shades are derived from
            self._tree_action_base_bg[key] = backgrounds.get(
                key, self._tree_field_bg)
            if self._tree_action_pos.get(key) != pos:
                utils.tooltip_dismiss(btn)
                btn.place(x=pos[0], y=pos[1])
                self._tree_action_pos[key] = pos
        self._tree_action_buttons_restyle()

    # tracks the hovered row: nested FB indicators get their hover style,
    # removable rows get the floating remove button
    def _tree_hover_set(self, iid):
        if iid and not self.tree.exists(iid):
            iid = None
        if iid == self._tree_hover_row:
            return
        prev = self._tree_hover_row
        if prev and prev in self._nested_fb_indicators and self.tree.exists(prev):
            self.tree.item(prev, tags=('nested_fb',))
        self._tree_hover_row = iid
        if iid and iid in self._nested_fb_indicators:
            self.tree.item(iid, tags=('nested_fb_hover',))
        self._tree_overlays_update()

    def _handle_tree_motion(self, event):
        self._tree_hover_set(self.tree.identify_row(event.y) or None)

    def _handle_tree_leave(self, event):
        # moving onto a floating action button also fires <Leave>; keep the
        # hover state in that case
        widget = self.winfo_containing(event.x_root, event.y_root)
        if widget is not None and widget in self._tree_action_buttons.values():
            return
        self._tree_hover_set(None)

    def _handle_action_button_leave(self, event):
        widget = self.winfo_containing(event.x_root, event.y_root)
        if widget is self.tree:
            return  # tree <Motion> takes over from here
        self._tree_hover_set(None)

    def add_nested_function_block(self, indicator_iid):
        parent_iid, fb_type_id = self._nested_fb_indicators.get(indicator_iid, (None, None))
        if parent_iid is None:
            return

        component = self.context.nodes.get(parent_iid)
        if component is None or not daq.IFunctionBlock.can_cast_from(component):
            return
        fb = daq.IFunctionBlock.cast_from(component)

        try:
            new_fb = fb.add_function_block(fb_type_id)
        except Exception as e:
            # Nothing reports how many instances of a nested type a block
            # accepts, so a refusal is the only way to find out. Remember it and
            # stop offering that type here rather than letting the row sit there
            # failing - each attempt also leaves the parent in Error status.
            self.context.nested_fb_full.add((parent_iid, fb_type_id))
            utils.show_error(
                'Cannot add function block',
                f'{fb_type_id} was refused by this block:\n\n{str(e)}\n\n'
                f'It will not be offered here again until you remove one of '
                f'them or refresh.', self)
            self.tree_update(self.context.selected_node)
            return

        self.tree_update(new_fb)

    DEFAULT_FOLDER_NAMES = frozenset(('Sig', 'FB', 'Dev', 'IP', 'IO', 'Srv'))

    def _is_default_folder(self, iid):
        if not iid:
            return False
        node = utils.find_component(iid, self.context.instance)
        if node is None or not daq.IFolder.can_cast_from(node):
            return False
        return node.name in self.DEFAULT_FOLDER_NAMES

    def get_standard_folder_name(self, component):
        if component == 'Sig':
            component = 'Signals'
        elif component == 'FB':
            component = 'Function blocks'
        elif component == 'Dev':
            component = 'Devices'
        elif component == 'IP':
            component = 'Input ports'
        elif component == 'IO':
            component = 'Inputs/Outputs'
        elif component == 'Srv':
            component = 'Servers'
        return component

    def operation_mode_to_string(self, op_mode):
        if op_mode == daq.OperationModeType.Unknown:
            return '/'
        if op_mode == daq.OperationModeType.Idle:
            return 'Idle'
        if op_mode == daq.OperationModeType.Operation:
            return 'Operation'
        if op_mode == daq.OperationModeType.SafeOperation:
            return 'SafeOperation'
        return ''

    def get_component_tree_name(self, component):
        component_name = self.get_standard_folder_name(component.name)
        if daq.IDevice.can_cast_from(component):
            device = daq.IDevice.cast_from(component)
            if device.operation_mode is not None:
                mode = self.operation_mode_to_string(device.operation_mode)
                if mode:
                    component_name = f'{component_name} | {mode}'
        return component_name

    def _build_component_state_labels(self, component, tags):
        labels = []

        # Component health state (for all components)
        if 'error' in tags:
            labels.append('err')
        elif 'warning' in tags:
            labels.append('warn')

        # Activity state (for all components)
        if component is not None and daq.IComponent.can_cast_from(component):
            try:
                if not daq.IComponent.cast_from(component).active:
                    labels.append('inactive')
            except Exception:
                pass

        # Device-only states
        if component is not None and daq.IDevice.can_cast_from(component):
            try:
                if not utils.is_device_connected(daq.IDevice.cast_from(component)):
                    labels.append('disconnected')
            except Exception:
                pass

        # Inherited lock state
        if 'locked' in tags:
            labels.append('locked')

        # Inside a begin_update block. A tag foreground loses to the selection
        # highlight and begin_update always runs on the selected row, so the
        # marker has to be in the text to be seen at all.
        if 'updating' in tags:
            labels.append('in update')

        return labels

    def _update_tree_item_visual_state(self, node):
        component = utils.find_component(node, self.context.instance)
        if component is None:
            return

        tags = set(self.tree.item(node, 'tags'))
        base_name = self.get_component_tree_name(component)
        labels = self._build_component_state_labels(component, tags)
        # ' | ' separated, matching the rest of the row rather than bracketing.
        suffix = ''.join(f' | {label}' for label in labels)
        self.tree.item(node, text=self._format_tree_item_text(base_name + suffix))

    def _format_tree_item_text(self, text):
        # Visual gap between icon and text in the tree item column.
        return f' {text}'

    def tree_restore_selection(self, old_node=None):
        desired_iid = old_node.global_id if old_node else ''
        current_iid = utils.treeview_get_first_selection(self.tree) or ''

        node = utils.find_component(desired_iid, self.context.instance)

        # if component is alive and in treeview
        if node and self.tree.exists(desired_iid):
            if desired_iid != current_iid:  # if component is not already selected
                self.tree.selection_set(desired_iid)
                self.tree.focus(desired_iid)
                self.tree.see(desired_iid)
        elif old_node and old_node.parent:  # try to select parent
            self.tree_restore_selection(old_node.parent)
        else:  # fallback
            self.tree.selection_set('')

    def right_side_panel_create(self, parent_frame):
        sframe = ttk.Frame(parent_frame)
        sframe.pack(fill=tk.BOTH, expand=True)

        self.right_side_panel = sframe
        self.right_side_canvas = None

    # MARK: - Add dialogs (non-modal)

    # shows an add dialog without grabbing input, so the main window stays
    # usable while it is open; an already open dialog is raised instead of
    # opening a second one
    def floating_dialog_show(self, key, factory, retarget=None):
        dialog = self._floating_dialogs.get(key)
        if dialog is not None and dialog.winfo_exists():
            if retarget is not None:
                retarget(dialog)
            dialog.deiconify()
            dialog.lift()
            dialog.focus_set()
            return
        dialog = factory()
        self._floating_dialogs[key] = dialog
        dialog.show_floating()

    # Cached dialogs are keyed by kind, and these two build a different layout
    # depending on it: opened on the instance they offer the parent tree,
    # opened on a component that component is the parent and there is no tree.
    # One cache entry per kind, or reopening would reuse the wrong layout.
    def _add_dialog_key(self, base, component):
        root = self.context.instance
        if component is None or root is None or \
                component.global_id == root.global_id:
            return f'{base}_root'
        return f'{base}_component'

    # MARK: - Add device dialog
    def add_device_dialog_show(self, component=None):
        def retarget(dialog):
            if component is not None:
                dialog.node = component
                dialog.select_parent_device(component.global_id)
        self.floating_dialog_show(
            self._add_dialog_key('add_device', component),
            lambda: AddDeviceDialog(self, self.context, component),
            retarget)

    # MARK: - Add function block dialog
    def add_function_block_dialog_show(self, component=None):
        def retarget(dialog):
            if component is not None:
                dialog.parent_component = component
                dialog.update_dialog()
        self.floating_dialog_show(
            self._add_dialog_key('add_function_block', component),
            lambda: AddFunctionBlockDialog(self, self.context, component),
            retarget)

    # MARK: - Add server dialog
    def add_server_dialog_show(self, component=None):
        self.floating_dialog_show(
            'add_server', lambda: AddServerDialog(self, self.context, component))

    # MARK: - Logs window
    def logs_window_show(self):
        if self._logs_window is not None and self._logs_window.winfo_exists():
            self._logs_window.deiconify()
            self._logs_window.lift()
            self._logs_window.focus_set()
            return
        self._logs_window = LogsWindow(self, self.context)

    # MARK: - Button handlers
    def handle_add_device_button_clicked(self):
        self.add_device_dialog_show()

    def handle_add_function_block_button_clicked(self):
        self.add_function_block_dialog_show()

    def handle_add_server_button_clicked(self):
        self.add_server_dialog_show()

    def handle_logs_button_clicked(self):
        self.logs_window_show()

    def handle_save_config_button_clicked(self):
        file = asksaveasfile(initialfile='config.json', title='Save configuration',
                             defaultextension='.json', filetypes=[('All Files', '*.*'), ('Json', '*.json')])
        if file is None:
            return
        config_string = self.context.instance.save_configuration()
        a = file.write(config_string)
        file.close()

    def handle_load_config_button_clicked(self):
        file = askopenfile(
            parent=self,
            title='Load configuration',
            defaultextension="json",
            filetypes=[('JSON', f'*.json')]
        )
        if file is None:
            return

        dialog = LoadInstanceConfigDialog(self, self.context, file)
        dialog.show()
        self.tree_update()

    def handle_load_modules_button_clicked(self):
        if platform.system() == 'Windows':
            extension = '.module.dll'
        elif platform.system() == 'Darwin':
            extension = '.dylib'
        else:
            extension = '.module.so'

        file_path = askopenfilename(
            parent=self,
            title='Load module',
            defaultextension=extension,
            filetypes=[('openDAQ module', f'*{extension}')]
        )

        if not file_path:
            return

        try:
            self.context.instance.module_manager.load_module(file_path)
            self.tree_update()
        except Exception as e:
            print('Load module failed:', e, file=sys.stderr)
            utils.show_error('Load module failed', str(e), self)

    def handle_refresh_button_clicked(self):
        # refresh is the way out of a refusal that was really a transient error
        self.context.forget_nested_fb_refusals()
        self.tree_update(self.context.selected_node)

    # MARK: - Tree view handlers
    def handle_tree_right_button(self, event):
        iid = event.widget.identify_row(event.y)
        if iid:
            selected_iid = utils.treeview_get_first_selection(
                event.widget) or ''
            if iid != selected_iid:
                event.widget.selection_set(iid)
        else:
            event.widget.selection_set()
            
    def _block_indicator_double_click(self, event):
        if self.tree.identify_element(event.x, event.y) == 'indicator':
            return 'break'
        # a double click on an indicator row already added the block on the
        # first click; swallow the second one
        iid = self.tree.identify_row(event.y)
        if iid and iid in self._nested_fb_indicators:
            return 'break'

    def handle_tree_click(self, event):
        iid = self.tree.identify_row(event.y)
        element = self.tree.identify_element(event.x, event.y)

        if iid and iid in self._nested_fb_indicators:
            # a single click on an indicator row adds the function block
            self.add_nested_function_block(iid)
            return 'break'

        if element == 'indicator':
            if iid:
                self.tree.item(iid, open=not self.tree.item(iid, 'open'))
            return 'break'

        if iid and self._is_default_folder(iid):
            self.tree.item(iid, open=not self.tree.item(iid, 'open'))
            return 'break'

        if iid and iid == utils.treeview_get_first_selection(self.tree):
            self.tree.item(iid, open=not self.tree.item(iid, 'open'))
            return 'break'

    def create_property_object_menu(self, node):
        icons = self.context.icons
        popup = tk.Menu(self.tree, tearoff=0)

        popup.add_command(label='Begin update', image=icons['begin_update'],
                          compound=tk.LEFT, command=self.handle_begin_update)
        popup.add_command(label='End update', image=icons['end_update'],
                          compound=tk.LEFT, command=self.handle_end_update)
        popup.add_command(label='Clear property values',
                          image=icons['clear_values'], compound=tk.LEFT,
                          command=lambda: self.handle_tree_clear_property_values(node))

        return popup

    def create_component_menu(self, node):
        return self.create_property_object_menu(node)

    def create_function_block_menu(self, node):
        popup = self.create_property_object_menu(node)

        try:
            has_fb_types = bool(node.available_function_block_types)
        except Exception:
            # Broader than RuntimeError on purpose: asking a remote block for
            # its nested types needs config protocol 9, and an older server, a
            # dropped connection or a permission refusal each surface
            # differently. None of them should cost the whole context menu.
            has_fb_types = False
        if has_fb_types:
            popup.add_command(
                label='Add function block',
                image=self.context.icons['function_block'], compound=tk.LEFT,
                command=lambda: self.add_function_block_dialog_show(node)
            )
        if not daq.IChannel.can_cast_from(node):
            popup.add_command(
                label='Remove',
                image=self.context.icons['trash'], compound=tk.LEFT,
                command=lambda: self.handle_tree_menu_remove_function_block(node)
            )

        return popup

    def create_device_menu(self, node):
        popup = self.create_property_object_menu(node)

        icons = self.context.icons
        popup.add_command(label='Lock', image=icons['lock'], compound=tk.LEFT,
                          command=self.handle_lock)
        popup.add_command(label='Unlock', image=icons['unlock'],
                          compound=tk.LEFT, command=self.handle_unlock)

        try:
            has_fb_types = bool(node.available_function_block_types)
        except Exception:
            # Broader than RuntimeError on purpose: asking a remote block for
            # its nested types needs config protocol 9, and an older server, a
            # dropped connection or a permission refusal each surface
            # differently. None of them should cost the whole context menu.
            has_fb_types = False
        if has_fb_types:
            popup.add_command(
                label='Add function block',
                image=icons['function_block'], compound=tk.LEFT,
                command=lambda: self.add_function_block_dialog_show(node)
            )

        if node.global_id != self.context.instance.global_id:
            popup.add_command(
                label='Remove',
                image=icons['trash'], compound=tk.LEFT,
                command=lambda: self.handle_tree_menu_remove_device(node)
            )

        return popup
    
    def create_server_menu(self, node):
        popup = self.create_property_object_menu(node)

        popup.add_command(label='Enable discovery',
                          image=self.context.icons['link'], compound=tk.LEFT,
                          command=lambda: self.handle_enable_discovery(node))
        popup.add_command(label='Disable discovery',
                          image=self.context.icons['unlink'], compound=tk.LEFT,
                          command=lambda: self.handle_disable_discovery(node))

        return popup

    def handle_enable_discovery(self, node):
        if node is None:
            return
        node.enable_discovery()

    def handle_disable_discovery(self, node):
        if node is None:
            return
        node.disable_discovery()

    def handle_tree_right_button_release(self, event):
        iid = utils.treeview_get_first_selection(self.tree)

        if iid and iid in self._nested_fb_indicators:
            popup = tk.Menu(self.tree, tearoff=0)
            popup.add_command(
                label='Add function block',
                image=self.context.icons['add_fb'], compound=tk.LEFT,
                command=lambda: self.add_nested_function_block(iid))
            try:
                popup.tk_popup(event.x_root, event.y_root, 0)
            finally:
                popup.grab_release()
            return

        node = None
        if iid:
            node = utils.find_component(iid, self.context.instance)

        popup = None
        if node:
            if daq.IFunctionBlock.can_cast_from(node):
                popup = self.create_function_block_menu(daq.IFunctionBlock.cast_from(node))
            elif daq.IDevice.can_cast_from(node):
                popup = self.create_device_menu(daq.IDevice.cast_from(node))
            elif daq.IServer.can_cast_from(node):
                popup = self.create_server_menu(daq.IServer.cast_from(node))

        if popup is None:
            popup = self.create_property_object_menu(node)

        try:
            popup.tk_popup(event.x_root, event.y_root, 0)
        finally:
            popup.grab_release()

    # MARK: - Right hand side panel

    def find_fb_device_folder(self, node):
        if daq.IChannel.can_cast_from(node):
            return daq.IChannel.cast_from(node)
        elif daq.IFunctionBlock.can_cast_from(node):
            return daq.IFunctionBlock.cast_from(node)
        elif daq.IDevice.can_cast_from(node):
            return daq.IDevice.cast_from(node)
        elif daq.IServer.can_cast_from(node):
            return daq.IServer.cast_from(node)
        elif daq.ISyncComponent.can_cast_from(node):
            return daq.ISyncComponent.cast_from(node)
        elif daq.IFolder.can_cast_from(node):
            return daq.IFolder.cast_from(node)
        elif daq.ISignal.can_cast_from(node):
            return daq.ISignal.cast_from(node)

        return self.find_fb_device_folder(
            node.parent) if node is not None else None

    def right_side_panel_clear(self):
        for widget in list(self.right_side_panel.children.values()):
            widget.destroy()

    def right_side_panel_draw_node(self, node):
        if node is None:
            return

        found = self.find_fb_device_folder(
            node) if node.global_id not in self.context.custom_component_ids else node
        if found is None:
            return
        if not found.visible and not self.context.view_hidden_components:
            return

        block_view = BlockView(self.right_side_panel, found, self.context)
        block_view.pack(fill=tk.BOTH,  expand=True)

    # MARK: - Right hand side panel - MODULES
    def right_side_panel_draw_module(self, mod_id):
        if mod_id not in self.modules_map:
            return
        
        mod = self.modules_map[mod_id]
        
        self._draw_module_header(self.right_side_panel, mod)
        self._draw_module_type_columns(self.right_side_panel, mod)

    def _draw_module_header(self, frame, mod):
        info = mod.module_info
        vi = info.version_info

        name = str(info.name) if info.name else str(info.id)

        ttk.Label(frame, text=name,
                  font=("TkDefaultFont", 13, "bold")).pack(anchor=tk.W, padx=10, pady=(10, 5))

        if daq.IDevelopmentVersionInfo.can_cast_from(vi):
            dev_vi = daq.IDevelopmentVersionInfo.cast_from(vi)
            version_str = f"{dev_vi.major}.{dev_vi.minor}.{dev_vi.patch}.{dev_vi.tweak}"
            branch = dev_vi.branch_name
            hash_digest = dev_vi.hash_digest
        elif vi:
            version_str = f"{vi.major}.{vi.minor}.{vi.patch}"
            branch = hash_digest = None
        else:
            version_str = "N/A"
            branch = hash_digest = None

        fields = [("ID", str(info.id)), ("Version", version_str)]
        if branch:
            fields.append(("Branch", branch))
        if hash_digest:
            fields.append(("Hash", hash_digest))

        for label, value in fields:
            row = ttk.Frame(frame)
            row.pack(fill=tk.X, padx=10, pady=1)
            ttk.Label(row, text=f"{label}:", width=12, anchor=tk.W).pack(side=tk.LEFT)
            ttk.Label(row, text=str(value), anchor=tk.W).pack(side=tk.LEFT)

        ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, padx=10, pady=(10, 5))

    def _draw_module_type_columns(self, frame, mod):
        columns_frame = ttk.PanedWindow(frame, orient=tk.HORIZONTAL)
        columns_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        left_frame = ttk.Frame(columns_frame)
        type_tree = ttk.Treeview(left_frame, show='tree', selectmode=tk.BROWSE)
        type_tree.column('#0', width=int(250 * self.context.dpi_factor))
        type_scroll = ttk.Scrollbar(left_frame, orient=tk.VERTICAL, command=type_tree.yview)
        type_tree.configure(yscrollcommand=type_scroll.set)
        type_tree.pack(fill=tk.BOTH, expand=True, side=tk.LEFT)
        type_scroll.pack(fill=tk.Y, side=tk.RIGHT)
        columns_frame.add(left_frame, weight=1)

        right_frame = ttk.Frame(columns_frame)
        columns_frame.add(right_frame, weight=1)

        def set_sash(event=None):
            w = columns_frame.winfo_width()
            if w > 1:
                columns_frame.sashpos(0, w // 2)
        columns_frame.bind('<Map>', set_sash)

        def _safe_get(getter):
            try:
                return getter()
            except (AttributeError, RuntimeError):
                return {}

        sections = [
            ("Device Types", _safe_get(lambda: mod.available_device_types), "device"),
            ("Function Block Types", _safe_get(lambda: mod.available_function_block_types), "function_block"),
            ("Server Types", _safe_get(lambda: mod.available_server_types), "server"),
            ("Streaming Types", _safe_get(lambda: mod.available_streaming_types), "streaming"),
        ]

        type_data_map = {}
        for section_name, types_dict, type_kind in sections:
            count = len(types_dict) if types_dict else 0
            section_id = type_tree.insert('', tk.END, text=f" {section_name} ({count})", open=False)
            if types_dict:
                for key, comp_type in types_dict.items():
                    ctype = daq.IComponentType.cast_from(comp_type)
                    iid = type_tree.insert(section_id, tk.END, text=f" {ctype.name or key}")
                    type_data_map[iid] = {"comp_type": comp_type, "type_kind": type_kind, "key": key}

        def on_type_selected(event):
            selected = utils.treeview_get_first_selection(type_tree)
            if selected is None or selected not in type_data_map:
                return
            for widget in right_frame.winfo_children():
                widget.destroy()
            self._draw_module_type_detail(right_frame, type_data_map[selected])

        type_tree.bind('<<TreeviewSelect>>', on_type_selected)

        def update_columns_height(event=None):
            if self.right_side_canvas is not None:
                self.right_side_canvas.update_idletasks()
                canvas_h = self.right_side_canvas.winfo_height()
                remaining = canvas_h - columns_frame.winfo_y() - 10
                if remaining > 100:
                    columns_frame.configure(height=remaining)

        if self.right_side_canvas is not None:
            self.right_side_canvas.bind('<Configure>', update_columns_height, add='+')
        columns_frame.after_idle(update_columns_height)

    def _draw_module_type_detail(self, frame, entry):
        comp_type = entry["comp_type"]
        type_kind = entry["type_kind"]
        key = entry["key"]
        ctype = daq.IComponentType.cast_from(comp_type)

        ttk.Label(frame, text=ctype.name or key,
                  font=("TkDefaultFont", 11, "bold")).pack(anchor=tk.W, padx=10, pady=(5, 2))

        if ctype.description:
            ttk.Label(frame, text=ctype.description, foreground="gray",
                      wraplength=400).pack(anchor=tk.W, padx=10, pady=(0, 5))

        info_fields = [("ID", ctype.id)]
        if type_kind == "device" and daq.IDeviceType.can_cast_from(comp_type):
            prefix = daq.IDeviceType.cast_from(comp_type).connection_string_prefix
            info_fields.append(("Prefix", prefix))  # the label row already handles None -> "N/A"
        elif type_kind == "streaming" and daq.IStreamingType.can_cast_from(comp_type):
            prefix = daq.IStreamingType.cast_from(comp_type).connection_string_prefix
            info_fields.append(("Prefix", prefix))

        for label, value in info_fields:
            row = ttk.Frame(frame)
            row.pack(fill=tk.X, padx=10, pady=1)
            ttk.Label(row, text=f"{label}:", width=12, anchor=tk.W).pack(side=tk.LEFT)
            ttk.Label(row, text=str(value) if value else "N/A", anchor=tk.W).pack(side=tk.LEFT)

        config = ctype.create_default_config()
        if config is not None and len(config.all_properties) > 0:
            ttk.Separator(frame, orient=tk.HORIZONTAL).pack(fill=tk.X, padx=10, pady=10)
            ttk.Label(frame, text="Default Configuration",
                      font=("TkDefaultFont", 10, "bold")).pack(anchor=tk.W, padx=10, pady=(0, 5))
            config_frame = ttk.Frame(frame, height=int(300 * self.context.dpi_factor))
            config_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
            config_frame.pack_propagate(False)
            PropertiesView(config_frame, config, self.context, read_only=True).pack(fill=tk.BOTH, expand=True)

    # MARK: - Tree view handlers

    def handle_tree_select(self, event):
        selected_iid = utils.treeview_get_first_selection(self.tree)
        if selected_iid is None:
            self.context.selected_node = None
            return
        
        if self.current_tab() == DisplayType.MODULES:
            self.right_side_panel_clear()
            self.right_side_panel_draw_module(selected_iid)
            return

        if selected_iid in self._nested_fb_indicators:
            return

        item = self.tree.item(selected_iid)
        # WA for IDs with spaces
        node_unique_id = ' '.join(str(val) for val in item['values'])
        if node_unique_id not in self.context.nodes:
            return
        node = self.context.nodes[node_unique_id]

        if (daq.IFolder.can_cast_from(node)
                and not daq.IDevice.can_cast_from(node)
                and not daq.IFunctionBlock.can_cast_from(node)
                and not daq.IServer.can_cast_from(node)
                and node.name in self.DEFAULT_FOLDER_NAMES):
            self.tree.item(selected_iid, open=not self.tree.item(selected_iid, 'open'))
            self.tree.selection_set('')
            return

        self.context.selected_node = node
        
        self.right_side_panel_clear()
        self.right_side_panel_draw_node(node)

    def handle_tree_menu_remove_function_block(self, node):
        if node is None:
            return
        if not daq.IFunctionBlock.can_cast_from(node):
            return

        node = daq.IFunctionBlock.cast_from(node)

        # searching nearest fb up the tree
        # if no parent fb found, then trying to remove from nearest parent device
        device = utils.get_nearest_device(node.parent, self.context.instance)
        parent_fb = utils.get_nearest_fb(node.parent, device)

        removed_id = node.global_id
        parent_fb.remove_function_block(node)
        # The removal frees whatever capacity a refusal was about, so the parent
        # offers its nested types again. The removed block's own refusals go too:
        # re-adding it gets the same global id back and would inherit them.
        self.context.forget_nested_fb_refusals(parent_fb.global_id)
        self.context.forget_nested_fb_refusals(removed_id)
        self.context.selected_node = parent_fb
        self.tree_update(self.context.selected_node)

    def handle_tree_menu_remove_device(self, node):
        if type(node) is not daq.IDevice:
            node = daq.IDevice.cast_from(
                node) if daq.IDevice.can_cast_from(node) else None

        if not node:
            return
        parent = node.parent
        removed_id = node.global_id
        self.context.remove_device(node)
        # same as removing a function block: nothing learned about what used to
        # be under here survives it
        self.context.forget_nested_fb_refusals(removed_id)

        self.context.selected_node = parent
        self.tree_update(self.context.selected_node)

    def handle_tree_clear_property_values(self, node):
        if node is None or not daq.IPropertyObject.can_cast_from(node):
            return
        prop_obj = daq.IPropertyObject.cast_from(node)
        prop_obj.clear_property_values()
        self.tree_update(self.context.selected_node)

    # MARK: - Other

    def on_refresh_event(self, event):
        self.tree_update(self.context.selected_node)

    def on_tab_change(self, event):
        self.tree_update(self.context.selected_node)

    def current_tab(self):
        return DisplayType.from_tab_index(self.nb.index(
            'current')) if self.nb is not None else DisplayType.UNSPECIFIED

    def handle_begin_update(self):
        selected_item = utils.treeview_get_first_selection(self.tree)
        if not selected_item:
            return

        self.context.updating_nodes.add(selected_item)
        self.begin_update_on_node(selected_item)
        self.set_node_update_status()
        self.tree_update(self.context.selected_node)

    def handle_end_update(self):
        selected_item = utils.treeview_get_first_selection(self.tree)
        if not selected_item:
            return

        self.context.updating_nodes.discard(selected_item)
        self.context.clear_pending_properties(selected_item)
        self.end_update_on_node(selected_item)
        self.set_node_update_status()
        self.tree_update(self.context.selected_node)

    def begin_update_on_node(self, node):
        node_obj = utils.find_component(node, self.context.instance)
        node_obj = daq.IPropertyObject.cast_from(node_obj)
        node_obj.begin_update()

    def end_update_on_node(self, node):
        node_obj = utils.find_component(node, self.context.instance)
        node_obj = daq.IPropertyObject.cast_from(node_obj)
        try:
            node_obj.end_update()
        except RuntimeError:
            # Remote components reject it; the marker is cleared regardless.
            pass

    def handle_lock(self):
        self.lock_device_node(utils.treeview_get_first_selection(self.tree))

    def handle_unlock(self):
        self.unlock_device_node(utils.treeview_get_first_selection(self.tree))

    # both the context menu and the hover button land here
    def lock_device_node(self, node):
        component = utils.find_component(node, self.context.instance)

        try:
            device = daq.IDevice.cast_from(component)
            device.lock()
            self._set_node_lock_status_recursive(node)
        except Exception as e:
            utils.show_error('Lock failed', f'{component.name}: {e}', self)
            print(f'Lock failed: {str(e)}', file=sys.stderr)
        self._tree_overlays_update()

    def unlock_device_node(self, node):
        component = utils.find_component(node, self.context.instance)

        try:
            device = daq.IDevice.cast_from(component)
            device.unlock()
            self._set_node_lock_status_recursive(node)
        except Exception as e:
            print(f'Unlock failed: {str(e)}', file=sys.stderr)
            msg = str(e) + '. Do you want to forcefully unlock the device?'
            do_force_unlock = messagebox.askyesno('Unlock failed', msg)
            if do_force_unlock:
                self._force_unlock_device(node, component)
        self._tree_overlays_update()

    def _force_unlock_device(self, node, component):
        try:
            device_private = daq.IDevicePrivate.cast_from(component)
            device_private.force_unlock()
            self._set_node_lock_status_recursive(node)
        except Exception as e:
            print('Force unlock failed: ', e, file=sys.stderr)
            utils.show_error('Force unlock failed', str(e), self)

    def set_node_update_status(self):
        for node in self.tree.get_children():
            self._set_node_update_status_recursive(node)

    def add_tag_and_configure(self, node, tag, color):
        current_tags = set(self.tree.item(node, 'tags'))
        current_tags.add(tag)
        self.tree.tag_configure(tag, foreground=color)
        self.tree.item(node, tags=tuple(current_tags))

    def remove_tag(self, node, tag):
        current_tags = set(self.tree.item(node, 'tags'))
        if tag in current_tags:
            current_tags.remove(tag)
        self.tree.item(node, tags=tuple(current_tags))

    def _set_node_update_status_recursive(self, node):
        if node in self._nested_fb_indicators:
            return
        node_obj = utils.find_component(node, self.context.instance)
        if node_obj is None:
            return
        # Either the user ran Begin update on this row, or the SDK reports it
        # inside a block - locally begin_update cascades, so the whole subtree
        # reports it. _update_tree_item_visual_state turns the tag into the
        # visible marker.
        if self.context.is_in_update(node_obj):
            self.add_tag_and_configure(node, 'updating', '#b8860b')
        else:
            self.remove_tag(node, 'updating')
        self._update_tree_item_visual_state(node)
        children = self.tree.get_children(node)
        for child in children:
            self._set_node_update_status_recursive(child)

    def set_node_lock_status(self):
        for node in self.tree.get_children():
            self._set_node_lock_status_recursive(node)

    def _set_node_lock_status_recursive(self, node, parent_locked=False):
        if node in self._nested_fb_indicators:
            return
        component = utils.find_component(node, self.context.instance)

        if daq.IDevice.can_cast_from(component):
            device = daq.IDevice.cast_from(component)
            try:
                locked = device.locked
            except:
                locked = False
        else:
            locked = parent_locked

        if locked:
            self.add_tag_and_configure(node, 'locked', 'gray')
        else:
            self.remove_tag(node, 'locked')
        self._update_tree_item_visual_state(node)

        children = self.tree.get_children(node)
        for child in children:
            self._set_node_lock_status_recursive(child, locked)

    def set_node_active_status(self):
        for node in self.tree.get_children():
            self._set_node_active_status_recursive(node)

    def _set_node_active_status_recursive(self, node):
        if node in self._nested_fb_indicators:
            return
        component = utils.find_component(node, self.context.instance)

        current_tags = set(self.tree.item(node, 'tags'))
        has_warning_or_error = 'warning' in current_tags or 'error' in current_tags

        is_inactive = False
        if component is not None and daq.IComponent.can_cast_from(component):
            try:
                is_inactive = not daq.IComponent.cast_from(component).active
            except Exception:
                is_inactive = False

        # warning/error color has higher priority
        if not has_warning_or_error and is_inactive:
            self.add_tag_and_configure(node, 'inactive', 'gray')
        else:
            self.remove_tag(node, 'inactive')
        self._update_tree_item_visual_state(node)

        for child in self.tree.get_children(node):
            self._set_node_active_status_recursive(child)

    def _load_config(self, config):
        file = open(config, 'r')
        if file is None:
            return
        config_string = file.read()
        file.close()

        updata_params = daq.UpdateParameters()
        self.context.instance.load_configuration(
            config_string, updata_params)


# MARK: - Entry point
if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Display openDAQ device configuration and plot values')
    parser.add_argument('--scale', help='UI scaling factor',
                        type=int, default=1.0)
    parser.add_argument('--connection_string',
                        help='Connection string', type=str, default='')
    parser.add_argument(
        '--demo', action=argparse.BooleanOptionalAction, default=True,
        help='Include internal demo/reference devices (default: on; use --no-demo to hide)')
    parser.add_argument(
        '--config', help='Saved config', type=str, default='')
    parser.add_argument(
        '--module_path', help='Additional modules path', type=str, default='')
    parser.add_argument('-v', '--version', action='version',
        version=f'{os.path.dirname(__file__)} {daq.__dict__.get("__version__", "@VERSION@").replace("@VERSION@", "Unknown version")}')
    parser.add_argument(
        '--discovery_server', help='Discovery server protocols (comma-separated, e.g. "mdns")',
        type=str, default='mdns')

    app = App(parser.parse_args())
    app.mainloop()
