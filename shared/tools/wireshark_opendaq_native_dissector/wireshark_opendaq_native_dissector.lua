-- Define the custom protocol
local my_proto = Proto("opendaq_native", "OpenDAQ native")

-- 1. Map your C++ Enums to Lua tables
local payload_types = {
    [1]  = "STR_PACKET",
    [2]  = "STR_SIGNAL_AVAIL",
    [3]  = "STR_SIGNAL_UNAVAIL",
    [4]  = "STR_SIGNAL_SUBSC_CMD",
    [5]  = "STR_SIGNAL_UNSUB_CMD",
    [6]  = "STR_PROTO_INIT_DONE",
    [7]  = "STR_SIGNAL_SUBSC_ACK",
    [8]  = "STR_SIGNAL_UNSUB_ACK",
    [9]  = "CONFIG",
    [10] = "TRANS_LAYER_PROPS",
    [11] = "STR_PROTO_INIT_REQ"
}

-- Map the Configuration PacketType Enum
local packet_types = {
    [0x80] = "GetProtocolInfo",
    [0x81] = "UpgradeProtocol",
    [0x82] = "Rpc",
    [0x83] = "ServerNotification",
    [0x84] = "InvalidRequest",
    [0x85] = "NoReplyRpc",
    [0x86] = "ConnectionRejected"
}

-- Map the Streaming PacketType Enum
local str_pkt_types = {
    [0] = "Event",
    [1] = "Data",
    [2] = "Release",
    [3] = "AlreadySent"
}

-- Fetch Wireshark's built-in JSON dissector
local json_dissector = Dissector.get("json")

-- Per-conversation, per-id timeline of name bindings:
--   signal_names[conv_key][numeric_id] = { {frame=N1, name="..."}, {frame=N2, name="..."}, ... }
-- entries are appended in increasing frame order during the first pass.
-- Signal IDs are only unique within a connection, and a single ID may be re-bound
-- to a different string over the life of the stream, hence the (conversation, id, frame) shape.
local signal_names = {}

-- A single frame can carry several WebSocket messages (and TCP-reassembled messages
-- complete on a later frame), so our dissector is invoked multiple times per frame.
-- Accumulate every call's per-PDU records and summarize them once, so the Info column
-- reflects ALL messages in the frame (deduped), not just the last one. Built in frame
-- order during the first pass; the resulting string is cached per frame and read back
-- idempotently on later re-dissection (clicking a packet).
local info_by_frame = {}   -- frame number -> final Info string
local acc_frame = nil      -- frame currently being accumulated
local acc_records = nil    -- per-PDU records gathered across the frame's calls
local acc_verbose = nil    -- per-PDU verbose strings gathered across the frame's calls

-- Canonical, direction-independent key for the conversation a packet belongs to.
local function conv_key(pinfo)
    local a = tostring(pinfo.src) .. ":" .. tostring(pinfo.src_port)
    local b = tostring(pinfo.dst) .. ":" .. tostring(pinfo.dst_port)
    if a <= b then return a .. "<->" .. b else return b .. "<->" .. a end
end

-- Resolve the name that was in effect for num_id at the current packet's position.
-- Returns the latest binding whose frame is <= pinfo.number, or nil.
local function lookup_signal_name(pinfo, num_id)
    local m = signal_names[conv_key(pinfo)]
    local timeline = m and m[num_id]
    if not timeline then return nil end
    local fnum = pinfo.number
    for i = #timeline, 1, -1 do
        if timeline[i].frame <= fnum then return timeline[i].name end
    end
    return nil
end

