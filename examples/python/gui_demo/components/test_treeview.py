import tkinter as tk

from gui_demo.components.generic_editable_treeview import GenericEditableTreeview

root = tk.Tk()
root.title('Treeview in Tk')

content = [
    ['prop 1', 'val 1', 'alt val 0'],
    ['prop 2', 'val 2', 'alt val 1'],
    ['prop 3', 'val 3', 'alt val 0'],
    ['category',
     ['nested prop 1', 'nested val 1', 'alt val 0'],
     ['nested category',
        ['dn 1', 'dn 1', 'alt val 0'],
        ['dn 2', 'dn 2', 'alt val 0'],
     ],
     ['nested prop 2', 'nested val 2', 'alt val 0'],
    ],
    ['prop 5', 'val 5', 'alt val 0'],
    ['prop 6', 'val 6', 'alt val 0'],
]

treeview = GenericEditableTreeview(None, ('Property', 'Value', 'Additional col'), content, 'Value')

treeview.pack()
root.mainloop()
