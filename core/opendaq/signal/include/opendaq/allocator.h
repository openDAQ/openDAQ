/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/data_descriptor.h>
#include <coretypes/baseobject.h>
#include <coretypes/common.h>
#include <opendaq/deleter.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_utility
 * @addtogroup opendaq_allocator Allocator
 * @{
 */

/*!
 * @brief An allocator used to allocate memory.
 *
 * The default BB allocator simply uses `malloc`, but the user can implement a custom allocator to
 * override this behavior (perhaps using a memory pool or different allocation strategy). An
 * example/reference implementation is provided which uses Microsoft `mimalloc`.
 */
DECLARE_OPENDAQ_INTERFACE(IAllocator, IBaseObject)
{
    /*!
     * @brief Allocates a chunk of memory for a packet.
     * @param descriptor The OPTIONAL data descriptor of the signal for which memory is to be
     *                   allocated. This can provide hints to the allocator. However, allocator
     *                   implementations MUST accept null values.
     * @param bytes The number of bytes to allocate.
     * @param align The alignment requirement of the caller (typically the element size). This
     *              value may be zero if the caller does not need to specify an alignment
     *              requirement.
     * @param[out] address The address of the allocated memory.
     *
     * The implementation MAY set address value to `nullptr` without returning an error code,
     * if the allocator is out of memory. Alternatively, the implementation MAY return an error
     * code in this case.
     */
    virtual ErrCode INTERFACE_FUNC allocate(
        const IDataDescriptor *descriptor,
        daq::SizeT bytes,
        daq::SizeT align,
        void** address) = 0;

    /*!
     * @brief Releases a chunk of memory allocated by allocate().
     * @param address The address of the allocated memory to release.
     *
     * The implementation MUST ignore calls where @p address is null.
     */
    virtual ErrCode INTERFACE_FUNC free(void* address) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, MallocAllocator,
    IAllocator
)

#ifdef OPENDAQ_MIMALLOC_SUPPORT
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, MiMallocAllocator,
    IAllocator
)
#endif

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ExternalAllocator,
    IAllocator,
    void*, data,
    IDeleter*, deleter
)

END_NAMESPACE_OPENDAQ
