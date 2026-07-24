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

#include <type_traits>
#include <coretypes/lock_free_stack.h>

namespace daq::object_pool
{

template <class T>
class ObjectPool
{
//	static_assert(HasReset<T>::value, "T should have reset method");
//	static_assert(HasNext<T>::value, "T should have next field");

public:
	ObjectPool(size_t initialCount);
    ~ObjectPool();

	template <class ... Params>
	T* get(Params ... params);
	void addToFreeList(T* obj);
	size_t cleanup();
	size_t getObjectCount() const;

private:
	LockFreeStackHandle free_list;
	
#ifndef NDEBUG
	size_t objCount = 0;
#endif

	void init(size_t initialCount);
	size_t cleanupInternal();
};

template <class T>
ObjectPool<T>::ObjectPool(size_t initialCount)
	: free_list(daqLockFreeStackCreate(initialCount))
{
	init(initialCount);
}

template <class T>
ObjectPool<T>::~ObjectPool()
{
	daqLockFreeStackDestroy(free_list);
}

template <class T>
void ObjectPool<T>::init(size_t initialCount)
{
	for (size_t i = 0; i < initialCount; ++i)
	{
		T* obj = new T(this);
		daqLockFreeStackPush(free_list, obj);
	}

#ifndef NDEBUG
	objCount = initialCount;
#endif
}

template <class T>
size_t ObjectPool<T>::cleanupInternal()
{
#ifndef NDEBUG
	size_t cnt = 0;
#endif

	while (T* obj = static_cast<T*>(daqLockFreeStackPop(free_list)))
	{
		delete obj;
#ifndef NDEBUG
		cnt++;
#endif
	}

#ifndef NDEBUG
	const auto oldObjCount = objCount;
	if (cnt != objCount)
		throw std::runtime_error("Some objects were not cleaned up.");

	objCount = 0;
	return oldObjCount;
#endif
	return 0;
}

template <class T>
template <class ... Params>
T* ObjectPool<T>::get(Params... params)
{
	T* obj = static_cast<T*>(daqLockFreeStackPop(free_list));
	if (!obj)
	{
		obj = new T(this);

#ifndef NDEBUG
		objCount++;
#endif

	}

	obj->reset(params...);

	return obj;
}

template <class T>
void ObjectPool<T>::addToFreeList(T* obj)
{
    daqLockFreeStackPush(free_list, obj);
}

template <class T>
size_t ObjectPool<T>::cleanup()
{
	return cleanupInternal();
}

template <class T>
size_t ObjectPool<T>::getObjectCount() const
{
#ifndef NDEBUG
	return objCount;
#else
	return 0;
#endif 
}

}
