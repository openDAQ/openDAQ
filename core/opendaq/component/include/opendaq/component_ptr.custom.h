/*
 * Copyright 2022-2024 Blueberry d.o.o.
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

/*!
 * @brief Notifies the component that it is being removed.
 *
 * Call `remove` on the component to mark it as removed. It's up to the implementation
 * to define what is does on the act of removal. Basic implementation of `Component`
 * will switch it to inactive state and it cannot be activated again.
 */
void remove() const
{
    if (this->object == nullptr)
        throw daq::InvalidParameterException();

    IRemovable* removable;
    auto errCode = this->object->borrowInterface(IRemovable::Id, reinterpret_cast<void**>(&removable));
    if (OPENDAQ_FAILED(errCode))
    {
        if (errCode == OPENDAQ_ERR_NOINTERFACE)
            return;
        daq::checkErrorInfo(errCode);
    }

    errCode = removable->remove();
    daq::checkErrorInfo(errCode);
}

/*!
 * @brief Returns True if component was removed.
 * @return True if component was removed; otherwise False.
 */
bool isRemoved() const
{
    if (this->object == nullptr)
        throw daq::InvalidParameterException();

    IRemovable* removable;
    auto errCode = this->object->borrowInterface(IRemovable::Id, reinterpret_cast<void**>(&removable));
    daq::checkErrorInfo(errCode);

    Bool removed;
    errCode = removable->isRemoved(&removed);
    daq::checkErrorInfo(errCode);

    return removed;
}
