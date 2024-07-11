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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <opendaq/block_reader.h>
#include <opendaq/input_port.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_block_reader Block reader builder
 * @{
 */

/*!
 * @brief Builder component of Block reader objects. Contains setter methods to configure the Block reader parameters
 * and a `build` method that builds the Unit object.
 */
DECLARE_OPENDAQ_INTERFACE(IBlockReaderBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns a Block reader object using the currently set values of the Builder.
     * @param[out] blockReader The built Block reader.
     */
    virtual ErrCode INTERFACE_FUNC build(IBlockReader** blockReader) = 0;

    // [returnSelf]
    /*!
     * @brief Sets old block reader instance to copy from
     * @param blockReader The old Block reader instance
     */
    virtual ErrCode INTERFACE_FUNC setOldBlockReader(IBlockReader* blockReader) = 0;

    /*!
     * @brief Gets the old Block reader instance to copy from
     * @param blockReader The old Block reader instance
     */
    virtual ErrCode INTERFACE_FUNC getOldBlockReader(IBlockReader** blockReader) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the signal to block reader
     * @param signal The signal which will be handled in block reader
     */
    virtual ErrCode INTERFACE_FUNC setSignal(ISignal* signal) = 0;

    /*!
     * @brief Gets the signal
     * @param signal The signal which will be handled in block reader
     */
    virtual ErrCode INTERFACE_FUNC getSignal(ISignal** signal) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the input port to block reader
     * @param port The input port which will be handled in block reader
     */
    virtual ErrCode INTERFACE_FUNC setInputPort(IInputPort* port) = 0;

    /*!
     * @brief Gets the input port
     * @param port The input port which will be handled in block reader
     */
    virtual ErrCode INTERFACE_FUNC getInputPort(IInputPort** port) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the value signal read type
     * @param type The value signal read type
     */
    virtual ErrCode INTERFACE_FUNC setValueReadType(SampleType type) = 0;

    /*!
     * @brief Gets the value signal read type
     * @param[out] type The value signal read type
     */
    virtual ErrCode INTERFACE_FUNC getValueReadType(SampleType* type) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the domain signal read type
     * @param type The domain signal read type
     */
    virtual ErrCode INTERFACE_FUNC setDomainReadType(SampleType type) = 0;

    /*!
     * @brief Gets the domain signal read type
     * @param[out] type The domain signal read type
     */
    virtual ErrCode INTERFACE_FUNC getDomainReadType(SampleType* type) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the read mode (Unscaled, Scaled, RawValue)
     * @param mode The read mode
     */
    virtual ErrCode INTERFACE_FUNC setReadMode(ReadMode mode) = 0;

    /*!
     * @brief Gets the read mode (Unscaled, Scaled, RawValue)
     * @param[out] mode The read mode
     */
    virtual ErrCode INTERFACE_FUNC getReadMode(ReadMode* mode) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the block size
     * @param mode The block size
     */
    virtual ErrCode INTERFACE_FUNC setBlockSize(SizeT size) = 0;

    /*!
     * @brief Gets the block size
     * @param[out] mode The block size
     */
    virtual ErrCode INTERFACE_FUNC getBlockSize(SizeT* size) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the overlap
     * @param mode The overlap
     */
    virtual ErrCode INTERFACE_FUNC setOverlap(SizeT overlap) = 0;

    /*!
     * @brief Gets the overlap
     * @param[out] mode The overlap[
     */
    virtual ErrCode INTERFACE_FUNC getOverlap(SizeT* overlap) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, BlockReaderBuilder, IBlockReaderBuilder)

/*!@}*/
END_NAMESPACE_OPENDAQ