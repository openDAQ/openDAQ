import opendaq
import pydot


class ConnectionMap:
    def __init__(self, dev, edge_color="orange", node_color="#A0A0A0", font_color="white", cluster_color="#A0A0A0",
                 edge_fontsize="15", node_fontsize="18", additional_properties=False, attributes_show=False,
                 input_ports=False, show_disconnected_signals=False, show_connections_only=False):
        self.additional_properties = additional_properties
        self.attributes_show = attributes_show
        self.input_ports = input_ports
        self.show_disconnected_signals = show_disconnected_signals
        self.show_connections_only = show_connections_only
        self.Edge_color = edge_color
        self.Node_color = node_color
        self.Font_color = font_color
        self.Cluster_color = cluster_color
        self.Edge_fontsize = edge_fontsize
        self.Node_fontsize = node_fontsize
        self.devs = dev
        self.draw_connection_map()

    def draw_connection_map(self):
        graph = pydot.Dot(graph_type='digraph', rankdir="LR", splines="ortho", compound="true")
        for device in self.devs:
            if not opendaq.IDevice.can_cast_from(device):
                print(f"Device {device} cannot be cast to IDevice.")
                continue
            self.process_device_recursively(device, graph)
        graph.write('connection_map.png', format='png')
        graph.write('connection_map.svg', format='svg')

    def process_device_recursively(self, device, graph, parent_cluster=None, depth=0):
        if not opendaq.IDevice.can_cast_from(device):
            raise RuntimeError("Invalid cast. The object does not implement this interface.")
        device = opendaq.IDevice.cast_from(device)
        device_id = self.fixup_id(device.global_id)
        darker_color = self.adjust_color(self.Cluster_color, depth)
        device_cluster = pydot.Cluster(device_id, label=device.name, style="filled", fillcolor=darker_color, pad="2",
                                       fontname="Courier New Bold", fontsize="20")
        if parent_cluster:
            parent_cluster.add_subgraph(device_cluster)
        else:
            graph.add_subgraph(device_cluster)
        connected = self.get_connected_nodes(device)
        for chan in list(device.channels) + list(device.function_blocks):
            if self.show_connections_only and chan.global_id not in connected:
                continue
            darker_color = self.adjust_color(self.Cluster_color, depth + 1)
            chan_cluster = pydot.Cluster(self.fixup_id(chan.global_id), label=chan.name, style="filled",
                                         fillcolor=darker_color, pad="2", fontname="Courier New Bold", fontsize="20")
            chan_label = f'<table border="0" cellborder="1" cellspacing="5" cellpadding="10" color="black">'
            chan_label += f'<tr><td border="0"> </td></tr>'
            if chan_cluster.get_name().split('_')[-1] != self.fixup_id(chan.global_id):
                chan_label += f'<tr><td border="0">{chan.name}</td></tr>'
            else:
                chan_label += f'<tr><td border="0">{chan.name}</td></tr>'
            if self.additional_properties:
                chan_label += f'<tr><td border="0">{chan.additional_properties}</td></tr>'
            if self.attributes_show:
                chan_label += f'<tr><td border="0">{chan.attributes_show}</td></tr>'
            chan_label += '</table>'
            chan_node = pydot.Node(self.fixup_id(chan.global_id), label=f'<{chan_label}>', shape="box",
                                   fontname="Courier New Bold", fontsize="20")
            chan_cluster.add_node(chan_node)
            device_cluster.add_subgraph(chan_cluster)
            if chan.function_blocks:
                for sub_fb in chan.function_blocks:
                    self.process_device_recursively(sub_fb, graph, chan_cluster, depth + 1)
        for subdevice in device.devices:
            self.process_device_recursively(subdevice, graph, device_cluster, depth + 1)
        self.add_connections(device, graph, list(self.devs) + [device], device_cluster)

    def adjust_color(self, color, depth):
        if not color or not color.startswith('#') or len(color) != 7:
            color = '#A0A0A0'
        base = int(color[1:], 16)
        multiplier = max(0, 1 - (0.2 * depth))
        r = max(0, int(((base >> 16) & 0xFF) * multiplier))
        g = max(0, int(((base >> 8) & 0xFF) * multiplier))
        b = max(0, int((base & 0xFF) * multiplier))
        return f'#{r:02x}{g:02x}{b:02x}'

    def fixup_id(self, id):
        return id.replace(':', '').replace('-', '').replace('/', '_').replace('_', '').replace('edge', 'dey')

    def get_attributes(self, entity):
        return {
            'Name': entity.name,
            'Description': entity.description,
            'Active': str(entity.active),
            'Global_ID': entity.global_id,
            'Local_id': entity.local_id,
            'Tags': str(entity.tags.list).replace("[]", ""),
            'Visible': str(entity.visible)
        }

    def get_FB_from_signal_global_id(self, globalid):
        globalid = globalid.split('/')
        return self.fixup_id('/'.join(globalid[:globalid.index('FB') + 2]))

    def get_signal_connections(self, signal):
        return [(self.get_FB_from_signal_global_id(conn.input_port.global_id), conn.input_port.name) for conn in
                signal.connections]

    def add_connections(self, device, graph, devices, device_cluster):
        signal_destinations = {}
        midway_nodes_tracker = {}

        def process_entity(entity):
            for sig in entity.signals:
                destinations = self.get_signal_connections(sig)
                signal_key = (self.fixup_id(entity.global_id), sig.name)
                if len(destinations) > 1 and signal_key not in midway_nodes_tracker:
                    signal_destinations.setdefault(signal_key, []).extend(destinations)
                    midway_nodes_tracker[signal_key] = True
            for fb in entity.function_blocks:
                process_entity(fb)

        for ent in list(device.channels) + list(device.function_blocks):
            process_entity(ent)

        for (source_id, sig_name), destinations in signal_destinations.items():
            midway_node_id = f"midway_{source_id}_{sig_name}"
            midway_node = pydot.Node(midway_node_id, label="", shape="none", height="0", width="0", margin="0")
            device_cluster.add_node(midway_node)
            graph.add_edge(
                pydot.Edge(source_id, midway_node_id, xlabel=sig_name, arrowhead="odot", color=self.Edge_color,
                           fontcolor=self.Font_color, fontname="Courier New Bold", fontsize=self.Edge_fontsize))
            for dest, port in destinations:
                if source_id != dest:
                    portlabel = f'{port}' if self.input_ports else ""
                    graph.add_edge(pydot.Edge(midway_node_id, dest, xlabel=sig_name, fontsize=self.Edge_fontsize,
                                              arrowhead="obox", tailport="e", headport="w", labelfloat="true",
                                              labeldistance="0", minlen="1", color=self.Edge_color, tailclip="true",
                                              headclip="true", lhead="cluster_" + dest, ltail="cluster_" + source_id,
                                              headlabel=portlabel, fontname="Courier New Bold",
                                              fontcolor=self.Font_color))

        def process_entity_signals(entity):
            for sig in entity.signals:
                signal_key = (self.fixup_id(sig.global_id), sig.name)
                destinations = self.get_signal_connections(sig)
                if not destinations and self.show_disconnected_signals and not self.show_connections_only:
                    invis_node_id = f"invis_{self.fixup_id(entity.global_id)}_{sig.name}"
                    invis_node = pydot.Node(invis_node_id, label="", shape="point", style="invis")
                    device_cluster.add_node(invis_node)
                    graph.add_edge(
                        pydot.Edge(self.fixup_id(entity.global_id), invis_node_id, xlabel=self.fixup_id(sig.name),
                                   arrowhead="odot", color=self.Edge_color, fontname="Courier New Bold",
                                   fontsize=self.Edge_fontsize, fontcolor=self.Font_color, minlen="1"))
                else:
                    if len(destinations) == 1 and signal_key not in midway_nodes_tracker:
                        for dest, port in destinations:
                            if self.fixup_id(entity.global_id) != dest:
                                portlabel = f'{port}' if self.input_ports else ""
                                graph.add_edge(
                                    pydot.Edge(self.fixup_id(entity.global_id), dest, fontsize=self.Edge_fontsize,
                                               arrowhead="obox", tailport="e", headport="w",
                                               labelfloat="true",
                                               labeldistance="0", minlen="1.5", xlabel=self.fixup_id(sig.name),
                                               color=self.Edge_color, tailclip="true", headclip="true",
                                               headlabel=portlabel, lhead="cluster_" + dest,
                                               ltail="cluster_" + self.fixup_id(entity.global_id),
                                               fontname="Courier New Bold", fontcolor=self.Font_color))
            for fb in entity.function_blocks:
                process_entity_signals(fb)

        for ent in list(device.channels) + list(device.function_blocks):
            process_entity_signals(ent)

    def add_entity(self, entity, graph, parent_cluster, depth):
        darker_color = self.adjust_color(self.Cluster_color, depth)
        entity = opendaq.IFunctionBlock.cast_from(entity)
        entity_id = self.fixup_id(entity.global_id)
        entity_cluster = pydot.Cluster(entity_id, label=entity.name, style="filled", fillcolor=darker_color, pad="2",
                                       fontname="Courier New Bold", fontsize="20")
        if not entity.function_blocks:
            entity_cluster = parent_cluster
        else:
            parent_cluster.add_subgraph(entity_cluster)
        darker_color = self.adjust_color(self.Cluster_color, depth + 1)
        chan_cluster = pydot.Cluster(self.fixup_id(entity.global_id), label=entity.name, style="filled",
                                     fillcolor=darker_color,
                                     pad="2", fontname="Courier New Italic", fontsize="20")
        chan_label = (f'<table border="0" cellborder="1" cellspacing="5" cellpadding="10">'
                      f'<tr><td colspan="2" align="center" border="0" padding-bottom="5">'
                      f'<font point-size="25" color="orange">{entity.name}</font></td></tr>')
        if self.additional_properties:
            chan_label += f'<tr><td align="center"><font point-size="20">Properties</font></td><td><font point-size="20">Value</font></td></tr>'
            for prop in entity.visible_properties:
                chan_label += f'<tr><td>{prop.name}</td><td>{entity.get_property_value(prop.name)}</td></tr>'
        if self.attributes_show:
            chan_label += f'<tr><td align="center"><font point-size="20">Attributes</font></td><td><font point-size="20">Value</font></td></tr>'
            for attr in self.get_attributes(entity):
                chan_label += f'<tr><td>{attr}</td><td>{self.get_attributes(entity)[attr]}</td></tr>'
        chan_label += '</table>'
        chan_node = pydot.Node(self.fixup_id(entity.global_id), label=f'<{chan_label}>', shape="box",
                               style="filled, rounded", fillcolor=self.Node_color, padding="2", height="1",
                               fontcolor=self.Font_color,
                               fontname="Courier New Bold")
        entity_cluster.add_node(chan_node)
        if entity.function_blocks:
            for fb in entity.function_blocks:
                self.add_entity(fb, graph, chan_cluster, depth + 2)

    def get_connected_nodes(self, device):
        connected = []
        device = opendaq.IDevice.cast_from(device)

        def process_entity(entity, depth=0):
            for sig in entity.signals:
                for conn in sig.connections:
                    ap = conn.signal.global_id.split('/')
                    ap = '/'.join(ap[:-(depth + 2)]) if depth != 0 else '/'.join(ap[:-2])
                    connected.append(ap)
                    connected.append(conn.input_port.parent.parent.global_id)
            for fb in entity.function_blocks:
                process_entity(fb, depth + 2)

        for chan in list(device.channels) + list(device.function_blocks):
            process_entity(chan)
        for dev in device.devices:
            connected.extend(self.get_connected_nodes(dev))
        return connected

    def process_device_recursively(self, device, graph, parent_cluster=None, depth=0):
        device = opendaq.IDevice.cast_from(device)
        device_id = self.fixup_id(device.global_id)
        darker_color = self.adjust_color(self.Cluster_color, depth)
        device_cluster = pydot.Cluster(device_id, label=device.name, style="filled", fillcolor=darker_color, pad="2",
                                       fontname="Courier New Bold", fontsize="20")
        if parent_cluster:
            parent_cluster.add_subgraph(device_cluster)
        else:
            graph.add_subgraph(device_cluster)
        connected = self.get_connected_nodes(device)
        for chan in list(device.channels) + list(device.function_blocks):
            if self.show_connections_only and chan.global_id not in connected:
                continue
            darker_color = self.adjust_color(self.Cluster_color, depth + 1)
            chan_cluster = pydot.Cluster(self.fixup_id(chan.global_id), label=chan.name, style="filled",
                                         fillcolor=darker_color,
                                         pad="2", fontname="Courier New Bold", fontsize="25", fontcolor="black")
            chan_label = f'<table border="0" cellborder="1" cellspacing="5" cellpadding="10" color="black">'
            chan_label += f'<tr><td border="0"> </td></tr>'
            if chan_cluster.get_name().split('_')[-1] != self.fixup_id(chan.global_id):
                chan_label += f'<tr><td colspan="2" align="center" border="0" padding-bottom="5">'
                chan_label += f'<font point-size="25" color="orange">{chan.name}</font></td></tr>'
            else:
                chan_cluster.set("fontcolor", "orange")
            if self.additional_properties:
                chan_label += f'<tr><td align="center"><font point-size="20">Properties</font></td><td><font point-size="20">Value</font></td></tr>'
                for prop in chan.visible_properties:
                    chan_label += f'<tr><td>{prop.name}</td><td>{chan.get_property_value(prop.name)}</td></tr>'
            if self.attributes_show:
                chan_label += f'<tr><td align="center"><font point-size="20">Attributes</font></td><td><font point-size="20">Value</font></td></tr>'
                for attr in self.get_attributes(chan):
                    chan_label += f'<tr><td>{attr}</td><td>{self.get_attributes(chan)[attr]}</td></tr>'
            chan_label += '</table>'
            chan_node = pydot.Node(self.fixup_id(chan.global_id), label=f'<{chan_label}>', shape="box",
                                   style="filled, rounded",
                                   fillcolor=darker_color, padding="0", fontcolor=self.Font_color,
                                   fontname="Courier New Bold", color=darker_color)
            chan_cluster.add_node(chan_node)
            device_cluster.add_subgraph(chan_cluster)
            if chan.function_blocks:
                for fb in chan.function_blocks:
                    self.add_entity(fb, graph, chan_cluster, depth + 2)
        for subdevice in device.devices:
            self.process_device_recursively(subdevice, graph, device_cluster, depth + 1)
        self.add_connections(device, graph, list(self.devs) + [device], device_cluster)