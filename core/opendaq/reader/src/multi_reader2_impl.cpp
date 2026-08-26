#include <coreobjects/ownable_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/validation.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/multi_reader2_impl.h>
#include <opendaq/tags_private_ptr.h>

#include <algorithm>

BEGIN_NAMESPACE_OPENDAQ

MultiReader2Impl::MultiReader2Impl(MultiReader2InputType inputType)
    : inputType(inputType)
{
}

std::vector<MultiReader2Impl::Slot>::iterator MultiReader2Impl::findSlot(const StringPtr& inputId)
{
    return std::find_if(slots.begin(), slots.end(), [&inputId](const Slot& slot) { return slot.inputId == inputId; });
}

ErrCode MultiReader2Impl::addInput(IComponent* input)
{
    OPENDAQ_PARAM_NOT_NULL(input);

    const auto component = ComponentPtr::Borrow(input);
    const auto listener = this->thisPtr<InputPortNotificationsPtr>();

    std::scoped_lock lock(mutex);

    if (!context.assigned())
        context = component.getContext();

    if (findSlot(component.getGlobalId()) != slots.end())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_DUPLICATEITEM, R"(Input "%s" was already added)", component.getGlobalId().getCharPtr());

    if (auto signal = component.asPtrOrNull<ISignal>(); signal.assigned())
    {
        if (inputType != MultiReader2InputType::Signals)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Reader accepts input ports, cannot add a signal");

        auto port = InputPort(context, nullptr, fmt::format("multi_reader_signal_{}", signal.getLocalId()));
        port.getTags().asPtr<ITagsPrivate>().add("MultiReaderInternalPort");
        port.setListener(listener);
        port.setNotificationMethod(PacketReadyNotification::SameThread);
        port.connect(signal);

        slots.push_back({port, signal.getGlobalId()});
    }
    else if (auto port = component.asPtrOrNull<IInputPortConfig>(); port.assigned())
    {
        if (inputType != MultiReader2InputType::Ports)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Reader accepts signals, cannot add an input port");

        if (!portBinder.assigned())
            portBinder = PropertyObject();
        port.asPtr<IOwnable>().setOwner(portBinder);

        port.setListener(listener);
        port.setNotificationMethod(PacketReadyNotification::SameThread);

        slots.push_back({port, port.getGlobalId()});
    }
    else
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Input is neither a signal nor an input port");
    }

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::removeInput(IComponent* input)
{
    OPENDAQ_PARAM_NOT_NULL(input);

    const auto component = ComponentPtr::Borrow(input);

    std::scoped_lock lock(mutex);

    const auto it = findSlot(component.getGlobalId());
    if (it == slots.end())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, R"(Input "%s" was not added to the reader)", component.getGlobalId().getCharPtr());

    // Internal ports are torn down; adopted external ports only stop notifying us
    if (inputType == MultiReader2InputType::Signals)
        it->port.disconnect();
    else
        it->port.setListener(nullptr);

    slots.erase(it);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::acceptsSignal(IInputPort* /*port*/, ISignal* /*signal*/, Bool* accept)
{
    OPENDAQ_PARAM_NOT_NULL(accept);

    *accept = True;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::connected(IInputPort* /*port*/)
{
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::disconnected(IInputPort* /*port*/)
{
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReader2Impl::packetReceived(IInputPort* /*port*/)
{
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
