/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <coretypes/string_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/signal_ptr.h>

#include <ws-streaming/metadata.hpp>

#include <websocket_streaming_server_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE

/*!
 * @brief Translates an openDAQ data descriptor to WebSocket Streaming metadata.
 *
 * @param descriptor The openDAQ data descriptor to translate.
 * @param signalId The global identifier of the signal to which the descriptor applies.
 * @param domainSignalId If the signal described by @p descriptor has an associated domain signal,
 *     that signal's global identifier; otherwise, either an unassigned StringPtr or an empty
 *     string.
 *
 * @return A WebSocket Streaming metadata object.
 */
wss::metadata descriptorToMetadata(
    const DataDescriptorPtr& descriptor,
    const StringPtr& signalId,
    const StringPtr& domainSignalId = "");

/*!
 * @brief Translates an openDAQ data descriptor to WebSocket Streaming metadata.
 *
 * @param The signal described by the @p descriptor. This object is used to determine the global
 *     identifier of the associated domain signal, if any; then
 *     descriptorToMetadata(const DataDescriptor&, const StringPtr&) is called.
 * @param descriptor The openDAQ data descriptor to translate.
 *
 * @return A WebSocket Streaming metadata object.
 */
wss::metadata descriptorToMetadata(
    const SignalPtr& signal,
    const DataDescriptorPtr& descriptor);

END_NAMESPACE_OPENDAQ_NEWER_WEBSOCKET_STREAMING_SERVER_MODULE
