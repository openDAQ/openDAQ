#include "app_signal.h"
#include <opendaq/sample_type_traits.h>
#include <iomanip>
#include <iostream>

BEGIN_NAMESPACE_OPENDAQ

bool AppSignal::processCommand(BaseObjectPtr& signal, const std::vector<std::string>& command)
{
    if (command.empty())
        return false;
    
    if (command[0] == "Print")
        return print(signal, command);
    if (command[0] == "Help")
        return help();
    if (command[0] == "Set")
        return set(signal, command);
    if (command[0] == "Select")
        return select(signal, command);
    if (command[0] == "List")
        return list(signal, command);

    return false;
}

bool AppSignal::print(const SignalPtr& signal, const std::vector<std::string>& command)
{
    if (command.size() != 2)
        return false;

    if (command[1] == "Name")
    {
        if (!signal.getDescriptor().assigned())
            return false;

        const auto name = signal.getDescriptor().getName();
        if (name.assigned() && name.getLength() > 0)
            std::cout << "Name: " + name << std::endl;
        else
            std::cout << "Not available." << std::endl;

        return true;
    }

    if (command[1] == "Id")
    {
        const auto id = signal.getGlobalId();
        std::cout << "ID: " + id << std::endl;

        return true;
    }

    if (command[1] == "Descriptor")
    {
        if (!signal.getDescriptor().assigned())
            return false;

        printDataDescriptor(signal.getDescriptor(), 4, 0);
        std::cout << std::endl;
        return true;
    }

    if (command[1] == "Public")
    {
        if (signal.getPublic())
            std::cout << "Public: true" << std::endl;
        else
            std::cout << "Public: false" << std::endl;
        return true;
    }

    if (command[1] == "Active")
    {
        if (signal.getActive())
            std::cout << "Active: true" << std::endl;
        else
            std::cout << "Active: false" << std::endl;
        return true;
    }

    if (command[1] == "domain-signal")
    {
        if (!signal.getDomainSignal().assigned())
        {
            std::cout << "Domain signal not available." << std::endl;
        }
        else
        {
            const auto sig = signal.getDomainSignal();
            std::string name = sig.getDescriptor().getName().assigned() ? sig.getDescriptor().getName() : "";
            std::string id = sig.getGlobalId();
            std::cout << "Name : " << name << ", Unique ID : " << id << std::endl;
        }
        return true;
    }

    return false;
}

bool AppSignal::list(const SignalPtr& signal, const std::vector<std::string>& command)
{
    if (command.size() != 2)
        return false;

    if (command[1] == "related")
    {
        int cnt = 0;
        for (auto sig : signal.getRelatedSignals())
        {
            std::string name = sig.getDescriptor().getName().assigned() ? sig.getDescriptor().getName() : "";
            std::string id = sig.getGlobalId();
            std::cout << "[" << std::to_string(cnt) << "] Name: " << name << ", Unique ID: " << id << std::endl;
            cnt++;
        }
        return true;
    }

    return false;
}

bool AppSignal::set(const SignalPtr& signal, const std::vector<std::string>& command)
{
    if (command.size() != 3)
        return false;

    bool value;
    if (command[2] == "true")
        value = true;
    else if (command[2] == "false")
        value = false;
    else
        return false;

    if (command[1] == "Public")
    {
        signal.setPublic(value);
        return true;
    }

    if (command[1] == "Active")
    {
        signal.setActive(value);
        return true;
    }

    return false;
}

bool AppSignal::select(BaseObjectPtr& signal, const std::vector<std::string>& command)
{
    const auto signalPtr = signal.asPtr<ISignal>();
    if (command.size() == 2 && command[1] == "domain-signal")
    {
        if (signalPtr.getDomainSignal().assigned())
            signal = signalPtr.getDomainSignal();
        else
            std::cout << "No domain signal." << std::endl;
        return true;
    }

    if (command.size() != 3)
        return false;

    size_t index;
    try
    {
        index = std::stoi(command[2]);
    }
    catch (...)
    {
        return false;
    }

    if (command[1] == "related")
    {
        if (signalPtr.getRelatedSignals().getCount() > index)
            signal = signalPtr.getRelatedSignals()[index];
        else
            std::cout << "Index out of bounds." << std::endl;
        return true;
    }

    return false;
}

