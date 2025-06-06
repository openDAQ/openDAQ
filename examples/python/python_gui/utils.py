import os
import tkinter as tk

def get_files_in_directory(directory):
    files = []
    for file in os.listdir(directory):
        if os.path.isfile(os.path.join(directory, file)) and file.endswith('.png'):
            files.append(file)
    return files


def load_and_resize_image(filename, x_subsample=10, y_subsample=10):
    image = tk.PhotoImage(file=filename)
    return image.subsample(x_subsample, y_subsample)
