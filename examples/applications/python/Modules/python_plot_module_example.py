##
# Example: Python module that creates a FunctionBlock with one input port.
# When a signal is connected, it opens a live plot and updates it on incoming packets.
##

from __future__ import annotations

import collections
import traceback
from datetime import datetime, timezone
from typing import Deque

import opendaq as daq
import time
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import matplotlib.ticker as mticker


def _numpy_dtype_from_sample_type(sample_type: daq.SampleType):

    mapping = {
        daq.SampleType.Float64: np.float64,
        daq.SampleType.Float32: np.float32,
        daq.SampleType.Int64: np.int64,
        daq.SampleType.Int32: np.int32,
        daq.SampleType.Int16: np.int16,
        daq.SampleType.Int8: np.int8,
        daq.SampleType.UInt64: np.uint64,
        daq.SampleType.UInt32: np.uint32,
        daq.SampleType.UInt16: np.uint16,
        daq.SampleType.UInt8: np.uint8,
    }

    return mapping.get(sample_type, np.float64)


def _coerce_float(v) -> float:
    try:
        return float(v)
    except Exception:
        return 0.0


def _linear_params(desc: daq.IDataDescriptor) -> tuple[float, float]:
    try:
        lin = daq.ILinearDataRule.cast_from(desc.rule)
        return _coerce_float(lin.delta), _coerce_float(lin.start)
    except Exception:
        return 1.0, 0.0


def _fixup_iso8601(epoch: str) -> str:
    # Mirrors `daq::reader::fixupIso8601` (see `core/opendaq/reader/include/opendaq/reader_utils.h`).
    if "T" not in epoch:
        epoch += "T00:00:00+00:00"
    elif epoch.endswith("Z"):
        epoch = epoch[:-1] + "+00:00"
    elif "+" not in epoch:
        epoch += "+00:00"
    return epoch


def _parse_epoch_seconds(origin: str) -> float:
    dt = datetime.fromisoformat(_fixup_iso8601(origin))
    if dt.tzinfo is None:
        dt = dt.replace(tzinfo=timezone.utc)
    return dt.timestamp()


def _ticks_to_epoch_seconds(ddesc: daq.IDataDescriptor, ticks: np.ndarray) -> np.ndarray:
    """
    Match `TimeReaderBase::readSamples` in `core/opendaq/reader/include/opendaq/time_reader.h`:
      output = round_sys_clock(parsedEpoch + (tick * num/den))

    We approximate the rounding by working in float seconds; good enough for plotting.
    """
    try:
        unit = ddesc.unit
        if unit.quantity != "time" or unit.symbol != "s":
            return ticks.astype(np.float64, copy=False)
    except Exception:
        return ticks.astype(np.float64, copy=False)

    origin = ddesc.origin
    if not origin:
        return ticks.astype(np.float64, copy=False)

    epoch_s = _parse_epoch_seconds(str(origin))
    tr = ddesc.tick_resolution
    num = float(tr.numerator)
    den = float(tr.denominator)
    if den == 0.0:
        den = 1.0

    tf = ticks.astype(np.float64, copy=False)
    offset = (tf * num) / den
    return epoch_s + offset


def _domain_x_from_value_packet(value_pkt: daq.IDataPacket, domain_signal_descriptor: daq.IDataDescriptor) -> np.ndarray:
    """
    Build X coordinates for the samples in `value_pkt`.

    Prefer explicit `value_pkt.domain_packet` samples. If absent, fall back to the value packet's
    linear rule + offset (value-domain).
    """
    n = int(value_pkt.sample_count)
    if n <= 0:
        return np.zeros(0, dtype=np.float64)

    domain_pkt = value_pkt.domain_packet
    if domain_pkt is not None and int(domain_pkt.sample_count) > 0:
        ddesc = domain_pkt.data_descriptor
        dtype = _numpy_dtype_from_sample_type(ddesc.sample_type)
        dn = min(n, int(domain_pkt.sample_count))
        # `TimeReader` converts raw tick samples (pre-post-scaling) using origin + tick_resolution.
        # Prefer `raw_data` when present; fall back to `data` if the packet only carries calculated samples.
        buf = domain_pkt.raw_data if int(domain_pkt.raw_data_size) > 0 else domain_pkt.data
        raw = np.frombuffer(buf, dtype=dtype, count=dn)

        # Time domain conversion matches `TimeReader` for tick-based domains.
        if np.issubdtype(raw.dtype, np.datetime64):
            x = raw.astype("datetime64[ns]").astype("int64") / 1e9
        else:
            # Use the domain *signal* descriptor for epoch + tick resolution (matches `TimeReader` expectations).
            x = _ticks_to_epoch_seconds(domain_signal_descriptor, raw)

        if dn < n:
            # If domain packet is shorter for some reason, pad with linear extrapolation using value descriptor.
            vdesc = value_pkt.data_descriptor
            delta, start = _linear_params(vdesc)
            base = _coerce_float(value_pkt.offset)
            idx = start + delta * np.arange(n, dtype=np.float64)
            ticks_full = base + idx
            x_full = _ticks_to_epoch_seconds(domain_signal_descriptor, ticks_full)
            x_full[:dn] = x
            x = x_full
        return x

    vdesc = value_pkt.data_descriptor
    delta, start = _linear_params(vdesc)
    base = _coerce_float(value_pkt.offset)
    idx = start + delta * np.arange(n, dtype=np.float64)
    ticks = base + idx
    return _ticks_to_epoch_seconds(domain_signal_descriptor, ticks)


