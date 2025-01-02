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
#include <coretypes/coretypes.h>
#include <functional>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

enum class ResolveStatus
{
    Unresolved,
    Resolved,
    Failed
};

enum class RefType
{
    Value,
    Property,
    Argument,
    Func,
    SelectedValue,
    PropertyNames
};

using GetReferenceEvent = std::function<BaseObjectPtr(std::string, RefType, int, std::string&, bool)>;

class BaseNode
{
public:
    CoreType resultType;

    BaseNode();
    virtual ~BaseNode() = default;
    virtual BaseObjectPtr getResult() = 0;

    virtual int visit(const std::function<int(BaseNode* node)>& visitFunc);
    virtual int resolveReference(bool lock);

    virtual std::unique_ptr<BaseNode> clone(GetReferenceEvent refCall) = 0;

    bool matchesType(CoreType otherType) const;
protected:
    static inline bool oneTypeBelongsToNonBasicTypes(CoreType ct1, CoreType ct2);
    static CoreType deduceCommonType(CoreType ct1, CoreType ct2);
};

class RefNode : public BaseNode
{
public:
    BaseObjectPtr refObject;
    std::string refStr;
    std::string postRef;
    int argIndex;

    GetReferenceEvent onResolveReference;

    RefNode(std::string refStr, RefType refType);
    RefNode(std::string refStr, int argIndex, RefType refType);
    explicit RefNode(int argIndex);

    BaseObjectPtr getResult() override;
    int resolveReference(bool lock) override;
    std::unique_ptr<BaseNode> clone(GetReferenceEvent refCall) override;

    void useAsArgument(RefNode* node);

protected:
    RefType refType;
    ResolveStatus resolveStatus;
};

class PropFuncNode : public BaseNode
{
public:
    PropFuncNode(std::unique_ptr<RefNode> refNode, std::unique_ptr<std::vector<std::unique_ptr<BaseNode>>> params);
    BaseObjectPtr getResult() override;
    std::unique_ptr<BaseNode> clone(GetReferenceEvent refCall) override;
private:
    std::unique_ptr<RefNode> refNode;
    std::unique_ptr<std::vector<std::unique_ptr<BaseNode>>> params;
};

template <class T, CoreType CT>
class ConstNode : public BaseNode
{
public:
    T value;
    ConstNode(T value);
    BaseObjectPtr getResult() override;
    std::unique_ptr<BaseNode> clone(GetReferenceEvent refCall) override;
};

using FloatConstNode = ConstNode<Float, ctFloat>;
using IntConstNode = ConstNode<Int, ctInt>;
using BoolConstNode = ConstNode<Bool, ctBool>;
using StringConstNode = ConstNode<StringPtr, ctString>;

class BinaryNode : public BaseNode
{
public:
    std::unique_ptr<BaseNode> leftNode;
    std::unique_ptr<BaseNode> rightNode;

    int visit(const std::function<int(BaseNode* node)>& visitFunc) override;
};

template <BinOperationType O>
class BinaryOpNode : public BinaryNode
{
public:
    BaseObjectPtr getResult() override;
    std::unique_ptr<BaseNode> clone(GetReferenceEvent refCall) override;
};

class UnaryNode : public BaseNode
{
public:
    std::unique_ptr<BaseNode> expNode = nullptr;

    int visit(const std::function<int(BaseNode* node)>& visitFunc) override;
};

template <UnaryOperationType O>
class UnaryOpNode : public UnaryNode
{
public:
    BaseObjectPtr getResult() override;
    std::unique_ptr<BaseNode> clone(GetReferenceEvent refCall) override;
};

class IfNode : public BaseNode
{
public:
    std::unique_ptr<BaseNode> condNode;
    std::unique_ptr<BaseNode> trueNode;
    std::unique_ptr<BaseNode> falseNode;

    int visit(const std::function<int(BaseNode* node)>& visitFunc) override;
    BaseObjectPtr getResult() override;
    std::unique_ptr<BaseNode> clone(GetReferenceEvent refCall) override;
};

class SwitchNode : public BaseNode
{
public:
    std::unique_ptr<std::vector<std::unique_ptr<BaseNode>>> valueNodes;
    std::unique_ptr<BaseNode> varNode;
    SwitchNode(std::unique_ptr<BaseNode> varNode, std::unique_ptr<std::vector<std::unique_ptr<BaseNode>>> valueNodes);

    int visit(const std::function<int(BaseNode* node)>& visitFunc) override;
    BaseObjectPtr getResult() override;
    std::unique_ptr<BaseNode> clone(GetReferenceEvent refCall) override;
};

class ListNode : public BaseNode
{
public:
    ListNode(std::unique_ptr<std::vector<std::unique_ptr<BaseNode>>> elements);
    int visit(const std::function<int(BaseNode* node)>& visitFunc) override;
    BaseObjectPtr getResult() override;
    std::unique_ptr<BaseNode> clone(GetReferenceEvent refCall) override;
private:
    std::unique_ptr<std::vector<std::unique_ptr<BaseNode>>> elements;
};

class UnitNode : public BaseNode
{
public:
    std::unique_ptr<std::vector<std::unique_ptr<BaseNode>>> unitParams;

    UnitNode(std::unique_ptr<std::vector<std::unique_ptr<BaseNode>>> unitParams);
    int visit(const std::function<int(BaseNode* node)>& visitFunc) override;
    BaseObjectPtr getResult() override;
    std::unique_ptr<BaseNode> clone(GetReferenceEvent refCall) override;
};

// -------- ConstNode ----------
template <class T, CoreType CT>
ConstNode<T, CT>::ConstNode(T value)
    : value(value)
{
}

template <class T, CoreType CT>
BaseObjectPtr ConstNode<T, CT>::getResult()
{
    return BaseObjectPtr(value);
}

template <class T, CoreType CT>
std::unique_ptr<BaseNode> ConstNode<T, CT>::clone(GetReferenceEvent refCall)
{
    return std::make_unique<ConstNode<T, CT>>(value);
}

// -------- BinaryOpNode ----------
template <BinOperationType O>
BaseObjectPtr BinaryOpNode<O>::getResult()
{
    assert(leftNode != nullptr);
    assert(rightNode != nullptr);
    typename BinOperationToStdOp<O>::op o{};
    return o(leftNode->getResult(), rightNode->getResult());
}

template <BinOperationType O>
std::unique_ptr<BaseNode> BinaryOpNode<O>::clone(GetReferenceEvent refCall)
{
    auto node = std::make_unique<BinaryOpNode<O>>();
    node->leftNode = leftNode->clone(refCall);
    node->rightNode = rightNode->clone(refCall);
    return node;
}

// UnaryOpNode

template <UnaryOperationType O>
BaseObjectPtr UnaryOpNode<O>::getResult()
{
    assert(expNode != nullptr);
    typename UnaryOperationToStdOp<O>::op o{};
    return o(expNode->getResult());
}

template <UnaryOperationType O>
std::unique_ptr<BaseNode> UnaryOpNode<O>::clone(GetReferenceEvent refCall)
{
    auto node = std::make_unique<UnaryOpNode<O>>();
    node->expNode = expNode->clone(refCall);
    return node;
}

END_NAMESPACE_OPENDAQ
