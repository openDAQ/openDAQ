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
#include <opendaq/typed_reader.h>
#include <opendaq/reader_domain_info.h>

BEGIN_NAMESPACE_OPENDAQ

class Comparable
{
public:
    explicit Comparable(const ReaderDomainInfo& domainInfo)
        : info(domainInfo)
    {
    }

    virtual ~Comparable() = default;

    Comparable(const Comparable& other) = delete;
    Comparable(Comparable&& other) noexcept = delete;
    Comparable& operator=(const Comparable& other) = delete;
    Comparable& operator=(Comparable&& other) noexcept = delete;

    friend bool operator<(const Comparable& lhs, const Comparable& rhs)
    {
        return lhs.compare(rhs) < 0;
    }

    friend bool operator<=(const Comparable& lhs, const Comparable& rhs)
    {
        return !(rhs < lhs);
    }

    friend bool operator>(const Comparable& lhs, const Comparable& rhs)
    {
        return rhs < lhs;
    }

    friend bool operator>=(const Comparable& lhs, const Comparable& rhs)
    {
        return !(lhs < rhs);
    }

    friend bool operator==(const Comparable& lhs, const Comparable& rhs)
    {
        return lhs.compare(rhs) == 0;
    }

    friend bool operator!=(const Comparable& lhs, const Comparable& rhs)
    {
        return !(lhs == rhs);
    }

    virtual void getValue(void* start) const noexcept = 0;

    virtual void roundUpOnUnitOfDomain() = 0;

    virtual void roundUpOnDomainInterval(const RatioPtr interval) = 0;

#if !defined(NDEBUG)
    virtual std::string asTime() const = 0;
#endif

    virtual void print(std::ostream& os) const = 0;

    friend std::ostream& operator<<(std::ostream& os, const Comparable& obj)
    {
        obj.print(os);
        return os;
    }

protected:
    const ReaderDomainInfo& info;
private:
    [[nodiscard]] virtual int compare(const Comparable& other) const = 0;
};

template <typename ReadType>
class ComparableValue : public Comparable
{
public:
    explicit ComparableValue(ReadType startingValue, const ReaderDomainInfo& domainInfo, bool log = true)
        : Comparable(domainInfo)
        , value(startingValue * domainInfo.multiplier.getNumerator() / static_cast<double>(domainInfo.multiplier.getDenominator()))
    {

#if !defined(NDEBUG)
        using namespace reader;

        if (log)
        {
            auto signalTime = timePointString(toSysTime<ReadType, std::chrono::microseconds>(
                startingValue,
                domainInfo.epoch,
                domainInfo.resolution
            ));
            auto readTime = timePointString(toSysTime<ReadType, std::chrono::microseconds>(
                value + info.offset,
                info.readEpoch,
                info.readResolution
            ));

            DAQLOGF_T(domainInfo.loggerComponent, "First domain value: {}", startingValue)
            DAQLOGF_T(domainInfo.loggerComponent, "adjusted: {}", this->value)
            DAQLOGF_T(domainInfo.loggerComponent, "adj. + offset: {}", (this->value + info.offset))
            DAQLOGF_T(domainInfo.loggerComponent, "{} | {}>", signalTime, readTime)

            // In case if the signal resolution less than the difference between
            // domainInfo.epoch and info.readEpoch, signalTime and readTime,
            // expressed in system_clock resolution ticks, rounded up to
            // microsecond, will differ. It is not like a bug, more like a
            // thing, that needs to be polished. Maybe it is worth to
            // round epochs to the max resolution.
            // 
            //assert(signalTime == readTime);
        }
#endif

        value += info.offset;
    }

    [[nodiscard]]
    const ReadType& getValue() const noexcept
    {
        return value;
    }

    void getValue(void* start) const noexcept override
    {
        if (start != nullptr)
        {
            *static_cast<ReadType*>(start) = value;
        }
    }

    void roundUpOnUnitOfDomain() override
    {
        // calc maxResolution num/den
        auto num = info.resolution.getNumerator() * info.multiplier.getDenominator();
        auto den = info.resolution.getDenominator() * info.multiplier.getNumerator();

        const Int gcd = std::gcd(num, den);
        num /= gcd;
        den /= gcd;

        if (den % num != 0)
            DAQ_THROW_EXCEPTION(NotSupportedException, "Resolution must be aligned on full unit of domain");

        value = (((value * num + den - 1) / den) * den) / num;
    }

    void roundUpOnDomainInterval(const RatioPtr interval) override
    {
        auto num = info.resolution.getNumerator() * info.multiplier.getDenominator() * interval.getDenominator();
        auto den = info.resolution.getDenominator() * info.multiplier.getNumerator() * interval.getNumerator();

        const Int gcd = std::gcd(num, den);
        num /= gcd;
        den /= gcd;

        if (den % num != 0)
            DAQ_THROW_EXCEPTION(NotSupportedException, "Resolution must be aligned on full unit of domain");

        value = (((value * num + den - 1) / den) * den) / num;
    }

