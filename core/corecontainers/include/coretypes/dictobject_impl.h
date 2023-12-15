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
#include <coretypes/dictobject.h>
#include <coretypes/baseobject_impl.h>
#include <coretypes/coretype.h>
#include <coretypes/freezable.h>
#include <coretypes/serializable.h>
#include <coretypes/deserializer.h>
#include <coretypes/dict_element_type.h>
#include <tsl/ordered_map.h>

BEGIN_NAMESPACE_OPENDAQ

struct BaseObjectHash
{
    size_t operator()(IBaseObject* key) const
    {
        SizeT hashCode;
        if (key != nullptr)
            key->getHashCode(&hashCode);
        else
            hashCode = 0;
        return hashCode;
    }
};

struct BaseObjectEqualTo
{
    bool operator()(IBaseObject* a, IBaseObject* b) const
    {
        Bool eq{false};
        return OPENDAQ_SUCCEEDED(a->equals(b, &eq)) && eq;
    }
};

using BaseObjectPair = std::pair<IBaseObject*, IBaseObject*>;

ErrCode INTERFACE_FUNC deserializeDict(ISerializedObject* ser, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

class DictImpl : public ImplementationOf<IDict, IIterable, ISerializable, ICoreType, IDictElementType, IFreezable>
{
private:
    using HashTableType = typename tsl::ordered_map<IBaseObject*, IBaseObject*, BaseObjectHash, BaseObjectEqualTo>;
public:
    explicit DictImpl() = default;
    explicit DictImpl(IntfID keyId, IntfID valueId);

    ErrCode INTERFACE_FUNC get(IBaseObject* key, IBaseObject** value) override;
    ErrCode INTERFACE_FUNC set(IBaseObject* key, IBaseObject* value) override;

    ErrCode INTERFACE_FUNC remove(IBaseObject* key, IBaseObject** value) override;
    ErrCode INTERFACE_FUNC deleteItem(IBaseObject* key) override;

    ErrCode INTERFACE_FUNC clear() override;
    ErrCode INTERFACE_FUNC getCount(SizeT* count) override;

    ErrCode INTERFACE_FUNC hasKey(IBaseObject* key, Bool* hasKey) override;

    ErrCode INTERFACE_FUNC getKeyList(IList** keys) override;
    ErrCode INTERFACE_FUNC getValueList(IList** values) override;

    ErrCode INTERFACE_FUNC getKeys(IIterable** iterable) override;
    ErrCode INTERFACE_FUNC getValues(IIterable** iterable) override;

    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override;

    ErrCode INTERFACE_FUNC freeze() override;
    ErrCode INTERFACE_FUNC isFrozen(Bool* isFrozen) const override;

    // IDictElementType
    ErrCode INTERFACE_FUNC getKeyInterfaceId(IntfID* id) override;
    ErrCode INTERFACE_FUNC getValueInterfaceId(IntfID* id) override;

    // IIterable
    ErrCode INTERFACE_FUNC createStartIterator(IIterator** iterator) override;
    ErrCode INTERFACE_FUNC createEndIterator(IIterator** iterator) override;

    // ISerializable
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equal) const override;

    static ConstCharPtr SerializeId();

    static ErrCode Deserialize(ISerializedObject* ser, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
    {
        return deserializeDict(ser, context, factoryCallback, obj);
    }

protected:
    IntfID keyId{};
    IntfID valueId{};

    bool frozen = false;
    HashTableType hashTable;

    void internalDispose(bool) override;
    ErrCode deleteItemInternal(IBaseObject* key, IBaseObject** obj, bool& deleted);

private:
    void releaseRefOnChildren();
    ErrCode enumerate(const std::function<IBaseObject*(const BaseObjectPair&)>& chooseElement, IList** list);

    friend class BaseDictIterableImpl;
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(DictImpl)

END_NAMESPACE_OPENDAQ
