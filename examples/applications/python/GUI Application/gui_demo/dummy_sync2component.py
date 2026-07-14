##
# Pseudo-code stubs for the reworked sync API (ISynchronization,
# ISyncInterface, IReferenceDomainInfo); bodies return plausible values for a
# connected Raspberry Pi.
#
# Synchronization holds the sync-service state; ReferenceDomainInfo the state
# applied to clock and data (no lag in the stub). ClockSyncInterface is the
# default source. on_property_value_changed is a stub stand-in for
# subscribing to property-value changes on the real property objects.
#
# Everything stub-only is grouped under interface.simulate; nothing found
# there exists on a real device.
##

class SyncMode:
    Off = 0     # Interface is disabled.
    Input = 2   # Interface can only receive a synchronization reference.
    Output = 4  # Interface can only distribute the device clock.
    Auto = 8    # Interface automatically selects its active role.

MODE_NAMES = {
    SyncMode.Off: "Off",
    SyncMode.Input: "Input",
    SyncMode.Output: "Output",
    SyncMode.Auto: "Auto",
}

class TimeProtocol:
    Unknown = 0
    Tai = 1
    Gps = 2
    Utc = 3

class SourceStatus:
    Off = "Off"                  # The interface mode is Off.
    Listening = "Listening"      # Waiting for a source or role negotiation.
    Calibrating = "Calibrating"  # Source found, not yet applied to the clock.
    Synced = "Synced"            # Applied to the clock, or actively outputting.
    Error = "Error"              # Protocol, network, or device error.
    Unknown = "Unknown"          # State cannot currently be determined.

class RoleStatus:
    Off = "Off"
    Input = "Input"
    Output = "Output"
    Unknown = "Unknown"          # Role negotiation still in progress.

class SyncStatusContainer:
    """Read-only status view; the values come from the owning interface."""

    def __init__(self, interface):
        self._interface = interface

    def get_status(self, name):
        if name == "SourceStatus":
            return self._interface._derive_source_status()
        if name == "RoleStatus":
            return self._interface._derive_role_status()
        return SourceStatus.Unknown

    def get_status_message(self, name):
        if name == "SourceStatus":
            return self._interface._derive_source_message()
        return ""

class ReferenceDomainInfo:
    """Read-only info about the sync state already applied to the device
    clock or a domain signal. On the real device this lives on the device
    domain (IDeviceDomain) and in each domain signal's data descriptor."""

    def __init__(self, source_reference_domain_id="", reference_domain_ids=None,
                 offset=0, time_protocol=TimeProtocol.Unknown):
        self._source_reference_domain_id = source_reference_domain_id
        self._reference_domain_ids = reference_domain_ids or []
        self._offset = offset
        self._time_protocol = time_protocol

    def get_source_reference_domain_id(self):
        """Id of the source applied to the clock; the device's local id when
        no external source is applied."""
        return self._source_reference_domain_id

    def get_reference_domain_ids(self):
        """The applied source id plus the ids of active output domains
        produced from this clock. Use this list to determine whether two
        devices or signals share a reference domain."""
        return list(self._reference_domain_ids)

    def get_reference_domain_offset(self):
        """Cumulative offset in device-domain ticks; 0 on boot, adjusted
        whenever a resynchronization causes a clock discontinuity."""
        return self._offset

    def get_reference_time_protocol(self):
        """Protocol used to interpret timestamps (TAI, GPS, or UTC)."""
        return self._time_protocol

class SyncInterfaceSimulator:
    """Stub-only simulation knobs, exposed as interface.simulate. Nothing
    here exists on a real device; it stands in for the network and protocol
    state the sync service observes by itself."""

    def __init__(self, upstream_reachable=False):
        # Whether the upstream time source is answering (PTP announces
        # arriving, NTP peers responding). Read by the owning interface.
        self.upstream_reachable = upstream_reachable
        # Negotiated role while mode is Auto; Unknown during negotiation.
        self.auto_role = RoleStatus.Input

    def upstream_loss(self):
        """The upstream time source becomes unreachable, as if the device
        lost network connectivity to the NTP server / PTP master."""
        self.upstream_reachable = False

    def upstream_restored(self):
        """Connectivity to the upstream time source recovers."""
        self.upstream_reachable = True

