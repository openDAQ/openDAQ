#!/usr/bin/env python3
"""Loops gtest suites until a deadline and records every failing test.

Usage: stress.py --deadline <unix time> <test binary>=<gtest filter> ...

The suites run in turn, one process at a time, as they listen on the same network ports. Every run has --gtest_shuffle
and a new --gtest_random_seed, which replays its test order. A run is not started when it would not finish before the
deadline; a run still going at the deadline is killed and not counted. lt_server_stress.json in the GTEST_OUTPUT
directory is rewritten after every run, and the console log of each failed run is kept beside it. Exits 1 when any test
failed.
"""

import argparse
import json
import os
import platform
import random
import re
import subprocess
import time
from pathlib import Path

HANG_SECONDS = 20 * 60  # a run that has not finished after this long is a hang
MAX_KEPT_LOGS = 100  # failed run logs kept per binary
MAX_FAILED_RUNS = 50  # failed runs listed per test
MESSAGE_LINES = 60  # output lines kept as a failure's message
LINE = re.compile(r"^\[ +(RUN|OK|FAILED|SKIPPED) +\] ([^\s,]+)")


def results_dir():
    output = os.environ.get("GTEST_OUTPUT", "")
    path = Path(output[len("xml:"):]) if output.startswith("xml:") else Path.cwd() / "lt-server-stress"
    path.mkdir(parents=True, exist_ok=True)
    return path


def parse(output):
    """Returns the passed tests, the failed tests with their output, and the test still running when the process ended."""
    passed, failed, current, lines = [], {}, None, []
    for line in output.splitlines():
        match = LINE.match(line)
        if match and match.group(1) == "RUN":
            current, lines = match.group(2), []
        elif match and match.group(2) == current:
            if match.group(1) == "OK":
                passed.append(current)
            elif match.group(1) == "FAILED":
                failed[current] = lines[-MESSAGE_LINES:]
            current = None
        elif current:
            lines.append(line)
    return passed, failed, (current, lines[-MESSAGE_LINES:]) if current else None


class Suite:
    def __init__(self, path, gtest_filter):
        self.path = path
        self.filter = gtest_filter
        self.binary = Path(path).stem
        self.longest = 0.0
        self.run = 0


