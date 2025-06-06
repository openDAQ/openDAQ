import tkinter as tk
from tkinter import ttk
from tkinter import messagebox

# Create main window
root = tk.Tk()
root.title("Treeview Sidebar Template")
root.geometry("800x600")

# --- Top Bar ---
top_frame = tk.Frame(root, height=50)
top_frame.pack(side="top", fill="x")

def on_new():
    messagebox.showinfo("New", "New File Created")

def on_exit():
    root.quit()

menu_bar = tk.Menu(root)

# File Menu
file_menu = tk.Menu(menu_bar, tearoff=0)
file_menu.add_command(label="New", command=on_new)
file_menu.add_command(label="Open")
file_menu.add_command(label="Save")
file_menu.add_separator()
file_menu.add_command(label="Exit", command=on_exit)
menu_bar.add_cascade(label="File", menu=file_menu)
root.config(menu=menu_bar)

# Edit Menu
edit_menu = tk.Menu(menu_bar, tearoff=0)
edit_menu.add_command(label="Undo")
edit_menu.add_command(label="Redo")
menu_bar.add_cascade(label="Edit", menu=edit_menu)

# Help Menu
help_menu = tk.Menu(menu_bar, tearoff=0)
help_menu.add_command(label="About")
menu_bar.add_cascade(label="Help", menu=help_menu)


# Top right icon
icon = tk.Label(top_frame, text="⚙")
icon.pack(side="right", padx=10)

# --- Content Area ---
content_frame = tk.Frame(root)
content_frame.pack(fill="both", expand=True)

# --- Sidebar with Treeview ---
sidebar = tk.Frame(content_frame, width=200)
sidebar.pack(side="left", fill="y")

# Dropdown (Selection list)
selection_var = tk.StringVar(value="Option 1")
selection_options = ["Option 1", "Option 2", "Option 3"]

selection_menu = tk.OptionMenu(sidebar, selection_var, *selection_options)
selection_menu.config(relief="flat", highlightthickness=0)
selection_menu.pack(anchor="w", fill="x", padx=5, pady=(5, 0))

# Treeview
tree = ttk.Treeview(sidebar)
tree.pack(fill="both", expand=True, padx=(5, 0), pady=5)

# --- Vertical separator ---
separator = tk.Frame(content_frame, width=2, bg="grey")
separator.pack(side="left", fill="y")

# --- Main Content Area ---
main_content = tk.Frame(content_frame)
main_content.pack(side="left", fill="both", expand=True)

# Label to show selected item
selected_label = tk.Label(main_content, text="Select an item from the tree", font=("Arial", 14))
selected_label.pack(padx=20, pady=20, anchor="nw")

# --- Tree Structure ---
parent1 = tree.insert("", "end", text="Section 1", open=True)
tree.insert(parent1, "end", text="Subsection 1.1")
tree.insert(parent1, "end", text="Subsection 1.2")

parent2 = tree.insert("", "end", text="Section 2", open=False)
tree.insert(parent2, "end", text="Subsection 2.1", )
tree.insert(parent2, "end", text="Subsection 2.2")

tree.insert("", "end", text="Section 3")

# --- Handle tree selection ---
def on_tree_select(event):
    selected_item = tree.focus()
    selected_text = tree.item(selected_item, "text")
    selected_label.config(text=f"You selected: {selected_text}")

tree.bind("<<TreeviewSelect>>", on_tree_select)

root.mainloop()