class SyncInterface:
    """One synchronization interface on a device (ISyncInterface): mode,
    reference-domain id, statuses, and property-change notifications.
    Stub-only controls live under .simulate."""

    def __init__(self, name, available_modes, time_protocol=TimeProtocol.Unknown,
                 upstream_domain_id="", upstream_reachable=False,
                 output_domain_id=""):
        self._name = name
        self._available_modes = list(available_modes)
        self._mode = SyncMode.Off
        self._output_only = False
        self._time_protocol = time_protocol
        # Canonical id of the upstream source observed in Input role (e.g. a
        # PTP grandmaster); known only while the upstream is reachable.
        self._upstream_domain_id = upstream_domain_id
        # Canonical id of the domain this interface produces in Output role.
        self._output_domain_id = output_domain_id
        self._is_source = False
        self._status_container = SyncStatusContainer(self)
        # Handlers subscribed through on_property_value_changed.
        self._property_changed_handlers = []
        # Stub-only knobs; nothing under .simulate exists on a real device.
        self.simulate = SyncInterfaceSimulator(upstream_reachable)

    @property
    def name(self):
        return self._name

    def get_mode(self):
        return self._mode

    def get_available_modes(self):
        return list(self._available_modes)

    def get_reference_domain_id(self):
        """Id currently observed (input role) or produced (output role);
        empty when no identity is available."""
        role = self._derive_role_status()
        if role == RoleStatus.Input and self.simulate.upstream_reachable:
            return self._get_upstream_domain_id()
        if role == RoleStatus.Output:
            return self._get_output_domain_id()
        return ""

    def set_output_only(self, output_only):
        """While output-only, the interface cannot be selected as the source."""
        if output_only:
            if self._is_source:
                raise ValueError(f"{self._name} is the selected source")
            self._set_mode(SyncMode.Output)
        else:
            # Back to Off; eligible for source selection again.
            self._set_mode(SyncMode.Off)
        self._output_only = output_only

    def get_output_only(self):
        return self._output_only

    def get_status_container(self):
        return self._status_container

    def on_property_value_changed(self, handler):
        """Stub stand-in for subscribing to the property object's
        value-change events on the real device. The handler is called as
        handler(sender, property_name, value) after a change is applied."""
        self._property_changed_handlers.append(handler)

    def _get_upstream_domain_id(self):
        """Identification of the time server this device is synced to,
        observed in Input role; empty when no upstream identity is known."""
        return self._upstream_domain_id

    def _get_output_domain_id(self):
        """Identification of this device as a time server to other devices,
        produced in Output role."""
        return self._output_domain_id

    def _notify_property_value_changed(self, property_name, value):
        for handler in self._property_changed_handlers:
            handler(self, property_name, value)

    def _set_mode(self, mode):
        """Internal (ISyncInterfaceInternal); used by Synchronization for
        source selection."""
        if mode not in self._available_modes:
            raise ValueError(f"{self._name} does not support mode {MODE_NAMES[mode]}")
        if self._mode == mode:
            return
        self._mode = mode
        self._notify_property_value_changed("Mode", MODE_NAMES[mode])

    def _derive_role_status(self):
        if self._mode == SyncMode.Off:
            return RoleStatus.Off
        if self._mode == SyncMode.Input:
            return RoleStatus.Input
        if self._mode == SyncMode.Output:
            return RoleStatus.Output
        return self.simulate.auto_role

    def _derive_source_status(self):
        # The stub jumps straight to Synced; a real device passes through
        # Listening and Calibrating while settling.
        if self._mode == SyncMode.Off:
            return SourceStatus.Off
        role = self._derive_role_status()
        if role == RoleStatus.Unknown:
            return SourceStatus.Listening
        if role == RoleStatus.Output:
            return SourceStatus.Synced
        if not self._get_upstream_domain_id():
            return SourceStatus.Listening
        if not self.simulate.upstream_reachable:
            return SourceStatus.Error
        return SourceStatus.Synced

    def _derive_source_message(self):
        if self._derive_source_status() == SourceStatus.Error:
            return "upstream time source unreachable"
        return ""