class Stress:
    def __init__(self, deadline, out):
        self.deadline = deadline
        self.out = out
        self.summary = {
            "host": platform.platform(),
            "machine": platform.machine(),
            "deadline": deadline,
            "binaries": {},
            "tests": {},
        }

    def save(self):
        tmp = self.out / "lt_server_stress.json.tmp"
        tmp.write_text(json.dumps(self.summary, indent=1))
        os.replace(str(tmp), str(self.out / "lt_server_stress.json"))

    def record(self, binary, run, seed, elapsed, passed, failures, output):
        stats = self.summary["binaries"][binary]
        stats["runs"] += 1
        stats["seconds"] += round(elapsed)
        for name in passed:
            self.test(binary, name)["runs"] += 1
        for name, (kind, lines) in failures.items():
            test = self.test(binary, name)
            test["runs"] += 1
            test["failures"] += 1
            if len(test["failed-runs"]) < MAX_FAILED_RUNS:
                test["failed-runs"].append({"run": run, "seed": seed, "kind": kind})
            test.setdefault("message", "\n".join(lines))
        if failures:
            stats["failed-runs"] += 1
            if stats["failed-runs"] <= MAX_KEPT_LOGS:
                (self.out / "{}_run{}_seed{}.log".format(binary, run, seed)).write_text(output, encoding="utf-8")
        self.save()

    def test(self, binary, name):
        return self.summary["tests"].setdefault(name, {"binary": binary, "runs": 0, "failures": 0, "failed-runs": []})

    def add(self, suite):
        self.summary["binaries"][suite.binary] = {"filter": suite.filter, "runs": 0, "failed-runs": 0, "seconds": 0}

    def run_once(self, suite, env):
        """Runs the suite once; returns whether it may run again."""
        remaining = self.deadline - time.time()
        if remaining < max(60.0, 1.5 * suite.longest):
            return False
        suite.run += 1
        seed = random.randint(1, 99999)
        command = [suite.path, "--gtest_filter=" + suite.filter, "--gtest_shuffle",
                   "--gtest_random_seed={}".format(seed), "--gtest_color=no"]
        limit = min(remaining, HANG_SECONDS)
        start = time.time()
        process = subprocess.Popen(command, cwd=os.path.dirname(suite.path), env=env, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT)
        try:
            output, hung = process.communicate(timeout=limit)[0], False
        except subprocess.TimeoutExpired:
            process.kill()
            output, hung = process.communicate()[0], True
        elapsed = time.time() - start
        if hung and limit < HANG_SECONDS:
            print("{} run {} reached the deadline and is not counted".format(suite.binary, suite.run), flush=True)
            return False
        suite.longest = max(suite.longest, elapsed)
        output = output.decode("utf-8", "replace")
        passed, failed, unfinished = parse(output)
        # gtest exits with 0 only when every test passed: a log line of another thread then hid the end of a test
        if unfinished and not hung and process.returncode == 0:
            passed.append(unfinished[0])
            unfinished = None
        failures = {name: ("failed", lines) for name, lines in failed.items()}
        if unfinished:
            kind = "hang" if hung else "crash (exit code {})".format(process.returncode)
            failures[unfinished[0]] = (kind, unfinished[1])
        elif process.returncode != 0 and not failed:
            tail = output.splitlines()[-MESSAGE_LINES:]
            failures["<{}>".format(suite.binary)] = ("exit code {}".format(process.returncode), tail)
        self.record(suite.binary, suite.run, seed, elapsed, passed, failures, output)
        print("{} run {} seed {}: {} passed, {} failed{}, {:.0f} s".format(
            suite.binary, suite.run, seed, len(passed), len(failures), " " + ", ".join(failures) if failures else "",
            elapsed), flush=True)
        if not passed and not failed and not unfinished:
            print("{} ran no test; stopping its loop".format(suite.binary), flush=True)
            return False
        return True

    def report(self):
        lines = ["### LT server stress: {}".format(self.summary["host"]), "",
                 "| Binary | Filter | Runs | Failed runs | Hours |", "|---|---|---|---|---|"]
        for binary, stats in self.summary["binaries"].items():
            lines.append("| {} | `{}` | {} | {} | {:.1f} |".format(
                binary, stats["filter"], stats["runs"], stats["failed-runs"], stats["seconds"] / 3600))
        failing = {name: test for name, test in self.summary["tests"].items() if test["failures"]}
        lines += ["", "| Failing test | Failures / runs | Kinds | Seeds |", "|---|---|---|---|"]
        for name, test in sorted(failing.items()):
            kinds = sorted({run["kind"] for run in test["failed-runs"]})
            seeds = ", ".join(str(run["seed"]) for run in test["failed-runs"][:5])
            lines.append("| `{}` | {} / {} | {} | {} |".format(name, test["failures"], test["runs"], ", ".join(kinds), seeds))
        if not failing:
            lines.append("| none | | | |")
        return "\n".join(lines) + "\n", bool(failing)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--deadline", type=float, required=True, help="unix time after which no run is started")
    parser.add_argument("suites", nargs="+", help="<test binary>=<gtest filter>")
    args = parser.parse_args()

    stress = Stress(args.deadline, results_dir())
    print("Running until {} UTC".format(time.strftime("%Y-%m-%d %H:%M", time.gmtime(args.deadline))), flush=True)
    env = dict(os.environ)
    env.pop("GTEST_OUTPUT", None)
    active = [Suite(*suite.split("=", 1)) for suite in args.suites]
    for suite in active:
        stress.add(suite)
    while active:
        active = [suite for suite in active if stress.run_once(suite, env)]

    text, failed = stress.report()
    print(text, flush=True)
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a") as file:
            file.write(text)
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
