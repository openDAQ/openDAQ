#pragma once

#include <coretypes/ratio_ptr.h>
#include <opendaq/range_type.h>

#include <chrono>
#include <iostream>
#include <memory>

BEGIN_NAMESPACE_OPENDAQ

struct DomainInfo
{
    std::chrono::system_clock::time_point epoch;
    RatioPtr resolution;

    friend bool operator==(const DomainInfo& lhs, const DomainInfo& rhs)
    {
        if (!(lhs.epoch == rhs.epoch))
            return false;
        if (!(lhs.resolution.getNumerator() == rhs.resolution.getNumerator()))
            return false;
        if (!(lhs.resolution.getDenominator() == rhs.resolution.getDenominator()))
            return false;
        return true;
    }
    
    friend bool operator!=(const DomainInfo& lhs, const DomainInfo& rhs)
    {
        return !(lhs == rhs);
    }
};

inline std::ostream& operator<<(std::ostream& os, const DomainInfo& info)
{
    os << "DomainInfo{"
       << "epoch=" << info.epoch.time_since_epoch().count()
       << ", resolution="
       << info.resolution.getNumerator() << "/" << info.resolution.getDenominator()
       << "}";

    return os;
}

class DomainValue
{
public:
	explicit DomainValue(const DomainInfo& info) : domain(info) {}
    virtual ~DomainValue() = default;

    const DomainInfo& getDomain() const
    {
        return domain;
    }

	virtual std::unique_ptr<DomainValue> toCommonDomain(const DomainInfo& commonDomain) = 0;
	virtual std::unique_ptr<DomainValue> fromCommonDomain(const DomainInfo& regularDomain) = 0;
	
	virtual double getTime() = 0;
	
	friend bool operator<(const DomainValue& lhs, const DomainValue& rhs)
    {
        return lhs.compare(rhs) < 0;
    }

protected:
	DomainInfo domain;
private:
    virtual int compare(const DomainValue& other) const = 0;
};

template<typename Type>
class DomainValueImpl : public DomainValue
{
public:
	explicit DomainValueImpl(const DomainInfo& info, Type value) : DomainValue(info), value(value) {
        if constexpr (std::is_integral_v<Type>){
            std::cout << "DomainValueImpl(value=" << value << ", domain=" << info << ")" << std::endl;
        }
	}

    ~DomainValueImpl() override = default;

	std::unique_ptr<DomainValue> toCommonDomain(const DomainInfo& commonDomain) override
	{
	    // Offset of current domain in common domain ticks
        Int epochOffset = domain.epoch.time_since_epoch().count() - commonDomain.epoch.time_since_epoch().count();

        using SysPeriod = std::chrono::system_clock::period;
        Int scaleNumerator = SysPeriod::num * commonDomain.resolution.getDenominator();
        Int scaleDenominator = SysPeriod::num * commonDomain.resolution.getNumerator();
        Int offsetFromCommon = epochOffset * scaleNumerator / scaleDenominator;

        // tick_common = tick * multiplier
        Int multiplierNumerator = domain.resolution.getNumerator() * commonDomain.resolution.getDenominator();
        Int multiplierDenominator = domain.resolution.getDenominator() * commonDomain.resolution.getNumerator();
        Type valueScaledToCommon = static_cast<Type>(value * multiplierNumerator / static_cast<double>(multiplierDenominator));
	    Type valueInCommon = offsetFromCommon + valueScaledToCommon;
        
		return std::make_unique<DomainValueImpl<Type>>(commonDomain, valueInCommon);
	}
	
	std::unique_ptr<DomainValue> fromCommonDomain(const DomainInfo& regularDomain) override
	{
	    const auto& commonDomain = this->domain;
	    const auto& valueInCommon = this->value;

	    // Offset of regularDomain domain in common domain ticks
        Int epochOffset = regularDomain.epoch.time_since_epoch().count() - commonDomain.epoch.time_since_epoch().count();

        using SysPeriod = std::chrono::system_clock::period;
        Int scaleNumerator = SysPeriod::num * commonDomain.resolution.getDenominator();
        Int scaleDenominator = SysPeriod::num * commonDomain.resolution.getNumerator();
        Int offsetFromCommon = epochOffset * scaleNumerator / scaleDenominator;
	    Type valueScaledToCommon = valueInCommon - offsetFromCommon;

        // tick_common = tick * multiplier
        Int multiplierNumerator = regularDomain.resolution.getNumerator() * commonDomain.resolution.getDenominator();
        Int multiplierDenominator = regularDomain.resolution.getDenominator() * commonDomain.resolution.getNumerator();
        Type regularValue = static_cast<Type>(valueScaledToCommon * multiplierDenominator / static_cast<double>(multiplierNumerator));
	    
		return std::make_unique<DomainValueImpl<Type>>(regularDomain, regularValue);
	}
	