bool AppSignal::help()
{
    std::cout << std::setw(25) << std::left << "print <info>"
              << "Prints the value of the given <info> parameter. Available information:" << std::endl
              << std::setw(25) << ""
              << "[name, id, description, tags, descriptor, active, public, domain-signal]" << std::endl
              << std::endl;

    std::cout << std::setw(25) << std::left << "set <property> <state>"
              << "Sets the value of <property> to <state>. Valid values for <property> are" << std::endl
              << std::setw(25) << ""
              << "[active, public]. Valid values for <state> are [true, false]" << std::endl
              << std::endl;

    std::cout << std::setw(25) << std::left << "select domain-signal"
              << "Moves the application to the domain signal if available." << std::endl
              << std::endl;

    std::cout << std::setw(25) << std::left << "select related <index>"
              << "Moves the application to the related signal at the given <index>." << std::endl
              << std::endl;

    std::cout << std::setw(25) << std::left << "list related"
              << "Lists signals related to the currently selected signal." << std::endl
              << std::endl;

    return true;
}

void AppSignal::printDataDescriptor(const DataDescriptorPtr& descriptor, std::streamsize indent, int indentLevel)
{
    std::cout << std::setw(indent * indentLevel) << ""
              << "Data descriptor : {" << std::endl;
    
    const std::string name = descriptor.getName().assigned() ? descriptor.getName() : "";
    if (!name.empty())
    {
        std::cout << std::setw(indent * (indentLevel + 1)) << ""
                  << "Name : \"" << name << "\"," << std::endl;
    }
    

    if (descriptor.getDimensions().assigned())
    {
        printDimensions(descriptor.getDimensions(), indent, indentLevel + 1);
        std::cout << "," << std::endl;
    }

    std::cout << std::setw(indent * (indentLevel + 1)) << ""
              << "Sample type : " << convertSampleTypeToString(descriptor.getSampleType()) << "," << std::endl;

    if (descriptor.getUnit().assigned())
    {
        printUnit(descriptor.getUnit(), indent, indentLevel + 1);
        std::cout << "," << std::endl;
    }

    if (descriptor.getValueRange().assigned())
    {
        const auto range = descriptor.getValueRange();
        std::cout << std::setw(indent * (indentLevel + 1)) << ""
                  << "Value range : [" << std::to_string(range.getLowValue().getFloatValue()) << ", "
                  << std::to_string(range.getHighValue().getFloatValue()) << "]," << std::endl;
    }

    if (descriptor.getRule().assigned())
    {
        printDataRule(descriptor.getRule(), indent, indentLevel + 1);
        std::cout << "," << std::endl;
    }

    
    const std::string absRef = descriptor.getOrigin().assigned() ? descriptor.getOrigin() : "";
    if (!absRef.empty())
    {
        std::cout << std::setw(indent * (indentLevel + 1)) << ""
                  << "Absolute reference : \"" << absRef << "\"," << std::endl;
    }

    if (descriptor.getTickResolution().assigned())
    {
        const auto resolution = descriptor.getTickResolution();
        std::cout << std::setw(indent * (indentLevel + 1)) << ""
                  << "Resolution : " << std::to_string(resolution.getNumerator()) << " / "
                  << std::to_string(resolution.getDenominator()) << "," << std::endl;
    }

    if (descriptor.getPostScaling().assigned())
    {
        printScaling(descriptor.getPostScaling(), indent, indentLevel + 1);
        std::cout << "," << std::endl;
    }

    const auto list = descriptor.getStructFields();
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        printDataDescriptor(*it, indent, indentLevel + 1);

        if (std::next(it) != list.begin())
            std::cout << ",";
        std::cout << std::endl;
    }


    if (descriptor.getMetadata().assigned())
    {
        printMetadata(descriptor.getMetadata(), indent, indentLevel + 1);
        std::cout << "," << std::endl;
    }

    std::cout << std::setw(indent * indentLevel) << "" << "}";
}

