// The test-local owner of Suite 2

#include "multi_reader2_harness.h"
#include "multi_reader2_harness_state.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <random>
#include <thread>

namespace daq::mr2test
{


using namespace detail;

// ---------------------------------------------------------------------------------------------------------------------
// Suite 2 owner

#define MR2_OWNER_REACTS(Name, Body)      \
    OwnerReacts OwnerReacts::Name() const \
    {                                     \
        OwnerReacts r = *this;            \
        Body;                             \
        return r;                         \
    }

MR2_OWNER_REACTS(cachesDescriptors, r.cache = true)
MR2_OWNER_REACTS(cachesUnlessError, r.cache = true; r.cacheUnderError = false)
MR2_OWNER_REACTS(setsComponentStatus, r.componentStatus = true)
MR2_OWNER_REACTS(warnsWhenOnlyDisconnected, r.componentStatus = true; r.warnDisconnected = true)
MR2_OWNER_REACTS(removesDisconnectedPorts, r.removeDisconnected = true)

#undef MR2_OWNER_REACTS

void MultiReader2Test::ownerReacts(const OwnerReacts& reacts)
{
    std::scoped_lock lock(owner->mutex);
    owner->reacts = reacts;
}

void MultiReader2Test::offerFreePort(Input in)
{
    ASSERT_TRUE(config->ports) << "a free port needs the ports fixture";
    ASSERT_EQ(inputs.count(in.slot), 0u) << "in" << in.slot << " exists";
    config->unconnected.insert(in.slot);
    config->blockListener[in.slot] = PacketReadyNotification::Scheduler;
    input(in.slot);
}

void MultiReader2Test::addPort(Input in)
{
    ASSERT_TRUE(config->ports) << "a port needs the ports fixture";
    ASSERT_EQ(inputs.count(in.slot), 0u) << "in" << in.slot << " exists";
    config->unconnected.insert(in.slot);
    input(in.slot);
}

void MultiReader2Test::removePort(Input in)
{
    auto& i = input(in.slot);
    ASSERT_TRUE(i.port.assigned()) << "removePort needs an input port";
    if (i.signal)
        i.port.disconnect();
    i.signal = nullptr;
    i.port.asPtr<IRemovable>().remove();
}

void MultiReader2Test::pauseRecording()
{
    owner->paused = true;
}

void MultiReader2Test::resumeRecording()
{
    owner->paused = false;
}

void MultiReader2Test::recorderDrain()
{
    drain(owner->paused ? ReadCall::Skip : ReadCall::ReadWithDomain);
}

void MultiReader2Test::expectComponentStatus(ComponentStatus level, const std::string& message)
{
    std::scoped_lock lock(owner->mutex);
    EXPECT_EQ(owner->status, level) << "the owner's component status";
    EXPECT_EQ(owner->message, message) << "the owner's component status message";
}

bool MultiReader2Test::acceptsByRule(int slot, const DataDescriptorPtr& desc)
{
    std::scoped_lock lock(owner->mutex);
    switch (*owner->rule)
    {
        case AcceptsRule::UnitV:
            return unitOf(desc) == "V";
        case AcceptsRule::Power:
            return slot != 1 || unitOf(desc) == "V" || unitOf(desc).empty();
        case AcceptsRule::Sum:
            return isScalarNumeric(desc) && (owner->referenceUnit.empty() || unitOf(desc) == owner->referenceUnit);
    }
    return true;
}

void MultiReader2Test::ownerOnStatus(const MultiReader2StatusPtr& status)
{
    std::scoped_lock lock(owner->mutex);
    const auto level = status.getStatus();
    if (owner->rule == AcceptsRule::Sum)
    {
        const auto main = status.getMainInput();
        const std::string unit = main.assigned() ? unitOf(status.getInputStatus(main).getDescriptor()) : std::string();
        owner->rejudge = owner->rejudge || unit != owner->referenceUnit;
        owner->referenceUnit = unit;
    }
    if (owner->reacts.cache && (owner->reacts.cacheUnderError || level != Error))
        for (const auto& i : status.getInputs())
            if (i.getDescriptor().assigned())
                owner->cache[slotOf(i.getInput())] = i.getDescriptor();
    if (owner->reacts.componentStatus)
    {
        bool onlyDisconnected = true;
        for (const auto& i : status.getInputs())
            if (i.getError() != InputError::None && i.getError() != InputError::Disconnected)
                onlyDisconnected = false;
        owner->status = level == Error && owner->reacts.warnDisconnected && onlyDisconnected ? Warning : level;
        owner->message = status.getStatusMessage().toStdString();
    }
}

void MultiReader2Test::ownerActs(const MultiReader2StatusPtr& status)
{
    OwnerReacts reacts;
    {
        std::scoped_lock lock(owner->mutex);
        if (owner->acting)
            return;
        owner->acting = true;
        reacts = owner->reacts;
    }
    std::vector<int> disconnected;
    for (const auto& i : status.getInputs())
        if (i.getError() == InputError::Disconnected)
            disconnected.push_back(slotOf(i.getInput()));
    if (reacts.removeDisconnected && !disconnected.empty())
    {
        for (int slot : disconnected)
        {
            removePort(Input{slot});
            configure(same().remove(Input{slot}));
            inputStatus(Input{slot});
        }
    }
    if (takeOwnerRejudge())
        reevaluate();
    std::scoped_lock lock(owner->mutex);
    owner->acting = false;
}

bool MultiReader2Test::takeOwnerRejudge()
{
    std::scoped_lock lock(owner->mutex);
    return std::exchange(owner->rejudge, false);
}

}