	Type getValue() const
	{
	    return value;
	}
	
	double getTime() override
	{
        using SysPeriod = std::chrono::system_clock::period;
	    return domain.epoch.time_since_epoch().count() * SysPeriod::num / static_cast<double>(SysPeriod::den)
            + value * domain.resolution.getNumerator() / static_cast<double>(domain.resolution.getDenominator());
	}
	
	int compare(const DomainValue& other) const override
	{
	    const auto* otherImpl = dynamic_cast<const DomainValueImpl<Type>*>(&other);
	    if (otherImpl == nullptr){
	        DAQ_THROW_EXCEPTION(InvalidParameterException, "Both DomainValue objects must be of the same type!");
	    }
	    if (otherImpl->domain != this->domain) // TODO: Implement equality operator
	        DAQ_THROW_EXCEPTION(InvalidParameterException, "Have to compare DomainValue objects in the same domain!");
	    
	    if (this->value > otherImpl->value)
	        return 1;
	    else if (this->value == otherImpl->value)
	        return 0;
	    else // this->value < otherImpl->value
	        return -1;
	}

private:
	Type value;
};

// TODO: Support RangeType64
template<>
class DomainValueImpl<RangeType64> final : public DomainValue
{
public:
    using RangeValue = RangeType64::Type;

    explicit DomainValueImpl(const DomainInfo& info, RangeType64 value) : DomainValue(info), value(value) {
        std::cout << "DomainValueImpl(value=" << value.start << ", domain=" << info << ")" << std::endl;
	}

    std::unique_ptr<DomainValue> toCommonDomain(const DomainInfo& commonDomain) override
    {
        // Offset of current domain in common domain ticks
        Int epochOffset = domain.epoch.time_since_epoch().count() - commonDomain.epoch.time_since_epoch().count();

        using SysPeriod = std::chrono::system_clock::period;
        Int scaleNumerator = SysPeriod::num * commonDomain.resolution.getDenominator();
        Int scaleDenominator = SysPeriod::num * commonDomain.resolution.getNumerator();
        RangeValue offsetFromCommon = static_cast<RangeValue>(epochOffset * scaleNumerator / scaleDenominator);

        // tick_common = tick * multiplier
        Int multiplierNumerator = domain.resolution.getNumerator() * commonDomain.resolution.getDenominator();
        Int multiplierDenominator = domain.resolution.getDenominator() * commonDomain.resolution.getNumerator();
        RangeValue startScaledToCommon = static_cast<RangeValue>(value.start * multiplierNumerator / static_cast<double>(multiplierDenominator));
        RangeValue endScaledToCommon = static_cast<RangeValue>(value.end * multiplierNumerator / static_cast<double>(multiplierDenominator));
	    
        RangeValue startInCommon = offsetFromCommon + startScaledToCommon;
        RangeValue endInCommon = value.end == -1
            ? static_cast<RangeValue>(-1)
            : offsetFromCommon + endScaledToCommon;
        
		return std::make_unique<DomainValueImpl<RangeType64>>(commonDomain, RangeType64{startInCommon, endInCommon});
    }