class PtpTransport:
    """How PTP messages are carried on the wire."""
    L2 = "L2"            # IEEE 802.3 / Ethernet (Layer 2)
    UDP_IPV4 = "UDPv4"   # UDP over IPv4
    UDP_IPV6 = "UDPv6"   # UDP over IPv6

class PtpDelayMechanism:
    """How link delay is measured."""
    E2E = "E2E"  # end-to-end (delay request/response with the master)
    P2P = "P2P"  # peer-to-peer (peer delay with the neighbour)

class PtpSyncInterface(SyncInterface):
    """A concrete PTP (IEEE 1588) sync interface. Extends SyncInterface with
    the standard PTP configuration settings, exposed as properties: assign to
    set, read to get (ptp.domain_number = 0 / x = ptp.domain_number)."""

    def __init__(self, name="PtpSyncInterface",
                 upstream_domain_id="ptp:default:0:001122fffe334455",
                 time_protocol=TimeProtocol.Utc, upstream_reachable=False):
        super().__init__(
            name,
            available_modes=[SyncMode.Off, SyncMode.Input, SyncMode.Output, SyncMode.Auto],
            time_protocol=time_protocol,
            upstream_domain_id=upstream_domain_id,
            upstream_reachable=upstream_reachable)

        # The device's own PTP clock identity, used in the produced domain id.
        self._clock_identity = "2ccf67fffedc684d"
        self._domain_number = 0
        self._transport = PtpTransport.L2
        self._delay_mechanism = PtpDelayMechanism.E2E
        self._log_sync_interval = 1
        self._log_announce_interval = 1
        self._priority1 = 128
        self._priority2 = 128

    def _get_output_domain_id(self):
        """Canonical form: ptp:<profile-id>:<domain-number>:<clock-identity>."""
        return f"ptp:default:{self._domain_number}:{self._clock_identity}"

    @property
    def domain_number(self):
        """PTP domain number (PTPv2 supports 0-255). Clocks only synchronise
        with clocks in the same domain, so this partitions independent PTP
        networks on one LAN."""
        return self._domain_number

    @domain_number.setter
    def domain_number(self, value):
        if not 0 <= value <= 255:
            raise ValueError("PTP domain number must be 0-255")
        self._domain_number = value

    @property
    def transport(self):
        """Transport carrying PTP messages (L2 Ethernet, UDP/IPv4, UDP/IPv6)."""
        return self._transport

    @transport.setter
    def transport(self, value):
        self._transport = value

    @property
    def delay_mechanism(self):
        """Delay measurement (E2E or P2P). Must match across the network;
        mixing the two breaks delay correction."""
        return self._delay_mechanism

    @delay_mechanism.setter
    def delay_mechanism(self, value):
        self._delay_mechanism = value

    @property
    def log_sync_interval(self):
        """log2 of the Sync message interval in seconds; the default is 1
        (2^1 = 2 s). Smaller = more frequent = tighter sync, more traffic
        (-3 = 0.125 s, 8/s)."""
        return self._log_sync_interval

    @log_sync_interval.setter
    def log_sync_interval(self, value):
        self._log_sync_interval = value

    @property
    def log_announce_interval(self):
        """log2 of the Announce message interval, used by master election
        (BMCA). Usually slower than Sync; 1 = 2 s."""
        return self._log_announce_interval

    @log_announce_interval.setter
    def log_announce_interval(self, value):
        self._log_announce_interval = value

    @property
    def priority1(self):
        """priority1 (0-255, lower wins): primary BMCA tie-breaker. Forces or
        forbids becoming grandmaster regardless of clock quality."""
        return self._priority1

    @priority1.setter
    def priority1(self, value):
        if not 0 <= value <= 255:
            raise ValueError("PTP priority1 must be 0-255")
        self._priority1 = value

    @property
    def priority2(self):
        """priority2 (0-255, lower wins): secondary tie-breaker, used when
        clock class and priority1 are equal."""
        return self._priority2

    @priority2.setter
    def priority2(self, value):
        if not 0 <= value <= 255:
            raise ValueError("PTP priority2 must be 0-255")
        self._priority2 = value

