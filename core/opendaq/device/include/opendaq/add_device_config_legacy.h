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
#include <opendaq/add_device_config_ptr.h>
#include <opendaq/device_type_ptr.h>
#include <opendaq/streaming_type_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <opendaq/authentication_config_ptr.h>
#include <coreobjects/property_object_ptr.h>
#include <coretypes/dict_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Creates the legacy `General` section with its default values.
 * @param scanOnAdd The default of `ScanOnAdd`
 * @returns The `General` section
 */
PUBLIC_EXPORT PropertyObjectPtr createLegacyGeneralConfig(Bool scanOnAdd);

/*!
 * @brief Creates a legacy config with `Device`, `Streaming` and `General` sections and their default values.
 * @param deviceTypes The Device types that get a `Device` entry
 * @param streamingTypes The Streaming types that get a `Streaming` entry
 * @param scanOnAdd The default of `ScanOnAdd`
 * @returns The legacy config
 */
PUBLIC_EXPORT PropertyObjectPtr createLegacyAddDeviceConfig(const DictPtr<IString, IDeviceType>& deviceTypes,
                                                            const DictPtr<IString, IStreamingType>& streamingTypes,
                                                            Bool scanOnAdd);

/*!
 * @brief Applies the legacy `Username` and `Password` of a type config to an authentication config.
 * @param authenticationConfig The authentication config, or null
 * @param typeConfig The type config, or null
 *
 * With a non-empty `Username`, the `UserNamePassword` method is selected and the user name and password are supplied as its credential.
 * Nothing changes without a `Username`, or when the authentication config has no `UserNamePassword` method.
 */
PUBLIC_EXPORT void applyLegacyCredentials(const AuthenticationConfigPtr& authenticationConfig, const PropertyObjectPtr& typeConfig);

/*!
 * @brief Translates a legacy config into an Add device config.
 * @param legacyConfig The legacy config
 * @param deviceType The Device type the config is created for
 * @param streamingTypes The loaded Streaming types
 * @param scanOnAdd The scan on add setting used when `General` has none
 * @param loggerComponent The logger component that warnings are written to
 * @returns The Add device config
 * @throws InvalidValueException when the Add device config rejects a value of the `Device` or `Streaming` section.
 * @throws NoInterfaceException when a section is not a property object.
 * @throws NotFoundException when a `General` selection value is not in its selection.
 * @throws ConversionFailedException when a `General` value can't be converted to its property's type.
 *
 * The legacy config has `Device`, `Streaming` and `General` sections, holds the type config and `General` keys itself, or is null.
 * A null legacy config is read as an empty one.
 * Values are copied only for properties that both the legacy section and the type config have.
 * A missing `General` or `Device.<id>` section is read from the legacy config itself.
 * A missing `Streaming` section lists every loaded Streaming type the Device type can take.
 * A present `Streaming` section lists only those that have an entry in it.
 * A Streaming type with a reserved ID is not listed, and a warning is logged.
 * A missing `General` key takes the default of the legacy `General` section.
 * The `General` keys `Username`, `Password`, `ClientType` and `ExclusiveControlDropOthers` go into the type configs.
 * They replace only values that equal the `General` default.
 * The `Username` and `Password` of each type config also go into its authentication config, see `applyLegacyCredentials`.
 * `PrimaryAddressType` `IPv6` gives IPv6, and any other value gives IPv4.
 * A `PrimaryAddressType` other than `IPv4`, `IPv6` or blank logs a warning.
 *
 * Adapter and Root Device types get no streaming options, and the streaming keys of `General` are ignored.
 * Without listed Streaming types the config has no streaming options either.
 * The streaming mode is MinHops for the `MinHops` heuristic, and MinConnections otherwise.
 * The selected Streaming type is the first prioritized protocol among the listed types that `AllowedStreamingProtocols` allows.
 * Without one, it is chosen from those types by `selectStreamingType`.
 * An empty allowed list allows every type.
 * `NotConnected` and `AutomaticallyConnectStreaming` false give no selection.
 */
PUBLIC_EXPORT AddDeviceConfigPtr fromLegacyAddDeviceConfig(const PropertyObjectPtr& legacyConfig,
                                                           const DeviceTypePtr& deviceType,
                                                           const DictPtr<IString, IStreamingType>& streamingTypes,
                                                           Bool scanOnAdd,
                                                           const LoggerComponentPtr& loggerComponent);

/*!
 * @brief Gets the `ScanOnAdd` value of a legacy config.
 * @param legacyConfig The legacy config
 * @param scanOnAdd The scan on add setting used when `General` has none
 * @returns The `ScanOnAdd` value
 * @throws ConversionFailedException when `ScanOnAdd` can't be converted to a Bool.
 *
 * The value is read as `fromLegacyAddDeviceConfig` reads it, without a Device type.
 */
PUBLIC_EXPORT Bool getLegacyScanOnAdd(const PropertyObjectPtr& legacyConfig, Bool scanOnAdd);

/*!
 * @brief Translates a legacy config into the type config of a Streaming type.
 * @param config The config
 * @param streamingType The Streaming type the config is created for
 * @returns The type config, or the config as given when it isn't a legacy config
 * @throws InvalidValueException when the type config rejects a value of the `Streaming` entry.
 * @throws NotFoundException when a `General` selection value is not in its selection.
 * @throws ConversionFailedException when a `General` value can't be converted to its property's type.
 *
 * A legacy config has `Device`, `Streaming` and `General` sections.
 * Any other config, null included, is returned as given.
 * The type config is the default config of the Streaming type.
 * It takes the values of the type's entry in the `Streaming` section, copied as `fromLegacyAddDeviceConfig` copies them.
 * The `General` keys `Username`, `Password`, `ClientType` and `ExclusiveControlDropOthers` go into it as in `fromLegacyAddDeviceConfig`.
 * Without an entry for the type, the type config takes only the `General` keys.
 */
PUBLIC_EXPORT PropertyObjectPtr fromLegacyAddStreamingConfig(const PropertyObjectPtr& config, const StreamingTypePtr& streamingType);

/*!
 * @brief Translates an Add device config into a legacy config with `Device`, `Streaming` and `General` sections.
 * @param config The Add device config
 * @returns The legacy config
 *
 * The `Device` section holds a copy of the type config, keyed by the Device type ID.
 * The `Streaming` section holds a copy of each listed streaming config.
 * The `General` section holds every legacy key.
 * Its `Username`, `Password`, `ClientType` and `ExclusiveControlDropOthers` hold their defaults.
 * The type configs hold their values.
 * `PrimaryAddressType` is `IPv4` or `IPv6`, never blank.
 * `AllowedStreamingProtocols` is empty.
 * A selected Streaming type gives a `PrioritizedStreamingProtocols` list with only its ID.
 * No selection turns `AutomaticallyConnectStreaming` off.
 * The streaming mode gives the `StreamingConnectionHeuristic` of the same name.
 * A Mirrored or Unspecified Device type without streaming options turns `AutomaticallyConnectStreaming` off.
 */
PUBLIC_EXPORT PropertyObjectPtr toLegacyAddDeviceConfig(const AddDeviceConfigPtr& config);

END_NAMESPACE_OPENDAQ