-- Record an id -> name binding for this conversation at the current frame.
-- Only appends on the first sequential pass, and only when the name actually
-- changed, so repeated identical announcements don't bloat the timeline.
-- A nil `name` is a teardown marker: lookups for later frames then return nil.
local function record_binding(pinfo, num_id, name)
    if not num_id or pinfo.visited then return end
    local key = conv_key(pinfo)
    local m = signal_names[key] or {}
    signal_names[key] = m
    local timeline = m[num_id] or {}
    m[num_id] = timeline
    local last = timeline[#timeline]
    if not last or last.name ~= name then
        timeline[#timeline + 1] = { frame = pinfo.number, name = name }
    end
end

-- Last path segment of a signal string id, for a compact Info-column label.
-- e.g. "/Dewesoft_DB26000025/IO/1/AI7/Sig/AITime" -> "AITime"
local function leaf_name(name)
    if not name then return nil end
    return name:match("([^/]+)$") or name
end

-- 2. Define the Protocol Fields
-- Base 4-byte Header
local f_type = ProtoField.uint32("opendaq_native.type", "Payload Type", base.DEC, payload_types, 0xF0000000)
local f_size = ProtoField.uint32("opendaq_native.size", "Payload Size (Bytes)", base.DEC, nil, 0x0FFFFFFF)

-- Configuration PacketHeader (16 bytes)
local f_config_hdr_size     = ProtoField.uint8("opendaq_native.config.header_size", "Config Header Size", base.DEC)
local f_config_type         = ProtoField.uint8("opendaq_native.config.type", "Config Packet Type", base.HEX, packet_types)
local f_config_unused       = ProtoField.bytes("opendaq_native.config.unused", "Unused Bytes")
local f_config_payload_size = ProtoField.uint32("opendaq_native.config.payload_size", "Config Payload Size", base.DEC)
local f_config_id           = ProtoField.uint64("opendaq_native.config.id", "Config ID", base.DEC)

-- GetProtocolInfo Payload Fields (16-bit integers)
local f_config_gpi_cur_version = ProtoField.uint16("opendaq_native.config.gpi.current_version", "Current Version", base.DEC)
local f_config_gpi_sup_count   = ProtoField.uint16("opendaq_native.config.gpi.supported_count", "Supported Versions Count", base.DEC)
local f_config_gpi_sup_version = ProtoField.uint16("opendaq_native.config.gpi.supported_version", "Supported Version", base.DEC)

-- UpgradeProtocol Payload Fields
local f_config_up_version = ProtoField.int16("opendaq_native.config.up.version", "Version", base.DEC)
local f_config_up_result  = ProtoField.int8("opendaq_native.config.up.result", "Result", base.DEC)

-- Streaming Packet (STR_PACKET) Fields
local f_str_hdr_size     = ProtoField.uint8("opendaq_native.str.size", "Header Size", base.DEC)
local f_str_hdr_type     = ProtoField.uint8("opendaq_native.str.type", "Packet Type", base.DEC, str_pkt_types)
local f_str_hdr_version  = ProtoField.uint8("opendaq_native.str.version", "Version", base.DEC)
local f_str_hdr_flags    = ProtoField.uint8("opendaq_native.str.flags", "Flags", base.HEX)
local f_str_hdr_sig_id   = ProtoField.uint32("opendaq_native.str.signal_id", "Signal ID", base.DEC)
local f_str_hdr_pl_size  = ProtoField.uint32("opendaq_native.str.payload_size", "Payload Size", base.DEC)

-- Generic generated field: the resolved full signal path, reused by every handler
-- that maps a numeric signal id to its name (STR_PACKET, ACKs, ...).
local f_signal_name = ProtoField.string("opendaq_native.signal_name", "Signal Name")

-- NEW: Field to visualize the C++ Struct Alignment Padding
local f_str_padding      = ProtoField.bytes("opendaq_native.str.padding", "Alignment Padding")

local f_str_pkt_id       = ProtoField.int64("opendaq_native.str.packet_id", "Packet ID", base.DEC)
local f_str_dom_pkt_id   = ProtoField.int64("opendaq_native.str.domain_packet_id", "Domain Packet ID", base.DEC)
local f_str_samp_count   = ProtoField.int64("opendaq_native.str.sample_count", "Sample Count", base.DEC)
local f_str_off_float    = ProtoField.double("opendaq_native.str.offset_float", "Offset (Float64)")
local f_str_off_int      = ProtoField.int64("opendaq_native.str.offset_int", "Offset (Int64)", base.DEC)

-- Release packet payload: a list of packet IDs the client may release
local f_str_rel_pkt_id   = ProtoField.int64("opendaq_native.str.release_packet_id", "Release Packet ID", base.DEC)

-- STR_SIGNAL_AVAIL Payload Fields
local f_sa_num_id  = ProtoField.uint32("opendaq_native.sa.num_id", "Signal Numeric ID", base.DEC)
local f_sa_str_len = ProtoField.uint16("opendaq_native.sa.str_len", "String ID Length", base.DEC)
local f_sa_str_id  = ProtoField.string("opendaq_native.sa.str_id", "Signal String ID")

-- STR_SIGNAL_SUBSC_CMD / STR_SIGNAL_UNSUB_CMD Payload Fields
local f_sub_num_id = ProtoField.uint32("opendaq_native.sub.num_id", "Signal Numeric ID", base.DEC)
local f_sub_str_id = ProtoField.string("opendaq_native.sub.str_id", "Signal String ID")

-- STR_SIGNAL_UNAVAIL Payload Fields
local f_ua_num_id = ProtoField.uint32("opendaq_native.ua.num_id", "Signal Numeric ID", base.DEC)
local f_ua_str_id = ProtoField.string("opendaq_native.ua.str_id", "Signal String ID")

-- STR_SIGNAL_SUBSC_ACK / STR_SIGNAL_UNSUB_ACK Payload Fields
local f_ack_num_id = ProtoField.uint32("opendaq_native.ack.num_id", "Signal Numeric ID", base.DEC)

-- Fallback Raw Payload and JSON String
local f_payload = ProtoField.bytes("opendaq_native.payload", "Raw Payload") 
local f_json_text = ProtoField.string("opendaq_native.json_text", "Raw JSON String")

-- Register all fields
my_proto.fields = { 
    f_type, f_size, 
    f_config_hdr_size, f_config_type, f_config_unused, f_config_payload_size, f_config_id,
    f_config_gpi_cur_version, f_config_gpi_sup_count, f_config_gpi_sup_version,
    f_config_up_version, f_config_up_result,
    
    f_str_hdr_size, f_str_hdr_type, f_str_hdr_version, f_str_hdr_flags,
    f_str_hdr_sig_id, f_str_hdr_pl_size, f_str_padding, f_str_pkt_id, f_str_dom_pkt_id,
    f_str_samp_count, f_str_off_float, f_str_off_int, f_str_rel_pkt_id,

    f_signal_name,

    f_sa_num_id, f_sa_str_len, f_sa_str_id,
    f_sub_num_id, f_sub_str_id,
    f_ua_num_id, f_ua_str_id,
    f_ack_num_id,
    f_payload, f_json_text
}

-- 3a. Dissect a single transport PDU. `buffer` is a tvb covering exactly one
-- transport message (the 4-byte header plus its payload). Returns the Info-column
-- text for this PDU, or nil if it was too short to parse.
local function dissect_pdu(buffer, pinfo, tree)
    -- If the payload is smaller than our base 4-byte header, ignore it
    if buffer:len() < 4 then
        return nil
    end

    -- Create the root tree for this PDU
    local subtree = tree:add(my_proto, buffer(), "OpenDAQ native")
    
    -- Grab the first 4 bytes directly from the buffer
    local header_range = buffer(0, 4)
    
    -- Add the fields in Little-Endian byte order
    subtree:add_le(f_type, header_range)
    subtree:add_le(f_size, header_range)
    
    -- Read the raw 32-bit unsigned integer as Little-Endian
    local header_val = header_range:le_uint()
    
    -- Extract the Type and Size
    local type_val = math.floor(header_val / 0x10000000)
    local size_val = header_val % 0x10000000
        
    -- Look up the string name for the Info column
    local type_str = payload_types[type_val] or "UNKNOWN_TYPE"
    local info_string = type_str .. " (Size: " .. size_val .. ")"

    -- Compact record describing this PDU, used to build a grouped Info summary
    -- when several PDUs are coalesced into one WebSocket message.
    local rec = { kind = type_str }

    -- Check if this is a Configuration Packet (Type 9)
    if type_val == 9 then
        -- Ensure we have enough bytes for the 16-byte PacketHeader
        if buffer:len() >= 20 then
            -- Create a sub-tree specifically for the Config Header
            local config_tree = subtree:add(my_proto, buffer(4, 16), "Configuration Header")
            
            -- Read the 16 bytes based on your C++ struct layout
            config_tree:add(f_config_hdr_size, buffer(4, 1))
            config_tree:add(f_config_type, buffer(5, 1))
            config_tree:add(f_config_unused, buffer(6, 2))
            config_tree:add_le(f_config_payload_size, buffer(8, 4))
            config_tree:add_le(f_config_id, buffer(12, 8))
            
            -- Append the specific Config Type to the Wireshark Info column for easier reading
            local c_type_val = buffer(5, 1):uint()
            local c_type_str = packet_types[c_type_val] or string.format("UNKNOWN (0x%02X)", c_type_val)
            info_string = info_string .. " [" .. c_type_str .. "]"
            
            -- Handle data beyond the 20 bytes (4 base + 16 config)
            if buffer:len() > 20 then
                local data_range = buffer(20)
                
                if c_type_val == 0x80 then
                    -- Specific binary parsing for GetProtocolInfo (0x80)
                    local gpi_tree = subtree:add(my_proto, data_range, "GetProtocolInfo Payload")
                    if data_range:len() >= 2 then
                        gpi_tree:add_le(f_config_gpi_cur_version, data_range(0, 2))
                    end
                    if data_range:len() >= 4 then
                        local sup_count = data_range(2, 2):le_uint()
                        gpi_tree:add_le(f_config_gpi_sup_count, data_range(2, 2))
                        local offset = 4
                        for i = 1, sup_count do
                            if data_range:len() >= offset + 2 then
                                gpi_tree:add_le(f_config_gpi_sup_version, data_range(offset, 2))
                                offset = offset + 2
                            else
                                break
                            end
                        end
                    end

                elseif c_type_val == 0x81 then
                    -- Specific parsing for UpgradeProtocol (0x81)
                    local up_tree = subtree:add(my_proto, data_range, "UpgradeProtocol Payload")
                    if data_range:len() == 2 then
                        up_tree:add_le(f_config_up_version, data_range(0, 2))
                    elseif data_range:len() == 1 then
                        local res_val = data_range(0, 1):le_int()
                        local res_item = up_tree:add_le(f_config_up_result, data_range(0, 1))
                        if res_val ~= 0 then
                            res_item:append_text(" (Success)")
                        else
                            res_item:append_text(" (Fail)")
                        end
                    else
                        up_tree:add(f_payload, data_range)
                    end

                elseif c_type_val == 0x82 or c_type_val == 0x83 then
                    -- Extract RPC Method Name or Error Code for the Info column
                    if c_type_val == 0x82 then
                        local payload_str = data_range:string()
                        local rpc_method = payload_str:match('"key"%s*:%s*"Name"%s*,%s*"value"%s*:%s*"([^"]+)"')
                        if rpc_method then
                            info_string = info_string .. " RPC=" .. rpc_method
                        end
                        local rpc_error = payload_str:match('"key"%s*:%s*"ErrorCode"%s*,%s*"value"%s*:%s*(%-?%d+)')
                        if rpc_error then
                            info_string = info_string .. " Error code=" .. rpc_error
                        end
                    end

                    -- Parse JSON via Wireshark's JSON Dissector
                    if json_dissector then
                        local json_tree = subtree:add(my_proto, data_range, c_type_str .. " JSON Payload")
                        json_tree:add(f_json_text, data_range)
                        json_dissector:call(data_range:tvb(), pinfo, json_tree)
                    else
                        subtree:add(f_payload, data_range)
                    end
                else
                    subtree:add(f_payload, data_range)
                end
            end -- closes: if buffer:len() > 20
        else
            subtree:add(buffer(4), "Truncated Configuration Header")
        end -- closes: if buffer:len() >= 20
        
    elseif type_val == 1 then
        -- STR_PACKET (1) packet handling (Streaming Packets)
        if buffer:len() >= 16 then
            local offset = 4
            local str_tree = subtree:add(my_proto, buffer(offset), "Streaming Packet")
            
            -- Read 12-byte GenericPacketHeader
            str_tree:add(f_str_hdr_size, buffer(offset, 1))
            local str_pkt_type = buffer(offset + 1, 1):uint()
            str_tree:add(f_str_hdr_type, buffer(offset + 1, 1))
            str_tree:add(f_str_hdr_version, buffer(offset + 2, 1))
            
            local flags = buffer(offset + 3, 1):uint()
            local flag_item = str_tree:add(f_str_hdr_flags, buffer(offset + 3, 1))
            
            local band = (bit32 and bit32.band) or (bit and bit.band)
            if band and band(flags, 0x1) == 0x1 then
                flag_item:append_text(" (CAN_RELEASE)")
            end
            
            local sig_id = buffer(offset + 4, 4):le_uint()
            local sig_id_item = str_tree:add_le(f_str_hdr_sig_id, buffer(offset + 4, 4))
            -- 0xFFFFFFFF is the sentinel for "no signal" (e.g. Release packets are
            -- connection-level housekeeping, not tied to a single signal).
            local sig_none = (sig_id == 0xFFFFFFFF)
            local sig_full_name = nil
            if sig_none then
                sig_id_item:append_text(" (none / not signal-specific)")
            else
                -- Add the resolved full signal path as a generated, filterable field
                sig_full_name = lookup_signal_name(pinfo, sig_id)
                if sig_full_name then
                    str_tree:add(f_signal_name, buffer(offset + 4, 4), sig_full_name):set_generated()
                end
            end
            str_tree:add_le(f_str_hdr_pl_size, buffer(offset + 8, 4))
            
            offset = offset + 12
            
            local str_pkt_str = str_pkt_types[str_pkt_type] or "Unknown"
            if sig_none then
                -- Connection-level packet (no signal). Release gets a "RELEASE(N IDs)"
                -- label built where its payload is parsed; other sentinel packets just
                -- show their sub-type here.
                if str_pkt_type ~= 2 then
                    info_string = info_string .. string.format(" [%s]", str_pkt_str)
                    rec.sub = str_pkt_str
                end
            else
                rec.sub = str_pkt_str
                local name = sig_full_name
                local sig_label = name and string.format("%s(%d)", name, sig_id) or tostring(sig_id)
                info_string = info_string .. string.format(" [%s | Signal: %s]", str_pkt_str, sig_label)
                rec.item = name and string.format("%s(%d)", leaf_name(name), sig_id) or tostring(sig_id)
            end
            
            if str_pkt_type == 1 then
                -- DataPacketHeader: C++ Struct Alignment Padding (4 bytes) + Additional Data (32 bytes)
                if buffer:len() >= offset + 36 then
                    -- Show the padding explicitly to prevent confusion
                    str_tree:add(f_str_padding, buffer(offset, 4))
                    offset = offset + 4
                    
                    -- Now safely read the aligned 64-bit integers
                    str_tree:add_le(f_str_pkt_id, buffer(offset, 8))
                    str_tree:add_le(f_str_dom_pkt_id, buffer(offset + 8, 8))
                    str_tree:add_le(f_str_samp_count, buffer(offset + 16, 8))
                    
                    local offset_type = math.floor((flags % 8) / 2)
                    if offset_type == 1 then
                        str_tree:add_le(f_str_off_int, buffer(offset + 24, 8))
                    elseif offset_type == 2 then
                        str_tree:add_le(f_str_off_float, buffer(offset + 24, 8))
                    end
                    offset = offset + 32
                end
                
            elseif str_pkt_type == 3 then
                -- AlreadySentPacketHeader: C++ Struct Alignment Padding (4 bytes) + Additional Data (16 bytes)
                if buffer:len() >= offset + 20 then
                    -- Handle padding
                    str_tree:add(f_str_padding, buffer(offset, 4))
                    offset = offset + 4
                    
                    -- Read aligned 64-bit integers
                    str_tree:add_le(f_str_pkt_id, buffer(offset, 8))
                    str_tree:add_le(f_str_dom_pkt_id, buffer(offset + 8, 8))
                    offset = offset + 16
                end
            end
            
            -- Handle remaining payload based on the streaming packet type
            if buffer:len() > offset then
                local data_range = buffer(offset)
                if str_pkt_type == 0 and json_dissector then
                    -- Parse remaining payload as JSON for 'Event' packets
                    local json_tree = str_tree:add(my_proto, data_range, "Event Payload (JSON)")
                    json_tree:add(f_json_text, data_range)
                    json_dissector:call(data_range:tvb(), pinfo, json_tree)
                elseif str_pkt_type == 2 then
                    -- Release payload: an array of int64 packet IDs the client may release
                    local count = math.floor(data_range:len() / 8)
                    local rel_tree = str_tree:add(my_proto, data_range, "Released Packet IDs")
                    rel_tree:append_text(string.format(" (%d)", count))
                    for i = 0, count - 1 do
                        rel_tree:add_le(f_str_rel_pkt_id, data_range(i * 8, 8))
                    end
                    -- Reflect the count in the Info column
                    info_string = info_string .. string.format(" RELEASE(%d IDs)", count)
                    rec.item = string.format("RELEASE(%d IDs)", count)
                else
                    -- Attach remaining bytes as binary payload for other streaming types
                    str_tree:add(f_payload, data_range)
                end
            end
        else
            subtree:add(f_payload, buffer(4))
        end

    elseif type_val == 2 then
        -- STR_SIGNAL_AVAIL packet handling
        if buffer:len() > 4 then
            local data_range = buffer(4)
            local sa_tree = subtree:add(my_proto, data_range, "Signal Available Payload")
            
            local offset = 0
            local num_id = nil
            if data_range:len() >= offset + 4 then
                num_id = data_range(offset, 4):le_uint()
                sa_tree:add_le(f_sa_num_id, data_range(offset, 4))
                offset = offset + 4
            end

            local str_len = 0
            if data_range:len() >= offset + 2 then
                str_len = data_range(offset, 2):le_uint()
                sa_tree:add_le(f_sa_str_len, data_range(offset, 2))
                offset = offset + 2
            end

            if str_len > 0 and data_range:len() >= offset + str_len then
                sa_tree:add(f_sa_str_id, data_range(offset, str_len))
                local str_id_val = data_range(offset, str_len):string()
                info_string = info_string .. " [" .. str_id_val .. "]"
                offset = offset + str_len

                if num_id then
                    rec.item = string.format("%s(%d)", leaf_name(str_id_val), num_id)
                else
                    rec.item = leaf_name(str_id_val)
                end

                -- Remember the id -> name binding for this conversation.
                record_binding(pinfo, num_id, str_id_val)
            end
            
            if data_range:len() > offset then
                local json_range = data_range(offset)
                if json_dissector then
                    local json_tree = sa_tree:add(my_proto, json_range, "Serialized Signal JSON")
                    json_tree:add(f_json_text, json_range)
                    json_dissector:call(json_range:tvb(), pinfo, json_tree)
                else
                    sa_tree:add(f_payload, json_range)
                end
            end
        end

    elseif type_val == 4 or type_val == 5 then
        -- STR_SIGNAL_SUBSC_CMD (4) and STR_SIGNAL_UNSUB_CMD (5) packet handling
        if buffer:len() > 4 then
            local data_range = buffer(4)
            local tree_name = (type_val == 4) and "Subscribe Command Payload" or "Unsubscribe Command Payload"
            local sub_tree = subtree:add(my_proto, data_range, tree_name)
            
            local offset = 0
            local num_id = nil
            if data_range:len() >= offset + 4 then
                num_id = data_range(offset, 4):le_uint()
                sub_tree:add_le(f_sub_num_id, data_range(offset, 4))
                offset = offset + 4
            end

            local remaining_len = data_range:len() - offset
            if remaining_len > 0 then
                sub_tree:add(f_sub_str_id, data_range(offset, remaining_len))
                local str_id_val = data_range(offset, remaining_len):string()
                info_string = info_string .. " [" .. str_id_val .. "]"
                rec.item = string.format("%s(%s)", leaf_name(str_id_val), tostring(num_id or "?"))
                -- Subscribe/unsubscribe carry the id->name binding too; feed the table so
                -- names resolve even when the STR_SIGNAL_AVAIL was not captured.
                record_binding(pinfo, num_id, str_id_val)
            end
        end
        
    elseif type_val == 7 or type_val == 8 then
        -- STR_SIGNAL_SUBSC_ACK (7) and STR_SIGNAL_UNSUB_ACK (8) packet handling
        if buffer:len() >= 8 then
            local data_range = buffer(4, 4)
            local tree_name = (type_val == 7) and "Subscribe ACK Payload" or "Unsubscribe ACK Payload"
            local ack_tree = subtree:add(my_proto, data_range, tree_name)
            
            local num_id = data_range:le_uint()
            ack_tree:add_le(f_ack_num_id, data_range)
            local name = lookup_signal_name(pinfo, num_id)
            if name then
                ack_tree:add(f_signal_name, data_range, name):set_generated()
                info_string = info_string .. string.format(" [%s(%d)]", name, num_id)
                rec.item = string.format("%s(%d)", leaf_name(name), num_id)
            else
                info_string = info_string .. " [ID: " .. num_id .. "]"
                rec.item = tostring(num_id)
            end

            if buffer:len() > 8 then
                ack_tree:add(f_payload, buffer(8))
            end
        elseif buffer:len() > 4 then
            subtree:add(f_payload, buffer(4))
        end
        
    elseif type_val == 3 then
        -- STR_SIGNAL_UNAVAIL (3) packet handling: uint32 numeric id + trailing string id
        if buffer:len() > 4 then
            local data_range = buffer(4)
            local ua_tree = subtree:add(my_proto, data_range, "Signal Unavailable Payload")

            local offset = 0
            local num_id = nil
            if data_range:len() >= 4 then
                num_id = data_range(0, 4):le_uint()
                ua_tree:add_le(f_ua_num_id, data_range(0, 4))
                offset = 4
            end

            if data_range:len() > offset then
                local str_id_val = data_range(offset):string()
                ua_tree:add(f_ua_str_id, data_range(offset))
                info_string = info_string .. " [" .. str_id_val .. "]"
                rec.item = string.format("%s(%s)", leaf_name(str_id_val), tostring(num_id or "?"))
            end

            -- The signal is gone: clear its name from this frame onward, so later
            -- packets don't render a stale name until the id is re-announced.
            record_binding(pinfo, num_id, nil)
        end

    elseif type_val == 10 then
        -- TRANS_LAYER_PROPS (10) packet handling
        if buffer:len() > 4 then
            local data_range = buffer(4)
            local tl_tree = subtree:add(my_proto, data_range, "Transport Layer Properties Payload")
            if json_dissector then
                tl_tree:add(f_json_text, data_range)
                json_dissector:call(data_range:tvb(), pinfo, tl_tree)
            else
                tl_tree:add(f_payload, data_range)
            end
        end

    else
        -- Standard Packet Handling for all other unmapped types
        if buffer:len() > 4 then
            local data_range = buffer(4)
            subtree:add(f_payload, data_range)
        end
    end
    
    return info_string, rec
end

-- Build the grouped Info summary from the per-PDU records, e.g.
--   "STR_PACKET x8 [Data]: AITime(703), AI(704), Status(705)"
-- Groups by payload type, lists distinct sub-types and distinct signal items.
local function summarize(records)
    local order, by_kind = {}, {}
    for _, rec in ipairs(records) do
        local g = by_kind[rec.kind]
        if not g then
            g = { kind = rec.kind, count = 0, subs = {}, sub_seen = {}, items = {}, item_seen = {} }
            by_kind[rec.kind] = g
            order[#order + 1] = g
        end
        g.count = g.count + 1
        if rec.sub and not g.sub_seen[rec.sub] then
            g.sub_seen[rec.sub] = true
            g.subs[#g.subs + 1] = rec.sub
        end
        if rec.item and not g.item_seen[rec.item] then
            g.item_seen[rec.item] = true
            g.items[#g.items + 1] = rec.item
        end
    end

    local parts = {}
    for _, g in ipairs(order) do
        local s = g.kind
        if g.count > 1 then s = s .. " x" .. g.count end
        if #g.subs > 0 then s = s .. " [" .. table.concat(g.subs, ",") .. "]" end
        if #g.items > 0 then s = s .. ": " .. table.concat(g.items, ", ") end
        parts[#parts + 1] = s
    end
    return table.concat(parts, "  ")
end

-- 3b. The Dissector Function: a single WebSocket message can carry several
-- transport PDUs concatenated back-to-back (the sender coalesces packets, possibly
-- from multiple signals, into one write). Split the buffer and dissect each PDU.
function my_proto.dissector(buffer, pinfo, tree)
    -- If the payload is smaller than our base 4-byte header, ignore it
    if buffer:len() < 4 then
        return 0
    end

    -- Change the protocol column
    pinfo.cols.protocol = "OpenDAQ native"

    local verbose = {}   -- per-PDU verbose strings (used when only one PDU)
    local records = {}   -- per-PDU compact records (used to summarize many PDUs)
    local offset = 0
    while offset + 4 <= buffer:len() do
        -- The low 28 bits of the 4-byte LE header are the payload size; the PDU is
        -- that payload plus the 4-byte header itself.
        local size_val = buffer(offset, 4):le_uint() % 0x10000000
        local pdu_len = 4 + size_val

        if offset + pdu_len > buffer:len() then
            -- A transport PDU is split across messages: ask for reassembly so the
            -- remainder is appended and the whole buffer is handed back next time.
            pinfo.desegment_offset = offset
            pinfo.desegment_len = (offset + pdu_len) - buffer:len()
            break
        end

        local info, rec = dissect_pdu(buffer(offset, pdu_len):tvb(), pinfo, tree)
        if info then verbose[#verbose + 1] = info end
        if rec then records[#records + 1] = rec end
        offset = offset + pdu_len
    end

    -- Accumulate this call's PDUs into the frame's running totals, then (re)build the
    -- frame's Info string: a single PDU keeps its rich per-PDU text; multiple PDUs (even
    -- across several messages in the frame) collapse into one deduped grouped summary.
    -- A frame matched by an `opendaq_native.signal` filter is then visibly shown as
    -- containing that signal, even when it lives in a non-final message of the frame.
    local fno = pinfo.number
    if not pinfo.visited then
        if acc_frame ~= fno then
            acc_frame, acc_records, acc_verbose = fno, {}, {}
        end
        for _, r in ipairs(records) do acc_records[#acc_records + 1] = r end
        for _, v in ipairs(verbose) do acc_verbose[#acc_verbose + 1] = v end

        local text
        if #acc_records == 1 then
            text = acc_verbose[1]
        elseif #acc_records > 0 then
            text = summarize(acc_records)
        end
        info_by_frame[fno] = text
    end

    -- Ensure Wireshark's native dissectors don't overwrite our Protocol column text
    pinfo.cols.protocol = "OpenDAQ native"
    local shown = info_by_frame[fno]
    if shown ~= nil then
        pinfo.cols.info:set(shown)
    end

    -- Tell Wireshark how many bytes we successfully parsed
    return buffer:len()
end

-- Reset per-capture state at the start of each capture load/reload, so data from a
-- previously opened file doesn't leak into the current one.
function my_proto.init()
    signal_names = {}
    info_by_frame = {}
    acc_frame, acc_records, acc_verbose = nil, nil, nil
end

-- 4. Register as a true WebSocket Sub-Dissector
local ws_dissector_table = DissectorTable.get("ws.port")
ws_dissector_table:add(7420, my_proto)