class NtpSyncInterface(SyncInterface):
    """A concrete NTP sync interface. Extends SyncInterface with the standard
    NTP client settings, exposed as properties: assign to set, read to get
    (ntp.min_poll = 6 / x = ntp.min_poll)."""

    def __init__(self, name="NtpSyncInterface",
                 time_protocol=TimeProtocol.Utc, upstream_reachable=False):
        super().__init__(
            name,
            available_modes=[SyncMode.Off, SyncMode.Input],
            time_protocol=time_protocol,
            upstream_reachable=upstream_reachable)

        self._servers = ["pool.ntp.org"]
        self._min_poll = 6
        self._max_poll = 10
        self._iburst = True
        self._version = 4
        self._domain_id = ""

    def _get_upstream_domain_id(self):
        """Canonical form: ntp:domain:<configured-domain-id> when a domain id
        is configured, otherwise ntp:peer:<selected-peer-identity>."""
        if self._domain_id:
            return f"ntp:domain:{self._domain_id}"
        return f"ntp:peer:{self._servers[0]}"

    @property
    def domain_id(self):
        """Configured reference-domain identity. All devices synced to the
        same NTP service must use the same id. When empty, the id of the
        selected peer is used instead."""
        return self._domain_id

    @domain_id.setter
    def domain_id(self, value):
        self._domain_id = value

    @property
    def servers(self):
        """NTP servers to request time from, in order of preference.
        Hostnames or IP addresses; a pool name resolves to multiple
        servers."""
        return self._servers

    @servers.setter
    def servers(self, value):
        if not value:
            raise ValueError("At least one NTP server is required")
        self._servers = list(value)

    @property
    def min_poll(self):
        """Shortest polling interval as log2 seconds (6 = 2^6 = 64 s). Used
        while the clock is still settling."""
        return self._min_poll

    @min_poll.setter
    def min_poll(self, value):
        if not 4 <= value <= 17:
            raise ValueError("NTP minPoll must be 4-17")
        self._min_poll = value

    @property
    def max_poll(self):
        """Longest polling interval as log2 seconds (10 = 2^10 = ~17 min).
        The client backs off from minPoll towards maxPoll as the clock
        settles."""
        return self._max_poll

    @max_poll.setter
    def max_poll(self, value):
        if not 4 <= value <= 17:
            raise ValueError("NTP maxPoll must be 4-17")
        self._max_poll = value

    @property
    def iburst(self):
        """Send a burst of requests when a server is first reached, so
        initial synchronization takes seconds instead of several polling
        intervals."""
        return self._iburst

    @iburst.setter
    def iburst(self, value):
        self._iburst = bool(value)

    @property
    def version(self):
        """NTP protocol version spoken to the servers (4 is current)."""
        return self._version

    @version.setter
    def version(self, value):
        if value not in (3, 4):
            raise ValueError("NTP version must be 3 or 4")
        self._version = value

class IrigSyncInterface(SyncInterface):
    """A concrete IRIG sync interface with a configurable source id."""

    def __init__(self, name="IrigSyncInterface", upstream_reachable=False):
        super().__init__(
            name,
            available_modes=[SyncMode.Off, SyncMode.Input],
            upstream_reachable=upstream_reachable)
        self._source_id = ""

    def _get_upstream_domain_id(self):
        """Canonical form: irig:<configured-source-id>."""
        return f"irig:{self._source_id}" if self._source_id else ""

    @property
    def source_id(self):
        """Configured reference-domain identity. All devices on the same IRIG
        feed must use the same id."""
        return self._source_id

    @source_id.setter
    def source_id(self, value):
        self._source_id = value

