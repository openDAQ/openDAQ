##
# Example that creates mock signals with value and domain descriptors. That are later validated
# individually for any issues with reading them as well as checking their cross compatibility.
#
# Applications should implement similar checks to determine whether or not the connected signal
# is appropriate for the connected input port and whether the application is able to read it
# correctly. The checks have two phases: in the first phase, a signal itself may be invalid
# or the provided set of signals may be invalid due to incompatibility between different
# signals.
#
# A similar system is used in Multireader to accept/reject reading from a set of signals.
#
##

import math
from dataclasses import dataclass, field
from datetime import datetime, timezone
from enum import Enum, auto

import opendaq as daq

# In a real openDAQ example, ctx should come from the instance/context setup.
# For example:
# instance = daq.Instance()
# ctx = instance.context
ctx = daq.NullContext()


class SignalIssue(Enum):
    MissingValueDescriptor = auto()
    MissingDomainSignal = auto()
    MissingDomainDescriptor = auto()

    ValueTypesNotConvertible = auto()
    DomainTypesNotConvertible = auto()

    OriginParsingFailed = auto()
    UnsupportedDomainRule = auto()
    DomainUnitInvalid = auto()


@dataclass
class SignalDomainInfo:
    origin: datetime | None = None
    tick_resolution_num: int | None = None
    tick_resolution_den: int | None = None
    sample_rate: int | None = None
    delta: int | None = None


@dataclass
class SignalCheckResult:
    signal_name: str
    issues: set[SignalIssue] = field(default_factory=set)
    domain_info: SignalDomainInfo = field(default_factory=SignalDomainInfo)

    @property
    def valid(self) -> bool:
        return len(self.issues) == 0


class CompatibilityIssue(Enum):
    SignalInvalid = auto()
    IncompatibleTickResolution = auto()
    DifferentOrigin = auto()
    NonIntegerRateRatio = auto()


@dataclass
class CompatibilityResult:
    issues: set[CompatibilityIssue] = field(default_factory=set)
    invalid_signals: set[str] = field(default_factory=set)
    domain_info: SignalDomainInfo | None = None
    sample_rate: int | None = None

    @property
    def compatible(self) -> bool:
        return len(self.issues) == 0


def try_parse_epoch(origin: str) -> datetime | None:
    """
    Parses a practical ISO-8601 timestamp.

    Supports strings such as:
        2022-09-27T00:02:03+00:00
        2022-09-27T00:02:04.123+00:00
        2022-09-27T00:02:03Z
    """
    if not origin:
        return None

    fixed = origin
    if fixed.endswith("Z"):
        fixed = fixed[:-1] + "+00:00"

    try:
        return datetime.fromisoformat(fixed)
    except ValueError:
        return None


def sample_type_convertible(input_type, output_type, is_domain: bool) -> bool:
    """
    Simplified example of the convertibility check. In practical cases, structs, binary data etc. may be allowed.
    """
    numeric_types = {
        daq.SampleType.Int8,
        daq.SampleType.Int16,
        daq.SampleType.Int32,
        daq.SampleType.Int64,
        daq.SampleType.UInt8,
        daq.SampleType.UInt16,
        daq.SampleType.UInt32,
        daq.SampleType.UInt64,
        daq.SampleType.Float32,
        daq.SampleType.Float64,
    }

    if is_domain:
        return input_type in {
            daq.SampleType.Int64,
            daq.SampleType.UInt64,
        } and output_type in {
            daq.SampleType.Int64,
            daq.SampleType.UInt64,
        }

    return input_type in numeric_types and output_type in numeric_types


def check_value_descriptor(signal, result: SignalCheckResult, value_out=daq.SampleType.Float64):
    descriptor = signal.descriptor

    if descriptor is None:
        result.issues.add(SignalIssue.MissingValueDescriptor)
        return

    value_in = descriptor.sample_type

    if value_out == daq.SampleType.Undefined:
        value_out = value_in

    if not sample_type_convertible(value_in, value_out, is_domain=False):
        result.issues.add(SignalIssue.ValueTypesNotConvertible)


def check_domain_descriptor(signal, result: SignalCheckResult, domain_out=daq.SampleType.Int64):
    domain_signal = signal.domain_signal

    if domain_signal is None:
        result.issues.add(SignalIssue.MissingDomainSignal)
        return

    descriptor = domain_signal.descriptor

    if descriptor is None:
        result.issues.add(SignalIssue.MissingDomainDescriptor)
        return

    domain_in = descriptor.sample_type

    if not sample_type_convertible(domain_in, domain_out, is_domain=True):
        result.issues.add(SignalIssue.DomainTypesNotConvertible)

    # Resolution and origin
    resolution = descriptor.tick_resolution
    result.domain_info.tick_resolution_num = resolution.numerator
    result.domain_info.tick_resolution_den = resolution.denominator

    origin = descriptor.origin
    parsed_origin = try_parse_epoch(origin)

    if parsed_origin is None:
        result.issues.add(SignalIssue.OriginParsingFailed)
    else:
        result.domain_info.origin = parsed_origin

    # Sample rate and delta
    rule = descriptor.rule
    rule_is_linear = rule is not None and rule.type == daq.DataRuleType.Linear

    unsupported_rule = False

    if not rule_is_linear:
        unsupported_rule = True
    else:
        delta = rule.parameters["delta"]
        delta_float = float(delta)
        delta_int = int(delta_float)

        delta_is_integer = delta_float == float(delta_int)

        sample_rate = (
            float(resolution.denominator)
            / (float(resolution.numerator * delta_float))
        )

        sample_rate_int = int(sample_rate)
        sample_rate_is_integer = sample_rate == float(sample_rate_int)

        unsupported_rule = not delta_is_integer or not sample_rate_is_integer

        if not unsupported_rule:
            result.domain_info.delta = delta_int
            result.domain_info.sample_rate = sample_rate_int

    if unsupported_rule:
        result.issues.add(SignalIssue.UnsupportedDomainRule)

    # Unit and quantity
    unit = descriptor.unit

    domain_is_time_in_seconds = (
        unit is not None
        and unit.quantity == "time"
        and unit.symbol == "s"
    )

    if not domain_is_time_in_seconds:
        result.issues.add(SignalIssue.DomainUnitInvalid)