void AppSignal::printDimensions(const ListPtr<IDimension>& dimensions, std::streamsize indent, int indentLevel)
{
    std::cout << std::setw(indent * indentLevel) << "" << "Dimensions : [" << std::endl;
    
    for (auto it = dimensions.begin(); it != dimensions.end(); ++it)
    {
        std::cout << std::setw(indent * (indentLevel + 1)) << "" << "{";

        std::string name = (*it).getName().assigned() ? (*it).getName() : "";
        if (!name.empty())
        {
            std::cout << std::setw(indent * (indentLevel + 2)) << ""
                      << "Name : \"" << name << "\",";
        }

        if ((*it).getUnit().assigned())
        {
            printUnit((*it).getUnit(), indent, indentLevel + 2);
            std::cout << "," << std::endl;
        }

        if ((*it).getRule().assigned())
        {
            printDimensionRule((*it).getRule(), indent, indentLevel + 2);
            std::cout << std::endl;
        }

        std::cout << std::setw(indent * (indentLevel + 1)) << "" << "}";
        if (std::next(it) != dimensions.begin())
            std::cout << ",";
        std::cout << std::endl;
    }

    std::cout << std::setw(indent * indentLevel) << "" << "]";
}

void AppSignal::printDataRule(const DataRulePtr& rule, std::streamsize indent, int indentLevel)
{
    auto params = rule.getParameters();
    if (!params.assigned())
        return;

    std::cout << std::setw(indent * indentLevel) << ""
              << "Dimension rule : {" << std::endl;

    switch (rule.getType())
    {
        case DataRuleType::Other:
            std::cout << std::setw((indentLevel + 1) * indent) << "" << "Type : Other," << std::endl;
            break;
        case DataRuleType::Linear:
            std::cout << std::setw((indentLevel + 1) * indent) << "" << "Type : Linear," << std::endl;
            break;
        case DataRuleType::Constant:
            std::cout << std::setw((indentLevel + 1) * indent) << "" << "Type : Constant," << std::endl;
            break;
        case DataRuleType::Explicit:
            std::cout << std::setw((indentLevel + 1) * indent) << "" << "Type : Explicit," << std::endl;
            break;
    }
    const auto list = params.getKeys();
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        std::cout << std::setw((indentLevel + 1) * indent) << "" << *it << " : " << params.get(*it).toString();
        if (std::next(it) != list.begin())
            std::cout << ",";
        std::cout << std::endl;
    }
    std::cout << std::setw(indent * indentLevel) << "" << "}";
}

void AppSignal::printDimensionRule(const DimensionRulePtr& rule, std::streamsize indent, int indentLevel)
{
    const auto params = rule.getParameters();
    if (!params.assigned())
        return;

    std::cout << std::setw(indent * indentLevel) << "" << "Dimension rule : {" << std::endl;

    switch (rule.getType())
    {
        case DimensionRuleType::Other:
            std::cout << std::setw((indentLevel + 1) * indent) << "" << "Type : Other," << std::endl;
            break;
        case DimensionRuleType::Linear:
            std::cout << std::setw((indentLevel + 1) * indent) << "" << "Type : Linear," << std::endl;
            break;
        case DimensionRuleType::Logarithmic:
            std::cout << std::setw((indentLevel + 1) * indent) << "" << "Type : Logarithmic," << std::endl;
            break;
        case DimensionRuleType::List:
            std::cout << std::setw((indentLevel + 1) * indent) << "" << "Type : List," << std::endl;
            break;
    }
    const auto list = params.getKeys();
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        std::cout << std::setw((indentLevel + 1) * indent) << "" << *it << " : " << params.get(*it).toString();
        if (std::next(it) != list.begin())
            std::cout << ",";
        std::cout << std::endl;
    }
    std::cout << std::setw(indent * indentLevel) << "" << "}";
}

