import math
import warnings
import tkinter as tk
import platform

from tkinter import ttk, filedialog
from tkinter.colorchooser import askcolor

import opendaq
from PIL import Image, ImageTk
from generate_connection_view_image import ConnectionMap

OS = platform.system()


# taken from https://github.com/foobar167/junkyard/blob/master/zoom_advanced3.py


class AutoScrollbar(ttk.Scrollbar):
    """ A scrollbar that hides itself if it's not needed. Works only for grid geometry manager """

    def set(self, lo, hi):
        if float(lo) <= 0.0 and float(hi) >= 1.0:
            self.grid_remove()
        else:
            self.grid()
            ttk.Scrollbar.set(self, lo, hi)

    def pack(self, **kw):
        raise tk.TclError('Cannot use pack with the widget ' + self.__class__.__name__)

    def place(self, **kw):
        raise tk.TclError('Cannot use place with the widget ' + self.__class__.__name__)


class CanvasImage:
    """ Display and zoom image """

    def __init__(self, placeholder, path):
        """ Initialize the ImageFrame """
        self.imscale = 1.0  # scale for the canvas image zoom, public for outer classes
        self.__delta = 1.2  # zoom magnitude
        self.__delta = 1.2  # zoom magnitude
        self.__filter = Image.Resampling.LANCZOS  # could be: NEAREST, BILINEAR, BICUBIC and ANTIALIAS
        self.__previous_state = 0  # previous state of the keyboard
        self.path = path  # path to the image, should be public for outer classes
        # Create ImageFrame in placeholder widget
        self.__imframe = ttk.Frame(placeholder)  # placeholder of the ImageFrame object
        # Vertical and horizontal scrollbars for canvas
        hbar = AutoScrollbar(self.__imframe, orient='horizontal')
        vbar = AutoScrollbar(self.__imframe, orient='vertical')
        hbar.grid(row=1, column=0, sticky='we')
        vbar.grid(row=0, column=1, sticky='ns')
        # Create canvas and bind it with scrollbars. Public for outer classes
        self.canvas = tk.Canvas(self.__imframe, highlightthickness=0,
                                xscrollcommand=hbar.set, yscrollcommand=vbar.set)
        self.canvas.grid(row=0, column=0, sticky='nswe')
        self.canvas.update()  # wait till canvas is created
        hbar.configure(command=self.__scroll_x)  # bind scrollbars to the canvas
        vbar.configure(command=self.__scroll_y)
        # Bind events to the Canvas
        self.canvas.bind('<Configure>', lambda event: self.__show_image())  # canvas is resized
        self.canvas.bind('<ButtonPress-1>', self.__move_from)  # remember canvas position
        self.canvas.bind('<B1-Motion>', self.__move_to)  # move canvas to the new position
        self.canvas.bind('<MouseWheel>', self.__wheel)  # zoom for Windows and MacOS, but not Linux
        self.canvas.bind('<Button-5>', self.__wheel)  # zoom for Linux, wheel scroll down
        self.canvas.bind('<Button-4>', self.__wheel)  # zoom for Linux, wheel scroll up
        # Handle keystrokes in idle mode, because program slows down on a weak computers,
        # when too many key stroke events in the same time
        self.canvas.bind('<Key>', lambda event: self.canvas.after_idle(self.__keystroke, event))
        # Decide if this image huge or not
        self.__huge = False  # huge or not
        self.__huge_size = 6000  # define size of the huge image
        self.__band_width = 1024  # width of the tile band
        Image.MAX_IMAGE_PIXELS = 1000000000  # suppress DecompressionBombError for big image
        with warnings.catch_warnings():  # suppress DecompressionBombWarning for big image
            warnings.simplefilter('ignore')
            self.__image = Image.open(self.path)  # open image, but down't load it into RAM
        self.imwidth, self.imheight = self.__image.size  # public for outer classes
        if self.imwidth * self.imheight > self.__huge_size * self.__huge_size and \
                self.__image.tile[0][0] == 'raw':  # only raw images could be tiled
            self.__huge = True  # image is huge
            self.__offset = self.__image.tile[0][2]  # initial tile offset
            self.__tile = [self.__image.tile[0][0],  # it have to be 'raw'
                           [0, 0, self.imwidth, 0],  # tile extent (a rectangle)
                           self.__offset,
                           self.__image.tile[0][3]]  # list of arguments to the decoder
        self.__min_side = min(self.imwidth, self.imheight)  # get the smaller image side
        # Create image pyramid
        self.__pyramid = [self.smaller()] if self.__huge else [Image.open(self.path)]
        # Set ratio coefficient for image pyramid
        self.__ratio = max(self.imwidth, self.imheight) / self.__huge_size if self.__huge else 1.0
        self.__curr_img = 0  # current image from the pyramid
        self.__scale = self.imscale * self.__ratio  # image pyramide scale
        self.__reduction = 2  # reduction degree of image pyramid
        (w, h), m, j = self.__pyramid[-1].size, 512, 0
        n = math.ceil(math.log(min(w, h) / m, self.__reduction)) + 1  # image pyramid length
        while w > m and h > m:  # top pyramid image is around 512 pixels in size
            j += 1
            print('\rCreating image pyramid: {j} from {n}'.format(j=j, n=n), end='')
            w /= self.__reduction  # divide on reduction degree
            h /= self.__reduction  # divide on reduction degree
            self.__pyramid.append(self.__pyramid[-1].resize((int(w), int(h)), self.__filter))
        print('\r' + (40 * ' ') + '\r', end='')  # hide printed string
        # Put image into container rectangle and use it to set proper coordinates to the image
        self.container = self.canvas.create_rectangle((0, 0, self.imwidth, self.imheight), width=0)
        self.__show_image()  # show image on the canvas
        self.canvas.focus_set()  # set focus on the canvas

    def smaller(self):
        """ Resize image proportionally and return smaller image """
        w1, h1 = float(self.imwidth), float(self.imheight)
        w2, h2 = float(self.__huge_size), float(self.__huge_size)
        aspect_ratio1 = w1 / h1
        aspect_ratio2 = w2 / h2  # it equals to 1.0
        if aspect_ratio1 == aspect_ratio2:
            image = Image.new('RGB', (int(w2), int(h2)))
            k = h2 / h1  # compression ratio
            w = int(w2)  # band length
        elif aspect_ratio1 > aspect_ratio2:
            image = Image.new('RGB', (int(w2), int(w2 / aspect_ratio1)))
            k = h2 / w1  # compression ratio
            w = int(w2)  # band length
        else:  # aspect_ratio1 < aspect_ration2
            image = Image.new('RGB', (int(h2 * aspect_ratio1), int(h2)))
            k = h2 / h1  # compression ratio
            w = int(h2 * aspect_ratio1)  # band length
        i, j, n = 0, 0, math.ceil(self.imheight / self.__band_width)
        while i < self.imheight:
            j += 1
            print('\rOpening image: {j} from {n}'.format(j=j, n=n), end='')
            band = min(self.__band_width, self.imheight - i)  # width of the tile band
            self.__tile[1][3] = band  # set band width
            self.__tile[2] = self.__offset + self.imwidth * i * 3  # tile offset (3 bytes per pixel)
            self.__image.close()
            self.__image = Image.open(self.path)  # reopen / reset image
            self.__image.size = (self.imwidth, band)  # set size of the tile band
            self.__image.tile = [self.__tile]  # set tile
            cropped = self.__image.crop((0, 0, self.imwidth, band))  # crop tile band
            image.paste(cropped.resize((w, int(band * k) + 1), self.__filter), (0, int(i * k)))
            i += band
        print('\r' + (40 * ' ') + '\r', end='')  # hide printed string
        return image

    def redraw_figures(self):
        """ Dummy function to redraw figures in the children classes """
        pass

    def grid(self, **kw):
        """ Put CanvasImage widget on the parent widget """
        self.__imframe.grid(**kw)  # place CanvasImage widget on the grid
        self.__imframe.grid(sticky='nswe')  # make frame container sticky
        self.__imframe.rowconfigure(0, weight=1)  # make canvas expandable
        self.__imframe.columnconfigure(0, weight=1)

    def pack(self, **kw):
        """ Exception: cannot use pack with this widget """
        raise Exception('Cannot use pack with the widget ' + self.__class__.__name__)

    def place(self, **kw):
        """ Exception: cannot use place with this widget """
        raise Exception('Cannot use place with the widget ' + self.__class__.__name__)

    # noinspection PyUnusedLocal
    def __scroll_x(self, *args, **kwargs):
        """ Scroll canvas horizontally and redraw the image """
        self.canvas.xview(*args)  # scroll horizontally
        self.__show_image()  # redraw the image

    # noinspection PyUnusedLocal
    def __scroll_y(self, *args, **kwargs):
        """ Scroll canvas vertically and redraw the image """
        self.canvas.yview(*args)  # scroll vertically
        self.__show_image()  # redraw the image

    def __show_image(self):
        """ Show image on the Canvas. Implements correct image zoom almost like in Google Maps """
        box_image = self.canvas.coords(self.container)  # get image area
        box_canvas = (self.canvas.canvasx(0),  # get visible area of the canvas
                      self.canvas.canvasy(0),
                      self.canvas.canvasx(self.canvas.winfo_width()),
                      self.canvas.canvasy(self.canvas.winfo_height()))
        box_img_int = tuple(map(int, box_image))  # convert to integer or it will not work properly
        # Get scroll region box
        box_scroll = [min(box_img_int[0], box_canvas[0]), min(box_img_int[1], box_canvas[1]),
                      max(box_img_int[2], box_canvas[2]), max(box_img_int[3], box_canvas[3])]
        # Horizontal part of the image is in the visible area
        if box_scroll[0] == box_canvas[0] and box_scroll[2] == box_canvas[2]:
            box_scroll[0] = box_img_int[0]
            box_scroll[2] = box_img_int[2]
        # Vertical part of the image is in the visible area
        if box_scroll[1] == box_canvas[1] and box_scroll[3] == box_canvas[3]:
            box_scroll[1] = box_img_int[1]
            box_scroll[3] = box_img_int[3]
        # Convert scroll region to tuple and to integer
        self.canvas.configure(scrollregion=tuple(map(int, box_scroll)))  # set scroll region
        x1 = max(box_canvas[0] - box_image[0], 0)  # get coordinates (x1,y1,x2,y2) of the image tile
        y1 = max(box_canvas[1] - box_image[1], 0)
        x2 = min(box_canvas[2], box_image[2]) - box_image[0]
        y2 = min(box_canvas[3], box_image[3]) - box_image[1]
        if int(x2 - x1) > 0 and int(y2 - y1) > 0:  # show image if it in the visible area
            if self.__huge and self.__curr_img < 0:  # show huge image, which does not fit in RAM
                h = int((y2 - y1) / self.imscale)  # height of the tile band
                self.__tile[1][3] = h  # set the tile band height
                self.__tile[2] = self.__offset + self.imwidth * int(y1 / self.imscale) * 3
                self.__image.close()
                self.__image = Image.open(self.path)  # reopen / reset image
                self.__image.size = (self.imwidth, h)  # set size of the tile band
                self.__image.tile = [self.__tile]
                image = self.__image.crop((int(x1 / self.imscale), 0, int(x2 / self.imscale), h))
            else:  # show normal image
                image = self.__pyramid[max(0, self.__curr_img)].crop(  # crop current img from pyramid
                    (int(x1 / self.__scale), int(y1 / self.__scale),
                     int(x2 / self.__scale), int(y2 / self.__scale)))
            #
            imagetk = ImageTk.PhotoImage(image.resize((int(x2 - x1), int(y2 - y1)), self.__filter))
            imageid = self.canvas.create_image(max(box_canvas[0], box_img_int[0]),
                                               max(box_canvas[1], box_img_int[1]),
                                               anchor='nw', image=imagetk)
            self.canvas.lower(imageid)  # set image into background
            self.canvas.imagetk = imagetk  # keep an extra reference to prevent garbage-collection

    def __move_from(self, event):
        """ Remember previous coordinates for scrolling with the mouse """
        self.canvas.scan_mark(event.x, event.y)

    def __move_to(self, event):
        """ Drag (move) canvas to the new position """
        self.canvas.scan_dragto(event.x, event.y, gain=1)
        self.__show_image()  # zoom tile and show it on the canvas

    def outside(self, x, y):
        """ Checks if the point (x,y) is outside the image area """
        bbox = self.canvas.coords(self.container)  # get image area
        if bbox[0] < x < bbox[2] and bbox[1] < y < bbox[3]:
            return False  # point (x,y) is inside the image area
        else:
            return True  # point (x,y) is outside the image area

    def __wheel(self, event):
        """ Zoom with mouse wheel """
        x = self.canvas.canvasx(event.x)  # get coordinates of the event on the canvas
        y = self.canvas.canvasy(event.y)
        if self.outside(x, y): return  # zoom only inside image area
        scale = 1.0
        if OS == 'Darwin':
            if event.delta < 0:  # scroll down, zoom out, smaller
                if round(self.__min_side * self.imscale) < 30: return  # image is less than 30 pixels
                self.imscale /= self.__delta
                scale /= self.__delta
            if event.delta > 0:  # scroll up, zoom in, bigger
                i = float(min(self.canvas.winfo_width(), self.canvas.winfo_height()) >> 1)
                if i < self.imscale: return  # 1 pixel is bigger than the visible area
                self.imscale *= self.__delta
                scale *= self.__delta
        else:
            # Respond to Linux (event.num) or Windows (event.delta) wheel event
            if event.num == 5 or event.delta == -120:  # scroll down, zoom out, smaller
                if round(self.__min_side * self.imscale) < 30: return  # image is less than 30 pixels
                self.imscale /= self.__delta
                scale /= self.__delta
            if event.num == 4 or event.delta == 120:  # scroll up, zoom in, bigger
                i = float(min(self.canvas.winfo_width(), self.canvas.winfo_height()) >> 1)
                if i < self.imscale: return  # 1 pixel is bigger than the visible area
                self.imscale *= self.__delta
                scale *= self.__delta
        # Take appropriate image from the pyramid
        k = self.imscale * self.__ratio  # temporary coefficient
        self.__curr_img = min((-1) * int(math.log(k, self.__reduction)), len(self.__pyramid) - 1)
        self.__scale = k * math.pow(self.__reduction, max(0, self.__curr_img))
        #
        self.canvas.scale('all', x, y, scale, scale)  # rescale all objects
        # Redraw some figures before showing image on the screen
        self.redraw_figures()  # method for child classes
        self.__show_image()

    def __keystroke(self, event):
        """ Scrolling with the keyboard.
            Independent from the language of the keyboard, CapsLock, <Ctrl>+<key>, etc. """
        if event.state - self.__previous_state == 4:  # means that the Control key is pressed
            pass  # do nothing if Control key is pressed
        else:
            self.__previous_state = event.state  # remember the last keystroke state
            # Up, Down, Left, Right keystrokes
            if event.keycode in [65, 37, 100]:  # scroll right, keys 'd' or 'Right'
                self.__scroll_x('scroll', 1, 'unit', event=event)
            elif event.keycode in [68, 39, 102]:  # scroll left, keys 'a' or 'Left'
                self.__scroll_x('scroll', -1, 'unit', event=event)
            elif event.keycode in [87, 38, 104]:  # scroll up, keys 'w' or 'Up'
                self.__scroll_y('scroll', -1, 'unit', event=event)
            elif event.keycode in [83, 40, 98]:  # scroll down, keys 's' or 'Down'
                self.__scroll_y('scroll', 1, 'unit', event=event)

    def crop(self, bbox):
        """ Crop rectangle from the image and return it """
        if self.__huge:  # image is huge and not totally in RAM
            band = bbox[3] - bbox[1]  # width of the tile band
            self.__tile[1][3] = band  # set the tile height
            self.__tile[2] = self.__offset + self.imwidth * bbox[1] * 3  # set offset of the band
            self.__image.close()
            self.__image = Image.open(self.path)  # reopen / reset image
            self.__image.size = (self.imwidth, band)  # set size of the tile band
            self.__image.tile = [self.__tile]
            return self.__image.crop((bbox[0], 0, bbox[2], band))
        else:  # image is totally in RAM
            return self.__pyramid[0].crop(bbox)

    def destroy(self):
        """ ImageFrame destructor """
        self.__image.close()
        map(lambda i: i.close, self.__pyramid)  # close all pyramid images
        del self.__pyramid[:]  # delete pyramid list
        del self.__pyramid  # delete pyramid variable
        self.canvas.destroy()
        self.__imframe.destroy()