    std::unique_ptr<DomainValue> fromCommonDomain(const DomainInfo& regularDomain) override
    {
        const auto& commonDomain = this->domain;
	    const auto& valueInCommon = this->value;

	    // Offset of regularDomain domain in common domain ticks
        Int epochOffset = regularDomain.epoch.time_since_epoch().count() - commonDomain.epoch.time_since_epoch().count();

        using SysPeriod = std::chrono::system_clock::period;
        Int scaleNumerator = SysPeriod::num * commonDomain.resolution.getDenominator();
        Int scaleDenominator = SysPeriod::num * commonDomain.resolution.getNumerator();
        Int offsetFromCommon = epochOffset * scaleNumerator / scaleDenominator;

	    RangeValue startScaledToCommon = valueInCommon.start - offsetFromCommon;
	    RangeValue endScaledToCommon = valueInCommon.end - offsetFromCommon;

        // tick_common = tick * multiplier
        Int multiplierNumerator = regularDomain.resolution.getNumerator() * commonDomain.resolution.getDenominator();
        Int multiplierDenominator = regularDomain.resolution.getDenominator() * commonDomain.resolution.getNumerator();
        RangeValue startValue = static_cast<RangeValue>(startScaledToCommon * multiplierDenominator / static_cast<double>(multiplierNumerator));
        RangeValue endValue = valueInCommon.end == -1
            ? static_cast<RangeValue>(-1)
            : static_cast<RangeValue>(endScaledToCommon * multiplierDenominator / static_cast<double>(multiplierNumerator));
	    
		return std::make_unique<DomainValueImpl<RangeValue>>(regularDomain, RangeType64{startValue, endValue});
    }

    RangeType64 getValue() const
	{
	    return value;
	}

    double getTime() override
	{
        using SysPeriod = std::chrono::system_clock::period;
	    return domain.epoch.time_since_epoch().count() * SysPeriod::num / static_cast<double>(SysPeriod::den)
            + value.start * domain.resolution.getNumerator() / static_cast<double>(domain.resolution.getDenominator());
	}
	
	int compare(const DomainValue& other) const override
	{
	    const auto* otherImpl = dynamic_cast<const DomainValueImpl<RangeType64>(&other);
	    if (otherImpl == nullptr){
	        DAQ_THROW_EXCEPTION(InvalidParameterException, "Both DomainValue objects must be of the same type!");
	    }
	    if (otherImpl->domain != this->domain) // TODO: Implement equality operator
	        DAQ_THROW_EXCEPTION(InvalidParameterException, "Have to compare DomainValue objects in the same domain!");
	    
	    if (this->value.start > otherImpl->value.start)
	        return 1;
	    else if (this->value.start == otherImpl->value.start)
	        return 0;
	    else // this->value.start < otherImpl->value.start
	        return -1;
	}

private:
    RangeType64 value;
};

template<>
class DomainValueImpl<ComplexFloat32> final : public DomainValue
{
public:
    explicit DomainValueImpl(const DomainInfo& info, ComplexFloat32 value) : DomainValue(info) {
	}

    std::unique_ptr<DomainValue> toCommonDomain(const DomainInfo& commonDomain) override
    {
        DAQ_THROW_EXCEPTION(NotSupportedException);
    }

    std::unique_ptr<DomainValue> fromCommonDomain(const DomainInfo& regularDomain) override
    {
        DAQ_THROW_EXCEPTION(NotSupportedException);
    }

    ComplexFloat32 getValue() const
	{
	    DAQ_THROW_EXCEPTION(NotSupportedException);
	}

    double getTime() override
	{
        DAQ_THROW_EXCEPTION(NotSupportedException);
	}
	
	int compare(const DomainValue& other) const override
	{
	    DAQ_THROW_EXCEPTION(NotSupportedException);
	}
};

template<>
class DomainValueImpl<ComplexFloat64> final : public DomainValue
{
public:
    explicit DomainValueImpl(const DomainInfo& info, ComplexFloat64 value) : DomainValue(info) {
	}

    std::unique_ptr<DomainValue> toCommonDomain(const DomainInfo& commonDomain) override
    {
        DAQ_THROW_EXCEPTION(NotSupportedException);
    }

    std::unique_ptr<DomainValue> fromCommonDomain(const DomainInfo& regularDomain) override
    {
        DAQ_THROW_EXCEPTION(NotSupportedException);
    }

    ComplexFloat64 getValue() const
	{
	    DAQ_THROW_EXCEPTION(NotSupportedException);
	}

    double getTime() override
	{
        DAQ_THROW_EXCEPTION(NotSupportedException);
	}
	
	int compare(const DomainValue& other) const override
	{
	    DAQ_THROW_EXCEPTION(NotSupportedException);
	}
};



END_NAMESPACE_OPENDAQ