class PlotSignalFunctionBlock(daq.FunctionBlock):
    TYPE_ID = "py_fb_plot_signal_uid"

    @staticmethod
    def create_function_block_type() -> daq.IFunctionBlockType:
        default_config = daq.PropertyObject()
        return daq.FunctionBlockType(
            PlotSignalFunctionBlock.TYPE_ID,
            "py_fb_plot_signal",
            "Python function block that plots a connected signal",
            default_config,
        )

    def __init__(self, context: daq.IContext = None, parent: daq.IComponent = None, local_id: str = None):
        super().__init__(context=context, parent=parent, local_id=local_id)

        # Rolling buffer of the last N samples for plotting.
        self._max_points = 5000
        self._times: Deque[float] = collections.deque(maxlen=self._max_points)
        self._samples: Deque[float] = collections.deque(maxlen=self._max_points)
        self._domain_signal_descriptor: daq.IDataDescriptor | None = None
        self._time_axis_configured = False

        self._plot_enabled = False
        self._fig = None
        self._ax = None
        self._line = None
        self._plot_error_logged = False

        self._input_port = None

    def on_init(self):
        # Called by the C++ wrapper after it sets `self._cpp_fb`.
        # Create one input port, packet notifications on the same thread for simplicity.
        self._input_port = self.create_and_add_input_port(
            local_id="in",
            notification_method=daq.PacketReadyNotification.SameThread,
        )

    def on_connected(self, port: daq.IInputPort):
        signal = port.signal
        descriptor = signal.descriptor
        if descriptor is None:
            self._plot_enabled = False
            raise Exception("PlotSignalFunctionBlock: descriptor not available")
        
        try:
            _numpy_dtype_from_sample_type(signal.descriptor.sample_type)
        except Exception as e:
            self._plot_enabled = False
            raise Exception(f"PlotSignalFunctionBlock: sample type not supported: {e}")

        domain_signal = signal.domain_signal
        if domain_signal is None:
            self._plot_enabled = False
            raise Exception("PlotSignalFunctionBlock: domain signal not available")

        domain_descriptor = domain_signal.descriptor
        if domain_descriptor is None:
            self._plot_enabled = False
            raise Exception("PlotSignalFunctionBlock: domain descriptor not available")

        self._domain_signal_descriptor = domain_descriptor
        self._time_axis_configured = False

        self._plot_enabled = True
        plt.ion()
        self._fig, self._ax = plt.subplots()
        (self._line,) = self._ax.plot([], [], lw=1)
        try:
            origin = str(domain_descriptor.origin)
        except Exception:
            origin = ""
        self._ax.set_title(f"PlotSignalFunctionBlock: live signal (origin={origin})")
        # X axis will be configured once we have data: unix seconds -> matplotlib dates.
        self._ax.set_xlabel("time")
        self._ax.set_ylabel("value")
        self._fig.canvas.draw_idle()
        self._fig.canvas.flush_events()

        print("PlotSignalFunctionBlock connected:", port)

    def on_disconnected(self, port: daq.IInputPort):
        self._time_axis_configured = False
        print("PlotSignalFunctionBlock disconnected:", port)

    def on_packet_received(self, port: daq.IInputPort):
        # Drain all available packets from the connection queue.
        conn = port.connection
        if conn is None:
            return

        while conn.packet_count > 0:
            pkt = conn.dequeue()
            if pkt is None:
                break

            if getattr(pkt, "type", None) != daq.PacketType.Data:
                continue

            try:
                data_pkt = daq.IDataPacket.cast_from(pkt)
            except Exception:
                continue

            if data_pkt.sample_count == 0:
                continue

            n = int(data_pkt.sample_count)

            vdesc = data_pkt.data_descriptor
            vdtype = _numpy_dtype_from_sample_type(vdesc.sample_type)
            y = np.frombuffer(data_pkt.data, dtype=vdtype, count=n).astype(np.float64, copy=False)

            if self._domain_signal_descriptor is None:
                continue

            t = _domain_x_from_value_packet(data_pkt, self._domain_signal_descriptor)
            m = min(int(y.size), int(t.size))
            y = y[:m]
            t = t[:m]
            if m == 0:
                continue

            self._times.extend(t)
            self._samples.extend(y)

        if not self._plot_enabled or self._ax is None or self._line is None:
            return

        if not self._samples or not self._times:
            return

        # Update plot with the rolling buffer.
        try:

            y = np.fromiter(self._samples, dtype=np.float64)
            x = np.fromiter(self._times, dtype=np.float64)

            # `x` is absolute unix seconds consistent with `TimeReader` (epoch(origin) + tick*resolution).
            # Avoid `mdates.epoch2num` (not available in all Matplotlib versions); `date2num` accepts numpy datetimes.
            x_dt = mdates.date2num((x.astype(np.float64, copy=False) * 1e9).astype("datetime64[ns]"))

            if not self._time_axis_configured:
                def _format_ts(x, pos=None):
                    # matplotlib date numbers -> UTC wall time with milliseconds
                    dt = mdates.num2date(x)
                    try:
                        # matplotlib may return timezone-aware datetime depending on version/settings
                        if dt.tzinfo is not None:
                            dt = dt.astimezone(timezone.utc).replace(tzinfo=None)
                    except Exception:
                        dt = dt.replace(tzinfo=None)

                    ms = int(dt.microsecond // 1000)
                    return f"{dt:%Y-%m-%d} {dt:%H:%M:%S}.{ms:03d}"

                self._ax.xaxis.set_major_formatter(mticker.FuncFormatter(_format_ts))
                self._ax.xaxis.set_minor_locator(mticker.NullLocator())
                # Do not use `autofmt_xdate()` — it rotates X labels; keep them horizontal.
                self._fig.subplots_adjust(bottom=0.22)
                self._time_axis_configured = True

            self._line.set_data(x_dt, y)

            x_min = float(np.min(x_dt))
            x_max = float(np.max(x_dt))
            if x_min == x_max:
                x_min -= 1e-6 / 86400.0  # ~1us in matplotlib date units
                x_max += 1e-6 / 86400.0
            self._ax.set_xlim(x_min, x_max)
            x_mid = 0.5 * (x_min + x_max)
            self._ax.set_xticks([x_min, x_mid, x_max])
            self._ax.tick_params(axis="x", labelrotation=0)
            for tick in self._ax.xaxis.get_major_ticks():
                tick.label1.set_horizontalalignment("center")
            y_min = float(np.min(y))
            y_max = float(np.max(y))
            if y_min == y_max:
                y_min -= 1.0
                y_max += 1.0
            self._ax.set_ylim(y_min, y_max)
            self._fig.canvas.draw_idle()
            self._fig.canvas.flush_events()
        except Exception:
            # Never let plotting errors break the data path.
            if not self._plot_error_logged:
                self._plot_error_logged = True
                traceback.print_exc()
            return


class PlotModule(daq.Module):
    def __init__(self, context=None):
        super().__init__(
            context=context,
            name="PlotPythonModule",
            version=(1, 0, 0),
            id="PlotPythonModule",
        )

    def on_get_available_function_block_types(self):
        fb_type = PlotSignalFunctionBlock.create_function_block_type()
        return {PlotSignalFunctionBlock.TYPE_ID: fb_type}

    def on_create_function_block(self, id, parent, local_id, config):
        if id != PlotSignalFunctionBlock.TYPE_ID:
            return None
        return PlotSignalFunctionBlock(self.context, parent, local_id)


def main() -> int:
    # Note: if you want to use the default openDAQ module discovery, remove module_path override.
    builder = daq.InstanceBuilder()
    instance = builder.build()

    instance.module_manager.add_module(PlotModule(instance.context))

    print("Adding function block:", PlotSignalFunctionBlock.TYPE_ID)
    fb = instance.add_function_block(PlotSignalFunctionBlock.TYPE_ID)

    # Connect a reference device signal to our input port (so the plot starts moving).
    device = instance.add_device("daqref://device0")
    signal = device.channels_recursive[0].signals[0]
    fb.input_ports[0].connect(signal)

    print("Connected. Close the plot window to exit.")
    try:

        # Python-implemented FB notifications may arrive from native worker threads.
        # They are marshalled into `opendaq.event_queue` and must be pumped on the main thread.
        plt.ion()
        daq.event_queue.process_events()
        fig = plt.gcf()

        while plt.fignum_exists(fig.number):
            daq.event_queue.process_events()
            plt.pause(0.01)
    except Exception:
        print("Error in main loop:", traceback.format_exc())


    return 0


if __name__ == "__main__":
    raise SystemExit(main())

