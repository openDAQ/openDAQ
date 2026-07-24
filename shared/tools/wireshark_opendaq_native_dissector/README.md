# OpenDAQ Native Protocol Dissector for Wireshark

A custom Lua-based Wireshark dissector for parsing and analyzing the **OpenDAQ native** protocol over WebSockets. 

The dissector provides a seamless debugging experience natively within Wireshark. It translates raw byte streams into fully structured, human-readable packet details without requiring external Lua JSON libraries.

## Features

* **Configurable WebSocket Hook:** Binds to WebSocket traffic on port `7420` by default; the port(s) are configurable under **Edit → Preferences → Protocols → OpenDAQ native** (or `-o opendaq_native.ports:"..."` on the CLI), and may be a single port, a list, or ranges (e.g. `7420,7500-7510`).
* **Base Header Decoding:** Parses the standard 4-byte Little-Endian protocol header (Payload Type and Size).
* **Smart Info Column:** Extracts critical context (RPC Method Names, Error Codes, Signal IDs, and Streaming Sub-types) and appends them directly to Wireshark's top-level "Info" column for quick scanning.
* **Native JSON Integration:** Handoffs embedded JSON payloads (e.g., RPC configurations, Transport Layer Properties, and Signal parameters) to Wireshark's highly optimized built-in JSON dissector, creating fully expandable, syntax-highlighted trees.
* **C++ Memory Alignment Handling:** Safely accounts for 64-bit struct alignment padding in streaming packets to ensure accurate extraction of double and int64 offsets.

### Supported Packet Types

* `STR_PACKET` (Type 1): Handles Event, Data, Release, and AlreadySent sub-types.
* `STR_SIGNAL_AVAIL` (Type 2): Extracts Signal Numeric ID, String Length, and String ID.
* `STR_SIGNAL_SUBSC_CMD` / `UNSUB_CMD` (Types 4 & 5)
* `STR_SIGNAL_SUBSC_ACK` / `UNSUB_ACK` (Types 7 & 8)
* `CONFIG` (Type 9): Fully parses the 16-byte configuration header and handles:
  * `GetProtocolInfo` (0x80)
  * `UpgradeProtocol` (0x81)
  * `Rpc` (0x82)
  * `ServerNotification` (0x83)
* `TRANS_LAYER_PROPS` (Type 10)

---

## Installation

To use this dissector, you simply need to place the `opendaq_native.lua` script into your Wireshark `plugins` folder.

### 1. Locate your Plugins Folder
Open Wireshark and navigate to **Help** > **About Wireshark** (or **Wireshark** > **About Wireshark** on macOS). 
Click on the **Folders** tab and double-click the path listed next to **Personal Lua Plugins** to open it in your file explorer.

If you prefer to navigate manually, the default paths are usually:
* **Windows:** `%APPDATA%\Wireshark\plugins`
* **macOS:** `~/.config/wireshark/plugins` 
* **Linux:** `~/.local/lib/wireshark/plugins` or `~/.config/wireshark/plugins`

### 2. Install the Script
1. Download or copy the `opendaq_native.lua` file.
2. Drop it directly into the `plugins` folder.
3. Restart Wireshark, or press `Ctrl+Shift+L` (`Cmd+Shift+L` on Mac) to reload all Lua scripts dynamically.

---

## Usage

1. Start a packet capture on the network interface handling your OpenDAQ traffic.
2. Because the dissector registers to WebSocket port `7420` by default, OpenDAQ native packets will automatically display as **OpenDAQ native** in the Protocol column. To capture on a different port (or several), set it under **Edit → Preferences → Protocols → OpenDAQ native** → **WebSocket port(s)** — accepts a single port, a list, or ranges (e.g. `7420,7500-7510`).
3. To filter your capture and see *only* this traffic, type the following into the Wireshark display filter bar:
   ```text
   opendaq_native
   ```

---

## See also

* [PROTOCOL.md](PROTOCOL.md) — the on-the-wire format of every packet type.
* [CHANGELOG.md](CHANGELOG.md) — version history.