    void print(std::ostream& os) const override
    {
        using namespace reader;

        os.setf(std::ios::left);
        os.width(10);
        os << value << " | "

#if !defined(NDEBUG)
        << toSysTime(value, info.readEpoch, info.readResolution)
#endif
        ;
    }

#if !defined(NDEBUG)
    virtual std::string asTime() const override
    {
        using namespace reader;

        std::stringstream ss;
        ss << toSysTime(value, info.readEpoch, info.readResolution);

        return ss.str();
    }
#endif

private:
    [[nodiscard]]
    int compare(const Comparable& other) const override
    {
        const auto* sortable = dynamic_cast<const ComparableValue<ReadType>*>(&other);
        if (sortable == nullptr)
        {
            // ReSharper disable once StringLiteralTypo
            DAQ_THROW_EXCEPTION(InvalidParameterException, "All Comparables must be of the same type!");
        }

        if (value > sortable->value)
        {
            return 1;
        }

        return sortable->value == value
            ? 0
            : -1;
    }

    ReadType value;
};

template <>
class ComparableValue<ComplexFloat32> final : public Comparable
{
public:
    explicit ComparableValue(ComplexFloat32 startingValue, const ReaderDomainInfo& domainInfo, bool log = true)
        : Comparable(domainInfo)
    {
    }

    void getValue(void* /*start*/) const noexcept override
    {
    }

    [[nodiscard]]
    const ComplexFloat32& getValue() const
    {
        DAQ_THROW_EXCEPTION(NotSupportedException);
    }

    void roundUpOnUnitOfDomain() override
    {
        DAQ_THROW_EXCEPTION(NotSupportedException);
    }

    void roundUpOnDomainInterval(const RatioPtr interval) override
    {
        DAQ_THROW_EXCEPTION(NotSupportedException);
    }

    void print(std::ostream& os) const override
    {
    }

#if !defined(NDEBUG)
    virtual std::string asTime() const override
    {
        return "";
    }
#endif

private:
    [[nodiscard]]
    int compare(const Comparable& other) const override
    {
        DAQ_THROW_EXCEPTION(NotSupportedException);
    }
};

template <>
class ComparableValue<ComplexFloat64> final : public Comparable
{
public:
    explicit ComparableValue(ComplexFloat64 startingValue, const ReaderDomainInfo& domainInfo, bool log = true)
        : Comparable(domainInfo)
    {
    }

    void getValue(void* start) const noexcept override
    {
    }

    [[nodiscard]]
    const ComplexFloat64& getValue() const
    {
        DAQ_THROW_EXCEPTION(NotSupportedException);
    }

    void roundUpOnUnitOfDomain() override
    {
        DAQ_THROW_EXCEPTION(NotSupportedException);
    }

    void roundUpOnDomainInterval(const RatioPtr interval) override
    {
        DAQ_THROW_EXCEPTION(NotSupportedException);
    }

    void print(std::ostream& os) const override
    {
    }

#if !defined(NDEBUG)
    virtual std::string asTime() const override
    {
        return "";
    }
#endif

private:
    [[nodiscard]]
    int compare(const Comparable& other) const override
    {
        DAQ_THROW_EXCEPTION(NotSupportedException);
    }
};

template <>
class ComparableValue<RangeType64> final : public Comparable
{
public:
    using RangeValue = RangeType64::Type;

    explicit ComparableValue(RangeType64 value, const ReaderDomainInfo& domainInfo, bool log = true)
        : Comparable(domainInfo)
        , value(RangeType64(
            (value.start * info.multiplier) + info.offset,
            value.end == -1
                ? static_cast<RangeValue>(-1)
                : static_cast<RangeValue>((value.end * info.multiplier) + info.offset)
        ))
    {
    }

    void getValue(void* start) const noexcept override
    {
        if (start != nullptr)
        {
            *static_cast<RangeType64*>(start) = value;
        }
    }

    [[nodiscard]]
    const RangeType64& getValue() const noexcept
    {
        return value;
    }

    void roundUpOnUnitOfDomain() override
    {
        DAQ_THROW_EXCEPTION(NotSupportedException);
    }

    void roundUpOnDomainInterval(const RatioPtr interval) override
    {
        DAQ_THROW_EXCEPTION(NotSupportedException);
    }


    virtual void print(std::ostream& os) const override
    {
        os << "[" << value.start << ", " << value.end << "]";
    }

#if !defined(NDEBUG)
    virtual std::string asTime() const override
    {
        return "";
    }
#endif

private:
    [[nodiscard]]
    int compare(const Comparable& other) const override
    {
        const auto* sortable = dynamic_cast<const ComparableValue<RangeType64>*>(&other);
        if (sortable == nullptr)
        {
            // ReSharper disable once StringLiteralTypo
            DAQ_THROW_EXCEPTION(InvalidParameterException, "All Comparables must be of the same type!");
        }

        if (value.start > sortable->value.start)
        {
            return 1;
        }

        return value.start == sortable->value.start
            ? 0
            : -1;
    }

    RangeType64 value;
};

END_NAMESPACE_OPENDAQ
