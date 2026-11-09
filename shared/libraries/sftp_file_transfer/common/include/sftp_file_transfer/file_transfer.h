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
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief A connection to a remote file server over which files can be transferred.
 *
 * The object is connected and authenticated for its whole lifetime. Releasing the last reference closes the
 * connection.
 *
 * Operations are added in later phases; for now the interface only represents an established connection.
 */
DECLARE_OPENDAQ_INTERFACE(IFileTransfer, IBaseObject)
{
};

END_NAMESPACE_OPENDAQ
