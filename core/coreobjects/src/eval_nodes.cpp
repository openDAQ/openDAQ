#include <coreobjects/eval_nodes.h>
#include <bitset>
#include <memory>
#include <utility>
#include <coreobjects/unit_factory.h>

BEGIN_NAMESPACE_OPENDAQ

// -------- BaseNode ----------
BaseNode::BaseNode()
    : resultType(ctUndefined)
{
}

int BaseNode::visit(const std::function<int(BaseNode* node)>& visitFunc)
{
    assert(visitFunc);
    return visitFunc(this);
}

int BaseNode::resolveReference()
{
    return 0;
}

// -------- RefNode ----------
RefNode::RefNode(std::string refStr, RefType refType)
    : refStr(std::move(refStr))
    , argIndex(-1)
    , refType(refType)
    , resolveStatus(ResolveStatus::Unresolved)
{
}

RefNode::RefNode(int argIndex)
    : argIndex(argIndex)
    , refType(RefType::Argument)
    , resolveStatus(ResolveStatus::Unresolved)
{
}

RefNode::RefNode(std::string refStr, int argIndex, RefType refType)
    : refStr(std::move(refStr))
    , argIndex(argIndex)
    , refType(refType)
    , resolveStatus(ResolveStatus::Unresolved)
{
}

BaseObjectPtr RefNode::getResult()
{
    return refObject;
}

int RefNode::resolveReference()
{
    if (resolveStatus == ResolveStatus::Resolved && refType != RefType::Value && refType != RefType::Func && refType != RefType::SelectedValue)
    {
        return 0;
    }

    assert(onResolveReference);

    try
    {
        refObject = onResolveReference(refStr, refType, argIndex, postRef);
        if (refObject.assigned())
        {
            resolveStatus = ResolveStatus::Resolved;
            return 0;
        }

        resolveStatus = ResolveStatus::Failed;
        return 1;
    }
    catch (...)
    {
        refObject = BaseObjectPtr(nullptr);
        return 1;
    }
}

std::unique_ptr<BaseNode> RefNode::clone(GetReferenceEvent refCall)
{
    auto node = std::make_unique<RefNode>(refStr, argIndex, refType);
    node->onResolveReference = refCall;
    return node;
}

void RefNode::useAsArgument(RefNode* node)
{
    this->refStr = node->refStr;
    this->refType = node->refType;
}

// -------- PropFuncNode ----------
PropFuncNode::PropFuncNode(std::unique_ptr<RefNode> refNode,
                           std::unique_ptr<std::vector<std::unique_ptr<BaseNode>>> params)
    : refNode(std::move(refNode))
    , params(std::move(params))
{
}

BaseObjectPtr PropFuncNode::getResult()
{
    return nullptr;
}

std::unique_ptr<BaseNode> PropFuncNode::clone(GetReferenceEvent refCall)
{
    auto newRefNode = std::unique_ptr<RefNode>{static_cast<RefNode*>(refNode->clone(refCall).release())};
    auto newParams = std::make_unique<std::vector<std::unique_ptr<BaseNode>>>();
    for (auto& el : *params)
        newParams->push_back(el->clone(refCall));

    return std::make_unique<PropFuncNode>(std::move(newRefNode), std::move(newParams));
}

// -------- BinaryNode ----------
int BinaryNode::visit(const std::function<int(BaseNode* node)>& visitFunc)
{
    int r;
    r = leftNode->visit(visitFunc);
    if (r)
        return r;
    r = BaseNode::visit(visitFunc);
    if (r)
        return r;
    r = rightNode->visit(visitFunc);
    return r;
}

// -------- UnaryNode ----------
int UnaryNode::visit(const std::function<int(BaseNode* node)>& visitFunc)
{
    int r;
    r = BaseNode::visit(visitFunc);
    if (r)
        return r;
    r = expNode->visit(visitFunc);
    return r;
}

// -------- IfNode ----------
int IfNode::visit(const std::function<int(BaseNode* node)>& visitFunc)
{
    if (int r = condNode->visit(visitFunc))
        return r;
    if (int r = trueNode->visit(visitFunc))
        return r;
    if (int r = falseNode->visit(visitFunc))
        return r;
    return BaseNode::visit(visitFunc);
}

BaseObjectPtr IfNode::getResult()
{
    assert(condNode != nullptr && trueNode != nullptr && falseNode != nullptr);

    auto condResult = condNode->getResult();

    if (Bool(condResult))
        return trueNode->getResult();

    return falseNode->getResult();
}