class ConnectionView(tk.Frame):
    def __init__(self, parent, context=None, **kwargs):
        super().__init__(parent, **kwargs)
        self.parent = parent
        self.context = context
        self.png = None
        self.svg = None
        self.canvas = None
        self.draw()
        self.parent.rowconfigure(0, weight=1)
        self.parent.columnconfigure(0, weight=1)
        self.parent.pack(fill=tk.BOTH, expand=True)

        self.grid(sticky=tk.NSEW)
        self.parent.rowconfigure(0, weight=1)
        self.parent.columnconfigure(0, weight=1)

        button_frame = tk.Frame(self)
        button_frame.grid(row=0, column=0, sticky=tk.E, padx=5, pady=5)

        export_button = tk.Button(button_frame, text="Export", command=self.export)
        export_button.grid(row=0, column=0, padx=5)

        settings_button = tk.Button(button_frame, text="Settings", command=self.open_settings_window)
        settings_button.configure(image=self.context.icons['settings'])
        settings_button.grid(row=0, column=1, padx=5)

        self.rowconfigure(1, weight=1)
        self.columnconfigure(0, weight=1)

    def draw(self, edge_color="orange", node_color="#A0A0A0", font_color="white", cluster_color="#A0A0A0",
             edge_fontsize="15", node_fontsize="18", additionalproperties=False, attributesshow=False,
             inputports=False, show_disconnectedsignals=False, show_connected_signalsonly=False):
        devices = opendaq.List()
        devices.push_back(self.context.instance)
        ConnectionMap(devices, edge_color, node_color, font_color, cluster_color, edge_fontsize, node_fontsize,
                      additionalproperties, attributesshow, inputports, show_disconnectedsignals,
                      show_connected_signalsonly)

        png_path = "connection_map.png"
        svg_path = "connection_map.svg"
        self.png = open(png_path, "rb").read()
        self.svg = open(svg_path, "r").read()
        self.canvas = CanvasImage(self, path="connection_map.png")
        self.canvas.grid(row=1, column=0, sticky=tk.NSEW)
        self.canvas.redraw_figures()

    def export(self):
        file_path = filedialog.asksaveasfilename(defaultextension=".svg",
                                                 filetypes=[("Scalable Vector Graphics", "*.svg"),
                                                            ("Portable Network Graphics", "*.png")])

        if file_path:
            if file_path.endswith('.svg'):
                if self.svg is not None:
                    with open(file_path, "w") as svg_file:
                        svg_file.write(self.svg)
                else:
                    print("SVG data is not available.")
            elif file_path.endswith('.png'):
                if self.png is not None:
                    with open(file_path, "wb") as png_file:
                        png_file.write(self.png)
                else:
                    print("PNG data is not available.")

    def open_settings_window(self):
        if hasattr(self, 'settings_window') and self.settings_window.winfo_exists():
            self.settings_window.lift()
            return

        self.settings_window = tk.Toplevel(self.parent)
        self.settings_window.title("Connection view settings")
        self.settings_window.protocol("WM_DELETE_WINDOW", self.on_settings_window_close)

        main_frame = tk.Frame(self.settings_window)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        left_column = tk.Frame(main_frame)
        left_column.grid(row=1, column=0, sticky=tk.NW, padx=5, pady=5)
        right_column = tk.Frame(main_frame)
        right_column.grid(row=1, column=2, sticky=tk.NE, padx=5, pady=5)

        separator = ttk.Separator(main_frame, orient='vertical')
        separator.grid(row=1, column=1, sticky='ns', padx=5, pady=5)

        left_header = tk.Label(main_frame, text="Settings Options", font=('Arial', 12, 'bold'))
        left_header.grid(row=0, column=0, padx=5, pady=5)

        right_header = tk.Label(main_frame, text="Font and Color Options", font=('Arial', 12, 'bold'))
        right_header.grid(row=0, column=2, padx=5, pady=5)

        self.checkbox_vars = [tk.IntVar() for _ in range(5)]
        checkbox_texts = ["Additional properties", "Show attributes", "Show Input ports", "Show disconnected signals",
                          "Show connected signals only"]
        for i, text in enumerate(checkbox_texts):
            checkbox = tk.Checkbutton(left_column, text=text, variable=self.checkbox_vars[i])
            checkbox.pack(anchor=tk.W, padx=5, pady=2)

        self.textbox_vars = [tk.StringVar() for _ in range(2)]
        font_labels = ["Edge Font Size", "Node Font Size"]
        v_list = [str(i) for i in range(10, 31)]
        for i in range(2):
            label = tk.Label(right_column, text=font_labels[i])
            label.pack(anchor=tk.W, padx=5, pady=2)
            Combo = ttk.Combobox(right_column, values=v_list, textvariable=self.textbox_vars[i], state="readonly")
            Combo.set("Font size")
            Combo.pack(padx=5, pady=5)

        self.color_vars = [tk.StringVar() for _ in range(4)]
        self.color_labels = [tk.Label(right_column, text="", width=10, bg="white") for _ in range(4)]
        array = ['Edge', 'Node', 'Font', 'Cluster']
        for i in range(4):
            color_button = tk.Button(right_column, text=f"Choose Color {array[i]}",
                                     command=lambda var=self.color_vars[i],
                                                    label=self.color_labels[i]: self.choose_color(var, label))
            color_button.pack(anchor=tk.W, padx=5, pady=2)
            self.color_labels[i].pack(anchor=tk.W, padx=5, pady=2)

        button_frame = tk.Frame(self.settings_window)
        button_frame.pack(side=tk.BOTTOM, anchor=tk.SE, padx=5, pady=5)

        apply_button = tk.Button(button_frame, text="Apply", command=self.apply_settings)
        apply_button.pack(side=tk.LEFT, padx=5)

    def on_settings_window_close(self):
        self.settings_window.destroy()
        del self.settings_window

    def choose_color(self, var, label):
        self.settings_window.grab_set()
        color = askcolor()[1]
        self.settings_window.grab_release()
        if color:
            var.set(color)
            label.config(bg=color)

    def apply_settings(self):
        edge_color = self.color_vars[0].get()
        if not edge_color:
            edge_color = "orange"
        node_color = self.color_vars[1].get()
        if not node_color:
            node_color = "gray17"
        font_color = self.color_vars[2].get()
        if not font_color:
            font_color = "white"
        cluster_color = self.color_vars[3].get()
        if not cluster_color:
            cluster_color = "#A0A0A0"
        edge_fontsize = self.textbox_vars[0].get()
        if not edge_fontsize.isdigit():
            edge_fontsize = 15
        node_fontsize = self.textbox_vars[1].get()
        if not node_fontsize.isdigit():
            node_fontsize = 15
        additional_properties = self.checkbox_vars[0].get()
        attributes_show = self.checkbox_vars[1].get()
        input_ports = self.checkbox_vars[2].get()
        show_disconnected_signals = self.checkbox_vars[3].get()
        show_connected_signals_only = self.checkbox_vars[4].get()

        self.draw(edge_color=edge_color, node_color=node_color, font_color=font_color, cluster_color=cluster_color,
                  edge_fontsize=edge_fontsize, node_fontsize=node_fontsize,
                  additionalproperties=additional_properties,
                  attributesshow=attributes_show, inputports=input_ports,
                  show_disconnectedsignals=show_disconnected_signals,
                  show_connected_signalsonly=show_connected_signals_only)
