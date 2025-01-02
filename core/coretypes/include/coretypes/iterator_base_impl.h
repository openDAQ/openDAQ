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

#include <coretypes/intfs.h>

#include <coretypes/iterator.h>
#include <coretypes/iterator_helper.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename T>
struct DefaultValueSelector
{
    IBaseObject* operator()(T t) const
    {
        if (t != nullptr)
            t->addRef();

        return t;
    }
};

template <typename T, typename ElementTypeInterface, typename ValueSelector = DefaultValueSelector<typename T::iterator::value_type>>
class IteratorBaseImpl : public ImplementationOf<IIterator, ElementTypeInterface>
{
protected:
    using IteratorType = typename T::const_iterator;

public:
    IteratorBaseImpl(IBaseObject* coreContainer, IteratorType it, IteratorType end, ValueSelector valueSelector = ValueSelector());
    ~IteratorBaseImpl() override;

    ErrCode INTERFACE_FUNC moveNext() override;
    ErrCode INTERFACE_FUNC getCurrent(IBaseObject** obj) const override;
    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equal) const override;

private:
    IBaseObject* coreContainer;
    IteratorType it;
    IteratorType end;
    bool started;
    ValueSelector valueSelector;
};

template <typename T, typename E, typename VS>
IteratorBaseImpl<T, E, VS>::IteratorBaseImpl(IBaseObject* coreContainer, IteratorType it, IteratorType end, VS valueSelector)
    : coreContainer(coreContainer)
    , it(std::move(it))
    , end(std::move(end))
    , started(false)
    , valueSelector(std::move(valueSelector))
{
    assert(coreContainer != nullptr);
    coreContainer->addRef();
}

template <typename T, typename E, typename VS>
IteratorBaseImpl<T, E, VS>::~IteratorBaseImpl()
{
    coreContainer->releaseRef();
}

template <typename T, typename E, typename VS>
ErrCode IteratorBaseImpl<T, E, VS>::getCurrent(IBaseObject** obj) const
{
    if (obj == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (it == end)
        return OPENDAQ_ERR_NOTASSIGNED;

    *obj = valueSelector(*it);
    return OPENDAQ_SUCCESS;
}

template <typename T, typename E, typename VS>
ErrCode IteratorBaseImpl<T, E, VS>::equals(IBaseObject* other, Bool* equal) const
{
    if (equal == nullptr)
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equal output parameter must not be null.");

    *equal = false;
    if (!other)
        return OPENDAQ_SUCCESS;

    IIterator* itOther;
    ErrCode err = other->borrowInterface(IIterator::Id, reinterpret_cast<void**>(&itOther));
    if (!OPENDAQ_SUCCEEDED(err))
        return err;

    return compareIterators(this, itOther, equal);
}

template <typename T, typename E, typename VS>
ErrCode IteratorBaseImpl<T, E, VS>::moveNext()
{
    return iteratorMoveNext<T>(started, it, end);
}

END_NAMESPACE_OPENDAQ
