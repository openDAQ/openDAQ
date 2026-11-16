/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/address_info.h>
#include <chrono>
#include <map>
#include <string>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

// Pings each address once: Reachable on a reply, Unreachable on none or no route, Unknown if ICMP is not permitted or unparsable.
std::map<std::string, AddressReachabilityStatus> icmpPing(const std::vector<std::string>& addresses,
                                                          int hopLimit,
                                                          std::chrono::milliseconds timeout);

END_NAMESPACE_OPENDAQ
