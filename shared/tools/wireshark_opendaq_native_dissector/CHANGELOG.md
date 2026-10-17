# Changelog

All notable changes to the OpenDAQ native Wireshark dissector are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-06-19

### Added
- Per-conversation, position-aware signal **id → name resolution**: a `STR_SIGNAL_AVAIL`
  binding is remembered and every later reference renders as `NAME(ID)`. Bindings are scoped
  per connection and per stream position, so a re-bound id resolves correctly before/after the
  re-bind, and names are also fed from SUBSCRIBE/UNSUBSCRIBE.
- Decoding of **SIGNAL_UNAVAILABLE** (type 3) packets, including teardown of the id → name
  binding from that point in the stream onward.
- Name resolution in **SUBSCRIBE_ACK / UNSUBSCRIBE_ACK** (`NAME(ID)` instead of a bare `ID`).
- Unified filter fields that match a signal across its whole lifecycle regardless of packet type:
  `opendaq_native.signal` (numeric id) and `opendaq_native.signal_name`. Numeric ids are also
  shown in the Info column for building filters naturally.
- **Configurable WebSocket port(s)** via the `opendaq_native.ports` preference
  (Edit → Preferences → Protocols → OpenDAQ native, or `-o opendaq_native.ports:`); accepts a
  single port, a list, or ranges (e.g. `7420,7500-7510`). Default remains `7420`.
- `PROTOCOL.md` documenting the on-the-wire format of every packet type.

### Changed
- Internal refactor: `dissect_pdu` decomposed into per-payload-type handlers dispatched via a
  table (no behavior change).

### Fixed
- Coalesced transport PDUs: a single WebSocket message carrying multiple back-to-back PDUs (often
  from different signals) is now fully parsed, not just the first PDU.
- The Info column now aggregates across all WebSocket messages in a frame, so a frame matched by a
  signal filter visibly shows that signal even when it lives in a non-final message of the frame.

## [1.0.0] - 2026-06-19

### Added
- Initial Lua dissector for the **OpenDAQ native** protocol over WebSocket (port `7420`):
  4-byte little-endian transport header decode, all payload types, CONFIG 16-byte header parsing
  with handoff of embedded JSON (RPC, transport-layer properties, signal descriptors) to
  Wireshark's built-in JSON dissector, STR_PACKET subtypes (Event / Data / Release / AlreadySent)
  with C++ struct alignment handling, and a Smart Info column.
