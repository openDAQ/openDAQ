import tkinter as tk

class SyncGraphWindow(tk.Toplevel):
    # Reference-domain graph: devices grouped by transitive synchronization.
    # Domain ids form the union-find nodes; every device unions its applied
    # source id with its output ids, which is exactly the doc's
    # source -> output topology relation.

    GROUP_COLORS = ('#e8f4e8', '#e8ecf8', '#f8ece8', '#f4e8f4', '#f8f4e0', '#e0f4f4')
    NODE_RADIUS = 18

    def __init__(self, parent, context, entries):
        tk.Toplevel.__init__(self, parent)
        self.title('Reference domain graph')
        scale = context.ui_scaling_factor * context.dpi_factor
        width, height = int(800 * scale), int(550 * scale)
        self.geometry(f'{width}x{height}')

        canvas = tk.Canvas(self, background='white')
        canvas.pack(fill=tk.BOTH, expand=True)
        self._draw(canvas, entries, width, height)

    def _draw(self, canvas, entries, width, height):
        if not entries:
            canvas.create_text(width // 2, height // 2, text='No devices')
            return

        parent = {}

        def find(x):
            parent.setdefault(x, x)
            while parent[x] != x:
                parent[x] = parent[parent[x]]
                x = parent[x]
            return x

        def union(a, b):
            parent[find(a)] = find(b)

        for name, applied, all_ids in entries:
            find(applied)
            for other in all_ids:
                union(applied, other)

        groups = {}
        for name, applied, all_ids in entries:
            groups.setdefault(find(applied), []).append((name, applied))

        # Groups laid out on a grid, devices on a ring around each group hub.
        import math
        count = len(groups)
        cols = max(1, math.ceil(math.sqrt(count)))
        rows = math.ceil(count / cols)
        cell_w, cell_h = width // cols, height // rows

        for index, (root, members) in enumerate(groups.items()):
            cx = (index % cols) * cell_w + cell_w // 2
            cy = (index // cols) * cell_h + cell_h // 2
            color = self.GROUP_COLORS[index % len(self.GROUP_COLORS)]
            ring = min(cell_w, cell_h) // 3

            canvas.create_oval(cx - ring - 40, cy - ring - 40,
                               cx + ring + 40, cy + ring + 40,
                               fill=color, outline='#999999')
            # The group is labeled with its root domain id; with one member
            # and a local id this is just an unsynced standalone device.
            canvas.create_text(cx, cy - ring - 25, text=self._shorten(root),
                               font=('TkDefaultFont', 9, 'bold'))

            for m_index, (name, applied) in enumerate(members):
                angle = 2 * math.pi * m_index / max(1, len(members))
                nx = cx + int(ring * math.cos(angle))
                ny = cy + int(ring * math.sin(angle))
                if len(members) > 1:
                    canvas.create_line(cx, cy, nx, ny, fill='#8899bb')
                r = self.NODE_RADIUS
                canvas.create_oval(nx - r, ny - r, nx + r, ny + r,
                                   fill='#cde8cd', outline='#557755')
                canvas.create_text(nx, ny + r + 10, text=self._shorten(name, 18))
            if len(members) > 1:
                canvas.create_oval(cx - 4, cy - 4, cx + 4, cy + 4,
                                   fill='#334466', outline='')

    def _shorten(self, text, limit=34):
        return text if len(text) <= limit else text[:limit - 3] + '...'