void AppSignal::printUnit(const UnitPtr& unit, std::streamsize indent, int indentLevel)
{
    std::cout << std::setw(indent * indentLevel) << ""
              << "Unit : {" << std::endl;

    const std::string id = std::to_string(unit.getId());
    std::cout << std::setw(indent * (indentLevel + 1)) << ""
              << "ID : \"" << id << "\"," << std::endl;

    const std::string symbol = unit.getSymbol().assigned() ? unit.getSymbol() : "";
    if (!symbol.empty())
    {
        std::cout << std::setw(indent * (indentLevel + 1)) << ""
                  << "Symbol : \"" << symbol << "\"," << std::endl;
    }

    const std::string name = unit.getName().assigned() ? unit.getName() : "";
    if (!name.empty())
    {
        std::cout << std::setw(indent * (indentLevel + 1)) << ""
                  << "Name : \"" << name << "\"," << std::endl;
    }

    const std::string quantity = unit.getQuantity().assigned() ? unit.getQuantity() : "";
    if (!quantity.empty())
    {
        std::cout << std::setw(indent * (indentLevel + 1)) << ""
                  << "Quantity : \"" << quantity << "\"" << std::endl;
    }
    
    std::cout << std::setw(indent * indentLevel) << "" << "}";
}

void AppSignal::printScaling(const ScalingPtr& scaling, std::streamsize indent, int indentLevel)
{
    const auto params = scaling.getParameters();
    if (!params.assigned())
        return;

    std::cout << std::setw(indent * indentLevel) << ""
              << "Post scaling : {" << std::endl;

    switch (scaling.getType())
    {
        case ScalingType::Other:
            std::cout << std::setw((indentLevel + 1) * indent) << "" << "Type : Other," << std::endl;
            break;
        case ScalingType::Linear:
            std::cout << std::setw((indentLevel + 1) * indent) << "" << "Type : Linear," << std::endl;
            break;
    }

    std::cout << std::setw(indent * (indentLevel + 1)) << ""
              << "Input data type : " << convertSampleTypeToString(scaling.getInputSampleType()) << "," << std::endl;

    std::cout << std::setw(indent * (indentLevel + 1)) << ""
              << "Output data type : " << convertScaledSampleTypeToString(scaling.getOutputSampleType()) << "," << std::endl;

    const auto list = params.getKeys();
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        std::cout << std::setw((indentLevel + 1) * indent) << "" << *it << " : " << params.get(*it).toString();
        if (std::next(it) != list.begin())
            std::cout << ",";
        std::cout << std::endl;
    }
    std::cout << std::setw(indent * indentLevel) << "" << "}";
}

void AppSignal::printTags(const TagsPtr& tags, std::streamsize indent, int indentLevel)
{
    std::cout << std::setw(indent * indentLevel) << "" << "Tags : [";
    const auto list = tags.getList();
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        std::cout << *it;
        if (std::next(it) != list.begin())
            std::cout << ",";
    }
    std::cout << "]";
}

void AppSignal::printMetadata(const DictPtr<IString, IString>& metadata, std::streamsize indent, int indentLevel)
{
    std::cout << std::setw(indent * indentLevel) << "" << "Metadata : {" << std::endl;

    const auto list = metadata.getKeys();
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        std::cout << std::setw((indentLevel + 1) * indent) << *it << " : " << metadata.get(*it);
        if (std::next(it) != list.begin())
            std::cout << ",";
        std::cout << std::endl;
    }

    std::cout << std::setw(indent * indentLevel) << "" << "}";
}

END_NAMESPACE_OPENDAQ