class Synchronization:
    """Current state of the device's synchronization services. The state
    already applied to the device clock and openDAQ data is exposed
    separately through ReferenceDomainInfo."""

    def __init__(self, device):
        self._device = device
        self._name = "DaqSynchronization"

        # The device's own clock: always reachable, selected as the source
        # by default. Its id is the device's local id.
        clock = SyncInterface(
            "ClockSyncInterface",
            available_modes=[SyncMode.Off, SyncMode.Input],
            time_protocol=TimeProtocol.Utc,
            upstream_domain_id=f"clock-sync:{device.global_id}",
            upstream_reachable=True)

        ptp = PtpSyncInterface(upstream_reachable=True)
        ntp = NtpSyncInterface()
        gps = SyncInterface(
            "GpsSyncInterface",
            available_modes=[SyncMode.Off, SyncMode.Input],
            time_protocol=TimeProtocol.Gps,
            upstream_domain_id="gps")
        irig = IrigSyncInterface()

        self._interfaces = {i.name: i for i in [clock, ptp, ntp, gps, irig]}
        self._local_id = clock._upstream_domain_id

        # Handlers subscribed through on_property_value_changed.
        self._property_changed_handlers = []

        # ClockSyncInterface is the default source; selecting it through
        # set_source runs the same activation path as any later selection.
        self._source = None
        self.set_source(clock)

    @property
    def name(self):
        return self._name

    def get_sync_interfaces(self):
        """<name, interface> pairs."""
        return dict(self._interfaces)

    def get_available_sources(self):
        """All interfaces that support Input or Auto mode."""
        sources = {}
        for name, interface in self._interfaces.items():
            modes = interface.get_available_modes()
            if SyncMode.Input in modes or SyncMode.Auto in modes:
                sources[name] = interface
        return sources

    def get_source(self):
        return self._source

    def on_property_value_changed(self, handler):
        """Stub stand-in for subscribing to the sync component's
        property-value changes on the real device. The handler is called as
        handler(sender, property_name, value) after a change is applied."""
        self._property_changed_handlers.append(handler)

    def set_source(self, source, prefer_auto=True):
        """Selects Auto when prefer_auto and the interface supports it,
        otherwise Input. The previous source is set to Off. On the real
        device a failed activation restores ClockSyncInterface as the
        source; stub activation cannot fail."""
        if source is None:
            raise ValueError("source is None")
        if source not in self._interfaces.values():
            raise ValueError(f"{source.name} does not belong to this device")
        if source.get_output_only():
            raise ValueError(f"{source.name} is configured output-only")
        modes = source.get_available_modes()
        if prefer_auto and SyncMode.Auto in modes:
            mode = SyncMode.Auto
        elif SyncMode.Input in modes:
            mode = SyncMode.Input
        else:
            raise ValueError(f"{source.name} cannot be selected as a source")

        previous = self._source
        if previous is not None:
            previous._set_mode(SyncMode.Off)
            previous._is_source = False
        source._set_mode(mode)
        source._is_source = True
        self._source = source
        # On the real device the change is also logged (LOG_I).
        self._notify_property_value_changed("Source", source.name)

    def get_reference_domain_ids(self):
        """Service-side view: the selected source id plus the id of each
        active output. For the state applied to data, use
        ReferenceDomainInfo.get_reference_domain_ids."""
        ids = []
        source_id = self._source.get_reference_domain_id()
        if source_id:
            ids.append(source_id)
        for interface in self._interfaces.values():
            if interface is self._source:
                continue
            if interface._derive_role_status() == RoleStatus.Output:
                output_id = interface.get_reference_domain_id()
                if output_id:
                    ids.append(output_id)
        return ids

    def get_reference_domain_info(self):
        """Applied state. On the real device this is read from the device
        domain or a domain signal's descriptor and can lag the service
        state while a source is calibrating; the stub has no lag."""
        source_id = self._source.get_reference_domain_id() or self._local_id
        ids = [source_id] + [i for i in self.get_reference_domain_ids() if i != source_id]
        return ReferenceDomainInfo(
            source_reference_domain_id=source_id,
            reference_domain_ids=ids,
            offset=0,  # no clock discontinuity has occurred in the stub
            time_protocol=self._source._time_protocol)

    def _notify_property_value_changed(self, property_name, value):
        for handler in self._property_changed_handlers:
            handler(self, property_name, value)

def get_synchronization(device):
    return Synchronization(device)
