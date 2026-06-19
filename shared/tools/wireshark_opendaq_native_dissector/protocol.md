# openDAQ Native Streaming Protocol — Wire Format

Reference for the **openDAQ native** protocol as carried over WebSocket (default port
**7420**). Compiled from the protocol/library sources in this repository and used as the basis
for the Wireshark dissector ([wireshark_opendaq_native_dissector.lua](wireshark_opendaq_native_dissector.lua)).

## Conventions

- **Endianness:** all multi-byte integers are **little-endian**. Structures are serialized as the
  raw in-memory layout of the C++ structs on little-endian (x86/x64) hosts — there is no explicit
  byte-swapping or field-by-field serialization in the streaming path.
- **`Int`** is `int64_t` (signed 64-bit). Defined in `core/coretypes/include/coretypes/common.h`.
- **Struct alignment:** several streaming headers are 8-byte aligned in C++, which inserts
  **4 bytes of padding** after the 12-byte generic header before the first 64-bit field. This
  padding is on the wire and must be skipped when parsing.
- A single WebSocket message (TCP write) may contain **multiple transport PDUs concatenated**
  back-to-back — see [Framing & coalescing](#framing--coalescing).

## Source locations

| Area | File |
|---|---|
| Transport header, `PayloadType` enum | `shared/libraries/native_streaming_protocol/include/native_streaming_protocol/native_streaming_protocol_types.h` (+ `src/...cpp`) |
| Streaming packet structs | `shared/libraries/packet_streaming/include/packet_streaming/packet_streaming.h` |
| STR_PACKET send/receive | `shared/libraries/native_streaming_protocol/src/base_session_handler.cpp` |
| Header dispatch (receiver) | `shared/libraries/native_streaming_protocol/src/client_session_handler.cpp`, `server_session_handler.cpp` |
| Coalescing (sender) | `shared/libraries/native_streaming_protocol/src/streaming_manager.cpp`, `packet_streaming/src/packet_streaming_server.cpp` |
| Release packet build/parse | `packet_streaming/src/packet_streaming_server.cpp`, `packet_streaming/src/packet_streaming_client.cpp` |

---

## 1. Transport header (4 bytes)

Every PDU starts with a single packed little-endian `uint32`:

```
 bits 31..28 : Payload Type   (mask 0xF0000000, >> 28)
 bits 27..0  : Payload Size   (mask 0x0FFFFFFF)   -- bytes that FOLLOW this 4-byte header
```

`MAX_PAYLOAD_SIZE = 0x0FFFFFFF`. The total PDU length on the wire is `4 + PayloadSize`.

### Payload Type enum

| Value | Name | Mnemonic | Payload summary |
|---|---|---|---|
| 1 | STREAMING_PACKET | STR_PACKET | A streaming packet (see §3) |
| 2 | SIGNAL_AVAILABLE | STR_SIGNAL_AVAIL | numeric id + string id + serialized signal (§4) |
| 3 | SIGNAL_UNAVAILABLE | STR_SIGNAL_UNAVAIL | numeric id + string id (§4b) |
| 4 | SUBSCRIBE | STR_SIGNAL_SUBSC_CMD | numeric id + string id (§5) |
| 5 | UNSUBSCRIBE | STR_SIGNAL_UNSUB_CMD | numeric id + string id (§5) |
| 6 | INIT_DONE | STR_PROTO_INIT_DONE | empty |
| 7 | SUBSCRIBE_ACK | STR_SIGNAL_SUBSC_ACK | numeric id (§6) |
| 8 | UNSUBSCRIBE_ACK | STR_SIGNAL_UNSUB_ACK | numeric id (§6) |
| 9 | CONFIGURATION_PACKET | CONFIG | config header + payload (§7) |
| 10 | TRANSPORT_LAYER_PROPERTIES | TRANS_LAYER_PROPS | JSON |
| 11 | INIT_REQUEST | STR_PROTO_INIT_REQ | empty |

---

## 2. Framing & coalescing

One transport PDU carries **exactly one** logical message (one streaming packet, one config
packet, etc.). The receiver reads one 4-byte header, then exactly `PayloadSize` bytes, dispatches
once, then reads the next 4-byte header.

However, for efficiency the **sender coalesces several complete PDUs into one socket write / one
WebSocket message** (`StreamingManager::cachePacketsToLinearBuffer` reserves one transport header
per packet and writes them back-to-back). These coalesced PDUs may belong to **different signals**.

**Implication for parsing:** a buffer handed to the dissector can contain N PDUs. Iterate:
read header → consume `4 + PayloadSize` bytes → repeat until the buffer is exhausted. A PDU may
also be split across WebSocket messages (handle via reassembly).

---

## 3. Streaming packet (STR_PACKET, type 1)

### 3.1 GenericPacketHeader (12 bytes)

| Offset | Field | Type | Notes |
|---|---|---|---|
| 0 | size | uint8 | Size of the **full per-type header** (12 / 48 / 32, see below) |
| 1 | type | uint8 | PacketType: `0=event, 1=data, 2=release, 3=alreadySent` |
| 2 | version | uint8 | |
| 3 | flags | uint8 | Bit flags (see §3.5) |
| 4 | signalId | uint32 | Per-connection numeric signal id. `0xFFFFFFFF` = none/not signal-specific |
| 8 | payloadSize | uint32 | Bytes of payload that follow the header |

`size` reports the per-type header length, so the variable trailer fields (packetId etc.) are part
of the "header" region; the actual data payload begins after `size` bytes and runs for
`payloadSize` bytes. (Transport PayloadSize for a STR_PACKET = `size + payloadSize`.)

### 3.2 Data packet (type 1) — DataPacketHeader (48 bytes)

| Offset | Field | Type | Notes |
|---|---|---|---|
| 0 | genericHeader | 12 bytes | §3.1 |
| 12 | (padding) | 4 bytes | alignment padding |
| 16 | packetId | Int (int64) | |
| 24 | domainPacketId | Int (int64) | `-1` if none |
| 32 | sampleCount | Int (int64) | |
| 40 | offset | 8 bytes | union, see §3.5 (present per offset-type flag) |

Followed by `payloadSize` bytes of raw sample data.

### 3.3 AlreadySent packet (type 3) — AlreadySentPacketHeader (32 bytes)

| Offset | Field | Type |
|---|---|---|
| 0 | genericHeader | 12 bytes |
| 12 | (padding) | 4 bytes |
| 16 | packetId | Int (int64) |
| 24 | domainPacketId | Int (int64) |

Tells the client a packet with this id was already sent; no data payload.

### 3.4 Event packet (type 0)

Bare 12-byte GenericPacketHeader followed by a **JSON** payload (an event/signal-descriptor
change). `size = 12`.

### 3.5 Release packet (type 2)

Connection-level housekeeping — **not tied to a signal**:

- Bare 12-byte GenericPacketHeader, `size = 12`.
- `signalId = 0xFFFFFFFF` (sentinel = none).
- Payload = an array of `Int` (int64) **packet IDs the client may now release**.
  `count = payloadSize / 8`.

Built by `PacketStreamingServer::checkAndSendReleasePacket` (from a global `readyForRelease`
vector, populated when server-side packets are destroyed); consumed by
`PacketStreamingClient::addReleasePacketBuffer`, which drops references / sets
`PACKET_FLAG_CAN_RELEASE` for each id.

### 3.6 Flags (byte at offset 3)

- **bit 0** — `PACKET_FLAG_CAN_RELEASE` (the packet may be released).
- **offset-type field** — bits above bit 0 (`PACKET_FLAG_OFFSET_TYPE_MASK`/`_SHIFT`). The
  dissector reads it as `offset_type = floor((flags % 8) / 2)`:
  - `1` → the 8-byte offset at DataPacketHeader+40 is **int64**.
  - `2` → the offset is **float64 (double)**.
  - `0` → no offset present.

---

## 4. Signal available (STR_SIGNAL_AVAIL, type 2)

| Offset | Field | Type | Notes |
|---|---|---|---|
| 0 | signalNumericId | uint32 | The per-connection numeric id assigned to this signal |
| 4 | stringIdLength | uint16 | Length of the string id |
| 6 | stringId | char[stringIdLength] | Signal string id, e.g. `/Dewesoft_DB26000025/IO/1/AI7/Sig/AITime` |
| 6+len | serialized signal | JSON | Full serialized signal descriptor |

This is the **binding** between a numeric `signalId` (used in STR_PACKET headers) and its
human-readable string path. The numeric id is only unique **within a single connection**, and may
be **re-bound** to a different string later in the stream — consumers should track bindings
per-connection and per-position (frame order).

---

## 4b. Signal unavailable (STR_SIGNAL_UNAVAIL, type 3)

| Offset | Field | Type | Notes |
|---|---|---|---|
| 0 | signalNumericId | uint32 | The numeric id being torn down |
| 4 | stringId | char[] | Signal string id; **no length prefix**, runs to end of payload (length = `payloadSize - 4`) |

Announces that a previously-available signal is gone. The numeric id may later be **re-used** for a
different signal via a new STR_SIGNAL_AVAIL, so consumers tracking id→name bindings should treat
this as a teardown from this frame onward.

---

## 5. Subscribe / Unsubscribe command (types 4 / 5)

| Offset | Field | Type | Notes |
|---|---|---|---|
| 0 | signalNumericId | uint32 | |
| 4 | stringId | char[] | Remainder of payload (no length prefix) |

---

## 6. Subscribe / Unsubscribe ACK (types 7 / 8)

| Offset | Field | Type |
|---|---|---|
| 0 | signalNumericId | uint32 |

(Any trailing bytes are unused/raw.)

---

## 7. Configuration packet (CONFIG, type 9)

### 7.1 Config PacketHeader (16 bytes)

| Offset | Field | Type | Notes |
|---|---|---|---|
| 0 | headerSize | uint8 | |
| 1 | type | uint8 | Config PacketType (see below) |
| 2 | unused | 2 bytes | |
| 4 | payloadSize | uint32 | |
| 8 | id | uint64 | Request/response correlation id |

The config payload begins after the 16-byte header (i.e. transport offset 20).

### 7.2 Config PacketType

| Value | Name |
|---|---|
| 0x80 | GetProtocolInfo |
| 0x81 | UpgradeProtocol |
| 0x82 | Rpc |
| 0x83 | ServerNotification |
| 0x84 | InvalidRequest |
| 0x85 | NoReplyRpc |
| 0x86 | ConnectionRejected |

### 7.3 GetProtocolInfo (0x80) payload

| Offset | Field | Type |
|---|---|---|
| 0 | currentVersion | uint16 |
| 2 | supportedCount | uint16 |
| 4 | supportedVersion[] | uint16 × supportedCount |

### 7.4 UpgradeProtocol (0x81) payload

- Request: `int16 version`.
- Response: `int8 result` (non-zero = success, 0 = fail).

### 7.5 Rpc (0x82) / ServerNotification (0x83) payload

**JSON.** RPC requests carry a `"Name"` method key and parameters; responses may carry an
`"ErrorCode"`. ServerNotification carries an asynchronous JSON notification from the server.

---

## 8. Transport layer properties (TRANS_LAYER_PROPS, type 10)

Payload is **JSON** describing transport-layer properties negotiated/announced for the connection.

---

## Appendix — quick byte map of a Data STR_PACKET

```
offset  bytes  field
   0      4    transport header  (type=1 STR_PACKET, size=transport payload size)
   4      1    generic.size            = 48
   5      1    generic.type            = 1 (data)
   6      1    generic.version
   7      1    generic.flags
   8      4    generic.signalId
  12      4    generic.payloadSize     = N (sample bytes)
  16      4    padding
  20      8    packetId        (int64)
  28      8    domainPacketId  (int64)
  36      8    sampleCount     (int64)
  44      8    offset          (int64 or float64, per flags)
  52      N    sample data
```