def check_signal(signal) -> SignalCheckResult:
    result = SignalCheckResult(signal_name=signal.name)

    check_value_descriptor(signal, result)
    check_domain_descriptor(signal, result)

    return result


def check_cross_compatibility(results: list[SignalCheckResult]) -> CompatibilityResult:
    compatibility = CompatibilityResult()

    for result in results:
        if not result.valid:
            compatibility.issues.add(CompatibilityIssue.SignalInvalid)
            compatibility.invalid_signals.add(result.signal_name)

    reference = results[0].domain_info

    for result in results[1:]:
        info = result.domain_info

        # If there is no common tick resolution calculation, difference in ticks means incompatibility
        if (
            info.tick_resolution_num != reference.tick_resolution_num
            or info.tick_resolution_den != reference.tick_resolution_den
        ):
            compatibility.issues.add(CompatibilityIssue.DifferentTickResolution)

        # If there is no origin related offset calculation to transform ticks into a common domain,
        # difference in origin means incompatibility
        if info.origin != reference.origin:
            compatibility.issues.add(CompatibilityIssue.DifferentOrigin)

        if reference.sample_rate is None or info.sample_rate is None:
            compatibility.issues.add(CompatibilityIssue.SignalInvalid)
            continue

        high = max(reference.sample_rate, info.sample_rate)
        low = min(reference.sample_rate, info.sample_rate)

        # If sample rates are not either multiples or divisors of the reference sample rate,
        # this means incompatibility
        if high % low != 0:
            compatibility.issues.add(CompatibilityIssue.NonIntegerRateRatio)
        
        # Note that for each application, the content of the rules may be different. Reasons for
        # incompatibility may be circumvented by implementing various features such as common
        # domain calculation, common sample rate with per-signal divisors and resampling.

    return compatibility


def print_signal_result(result: SignalCheckResult):
    print(f"\nSignal: {result.signal_name}")
    print(f"  valid: {result.valid}")

    if result.issues:
        print("  issues:")
        for issue in sorted(result.issues, key=lambda x: x.name):
            print(f"    - {issue.name}")

    info = result.domain_info
    print("  domain:")
    print(f"    origin: {info.origin}")
    print(f"    tick resolution: {info.tick_resolution_num}/{info.tick_resolution_den}")
    print(f"    sample rate: {info.sample_rate}")
    print(f"    delta: {info.delta}")


def print_compatibility_result(result: CompatibilityResult):
    print("\nCross-signal compatibility")
    print(f"  compatible: {result.compatible}")

    if result.issues:
        print("  issues:")
        for issue in sorted(result.issues, key=lambda x: x.name):
            print(f"    - {issue.name}")


def create_signal(id, epoch):
    """
    Creates a signal with nested domain signal and the given id and origin.
    """
    signal = daq.Signal(ctx, None, id + "_values", None)
    domain = daq.Signal(ctx, None, id + "_domain", None)

    vals_desc_bldr = daq.DataDescriptorBuilder()
    vals_desc_bldr.sample_type = daq.SampleType.Float64

    domain_desc_bldr = daq.DataDescriptorBuilder()
    domain_desc_bldr.sample_type = daq.SampleType.Int64
    domain_desc_bldr.tick_resolution = daq.Ratio(1, 1000)
    domain_desc_bldr.rule = daq.LinearDataRule(1, 0)
    domain_desc_bldr.unit = daq.Unit(-1, "s", "second", "time")
    domain_desc_bldr.origin = epoch

    domain.descriptor = domain_desc_bldr.build()
    signal.descriptor = vals_desc_bldr.build()
    signal.domain_signal = domain

    return daq.ISignal.cast_from(signal)


if __name__ == "__main__":
    sig0 = create_signal("sig0", "2022-09-27T00:02:03+00:00")
    sig1 = create_signal("sig1", "2022-09-27T00:02:04+00:00")
    sig2 = create_signal("sig2", "2022-09-27T00:02:04.123+00:00")

    signals = [sig0, sig1, sig2]

    print("Stage 1: Check individual signals")

    results = []
    for signal in signals:
        result = check_signal(signal)
        results.append(result)
        print_signal_result(result)

    print("\nStage 2: Check cross-signal compatibility")

    compatibility = check_cross_compatibility(results)
    print_compatibility_result(compatibility)