std::unique_ptr<BaseNode> IfNode::clone(GetReferenceEvent refCall)
{
    auto node = std::make_unique<IfNode>();
    node->condNode = condNode->clone(refCall);
    node->trueNode = trueNode->clone(refCall);
    node->falseNode = falseNode->clone(refCall);
    return node;
}

// -------- SwitchNode ----------
SwitchNode::SwitchNode(std::unique_ptr<BaseNode> varNode,
                       std::unique_ptr<std::vector<std::unique_ptr<BaseNode>>> valueNodes)
    : valueNodes(std::move(valueNodes))
    , varNode(std::move(varNode))
{
}

int SwitchNode::visit(const std::function<int(BaseNode* node)>& visitFunc)
{
    assert(valueNodes != nullptr && valueNodes->size() >= 2);

    if (int r = varNode->visit(visitFunc))
        return r;

    for (auto& node : *valueNodes)
    {
        if (int r = node->visit(visitFunc))
            return r;
    }

    return BaseNode::visit(visitFunc);
}

BaseObjectPtr SwitchNode::getResult()
{
    assert(valueNodes != nullptr && valueNodes->size() >= 2);

    auto varResult = varNode->getResult();

    for (size_t i = 0; i < valueNodes->size(); i += 2)
    {
        auto caseResult = valueNodes->at(i)->getResult();
        if (varResult == caseResult)
            return valueNodes->at(i + 1)->getResult();
    }

    if (valueNodes->size() % 2 == 1)
        return valueNodes->back()->getResult();

    throw std::logic_error("No value matches");
}

std::unique_ptr<BaseNode> SwitchNode::clone(GetReferenceEvent refCall)
{
    auto newElements = std::make_unique<std::vector<std::unique_ptr<BaseNode>>>();

    for (auto& el : *valueNodes)
        newElements->push_back(el->clone(refCall));

    return std::make_unique<SwitchNode>(varNode->clone(refCall), std::move(newElements));
}

// -------- ListNode ----------
ListNode::ListNode(std::unique_ptr<std::vector<std::unique_ptr<BaseNode>>> elements)
    : elements(std::move(elements))
{
    assert(this->elements != nullptr);
}

int ListNode::visit(const std::function<int(BaseNode* node)>& visitFunc)
{
    for (auto& el : *elements)
    {
        if (int r = el->visit(visitFunc))
            return r;
    }
    return BaseNode::visit(visitFunc);
}

BaseObjectPtr ListNode::getResult()
{
    auto list = List<IBaseObject>();
    for (auto& el : *elements)
        list.pushBack(el->getResult());
    return list;
}

std::unique_ptr<BaseNode> ListNode::clone(GetReferenceEvent refCall)
{
    auto newElements = std::make_unique<std::vector<std::unique_ptr<BaseNode>>>();

    for (auto& el : *elements)
        newElements->push_back(el->clone(refCall));

    return std::make_unique<ListNode>(std::move(newElements));
}

// -------- UnitNode ----------

UnitNode::UnitNode(std::unique_ptr<std::vector<std::unique_ptr<BaseNode>>> unitParams)
    : unitParams(std::move(unitParams))
{
    assert(this->unitParams != nullptr);
    assert(this->unitParams->size() > 0);
}

int UnitNode::visit(const std::function<int(BaseNode* node)>& visitFunc)
{
    for (auto& el : *unitParams)
    {
        if (int r = el->visit(visitFunc))
            return r;
    }
    return BaseNode::visit(visitFunc);
}

BaseObjectPtr UnitNode::getResult()
{
    auto unit = UnitBuilder();
    unit.setSymbol((*unitParams)[0]->getResult());

    if (unitParams->size() > 1)
        unit.setName((*unitParams)[1]->getResult());
    if (unitParams->size() > 2)
        unit.setQuantity((*unitParams)[2]->getResult());
    if (unitParams->size() > 3)
        unit.setId((*unitParams)[3]->getResult());
    
    return unit.build();
}

std::unique_ptr<BaseNode> UnitNode::clone(GetReferenceEvent refCall)
{
    auto newUnitParams = std::make_unique<std::vector<std::unique_ptr<BaseNode>>>();

    for (auto& el : *unitParams)
        newUnitParams->push_back(el->clone(refCall));

    return std::make_unique<UnitNode>(std::move(newUnitParams));
}

END_NAMESPACE_OPENDAQ
