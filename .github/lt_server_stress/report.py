#!/usr/bin/env python3
"""Merges the lt_server_stress.json files of a stress workflow run into one Markdown report.

Usage: report.py <artifacts dir> [--jobs <JSON list of matrix jobs>]

The artifacts dir holds one <pass>-test-results-<job> folder per matrix job, as actions/download-artifact lays them out
(gh run download does the same). A job in --jobs without results failed before its tests ran.
"""

import argparse
import json
import re
from pathlib import Path

FOLDER = re.compile(r"^(pass\d+)-test-results-(.+)$")


def load(root):
    """Returns {job: {pass: summary}} for every result folder under root."""
    results = {}
    for path in sorted(Path(root).glob("*/lt_server_stress.json")):
        match = FOLDER.match(path.parent.name)
        if match:
            results.setdefault(match.group(2), {})[match.group(1)] = json.loads(path.read_text())
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("root")
    parser.add_argument("--jobs", default="[]", help="JSON list of the matrix jobs the run included")
    args = parser.parse_args()

    results = load(args.root)
    jobs = sorted(set(json.loads(args.jobs)) | set(results))
    passes = sorted({p for by_pass in results.values() for p in by_pass})

    out = ["## Platforms", "", "| Job | " + " | ".join("{} runs (failed)".format(p) for p in passes) + " |",
           "|---|" + "---|" * len(passes)]
    for job in jobs:
        cells = []
        for p in passes:
            summary = results.get(job, {}).get(p)
            if not summary:
                cells.append("no results")
                continue
            cells.append(", ".join("{} {} ({})".format(binary, stats["runs"], stats["failed-runs"])
                                   for binary, stats in summary["binaries"].items()))
        out.append("| {} | {} |".format(job, " | ".join(cells)))

    # test -> job -> merged counts over the passes
    failing = {}
    for job, by_pass in results.items():
        for summary in by_pass.values():
            for name, test in summary["tests"].items():
                if not test["failures"]:
                    continue
                entry = failing.setdefault(name, {}).setdefault(
                    job, {"binary": test["binary"], "failures": 0, "runs": 0, "kinds": set(), "seeds": [], "message": ""})
                entry["failures"] += test["failures"]
                entry["runs"] += test["runs"]
                entry["kinds"].update(run["kind"] for run in test["failed-runs"])
                entry["seeds"] += [run["seed"] for run in test["failed-runs"]]
                entry["message"] = entry["message"] or test.get("message", "")
    # a test that passed on a job in another pass still counts its runs there
    for name, by_job in failing.items():
        for job, entry in by_job.items():
            for summary in results[job].values():
                test = summary["tests"].get(name)
                if test and not test["failures"]:
                    entry["runs"] += test["runs"]

    out += ["", "## Failing tests", "", "| Test | Job | Failures / runs | Kinds | Seeds (--gtest_random_seed) |", "|---|---|---|---|---|"]
    for name in sorted(failing):
        for job in sorted(failing[name]):
            entry = failing[name][job]
            out.append("| `{}` | {} | {} / {} | {} | {} |".format(
                name, job, entry["failures"], entry["runs"], ", ".join(sorted(entry["kinds"])),
                ", ".join(str(seed) for seed in entry["seeds"][:5])))
    if not failing:
        out.append("| none | | | | |")

    out += ["", "## Failure output", ""]
    for name in sorted(failing):
        for job in sorted(failing[name]):
            entry = failing[name][job]
            out += ["<details><summary>{} on {}</summary>".format(name, job), "", "```",
                    entry["message"].strip() or "(no output)", "```", "", "</details>", ""]
    print("\n".join(out))


if __name__ == "__main__":
    main()
