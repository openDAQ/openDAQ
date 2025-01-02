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

/*!
 * @brief Sets the continuation to only execute after this task completes.
 * Be careful of forming cycles as tasks whose dependencies cannot be satisfied will never execute.
 * @param continuation The task that should wait for this one to complete.
 * @throws DaqNotSupportedException If the @p continuation implementation was not crated by the scheduler library.
 * @returns the continuation task
 */
TaskPtr then(const daq::TaskPtr& continuation) const
{
    if (this->object == nullptr)
        throw daq::InvalidParameterException();

    auto errCode = this->object->then(continuation);
    daq::checkErrorInfo(errCode);

    return continuation;
}

/*!
 * @brief Sets the continuation to only execute after this task completes.
 * Be careful of forming cycles as tasks whose dependencies cannot be satisfied will never execute.
 * @param continuation The task that should wait for this one to complete.
 * @throws DaqNotSupportedException If the @p continuation implementation was not crated by the scheduler library.
 * @returns the continuation task
 */
TaskPtr then(daq::ITask* continuation) const
{
    return then(daq::TaskPtr(continuation));
}

/*!
 * @brief Sets the continuation to only execute after this task completes.
 * Be careful of forming cycles as tasks whose dependencies cannot be satisfied will never execute.
 * @param continuation The task that should wait for this one to complete.
 * @param name The name used for diagnostics.
 * @throws DaqNotSupportedException If the @p work implementation was not crated by the scheduler library.
 * @returns the continuation task created from the @p work procedure.
 */
TaskPtr then(daq::ProcedurePtr work, daq::StringPtr name = "") const
{
    if (this->object == nullptr)
        throw daq::InvalidParameterException();

    TaskPtr continuation = Task_Create(work, name);
    auto errCode = this->object->then(continuation);
    daq::checkErrorInfo(errCode);

    return continuation;
}
