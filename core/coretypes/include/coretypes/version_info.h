/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_utility
 * @defgroup types_version_info VersionInfo
 * @{
 */

/*!
 * @brief Represents a semantic version
 * composing of:
 *  - major version representing breaking changes
 *  - minor version representing new features
 *  - patch version representing only bug fixes.
 */
DECLARE_OPENDAQ_INTERFACE_EX(IVersionInfo, IBaseObject)
{
    DEFINE_LEGACY_INTFID("IVersionInfo")

    /*!
     * @brief The major version incremented at breaking changes.
     * @param[out] major The major version component.
     */
    virtual ErrCode INTERFACE_FUNC getMajor(SizeT* major) = 0;

    /*!
     * @brief The minor version incremented at new features with full backwards compatibility.
     * @param[out] minor The minor version component.
     */
    virtual ErrCode INTERFACE_FUNC getMinor(SizeT* minor) = 0;

    /*!
     * @brief The patch version incremented when only bug-fixes are added.
     * @param[out] patch The patch version component.
     */
    virtual ErrCode INTERFACE_FUNC getPatch(SizeT* patch) = 0;
};

/*!
 * @}
 */

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, VersionInfo, SizeT, major, SizeT, minor, SizeT, patch);

END_NAMESPACE_OPENDAQ
