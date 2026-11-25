#!/usr/bin/env python3
"""Waits for the start time of a stress pass and says how long the pass may run.

Usage: gate.py --pass <1|2> [--start-at HH:MM|now] [--end-at HH:MM] [--minutes N]

Times are Europe/Ljubljana. The first pass waits for the start time; between then and 06:00 the night is under way, so a
run pushed then starts at once. A pass runs for --minutes, or less when the end time is nearer, and not at all when less
than an hour is left. Writes `minutes` and `run` to GITHUB_OUTPUT.

A file named `trial` beside this script, holding a number of minutes, makes a trial run: one pass of that length, at once.
"""

import argparse
import datetime
import os
import time
from pathlib import Path
from zoneinfo import ZoneInfo

ZONE = ZoneInfo("Europe/Ljubljana")
SETUP_MINUTES = 15  # a job needs about that long before configure, where the minutes of a pass start to count


def at(now, text):
    hour, minute = map(int, text.split(":"))
    return now.replace(hour=hour, minute=minute, second=0, microsecond=0)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--pass", dest="number", type=int, required=True)
    parser.add_argument("--start-at", default="18:00")
    parser.add_argument("--end-at", default="06:30")
    parser.add_argument("--minutes", type=int, default=300)
    args = parser.parse_args()

    trial = Path(__file__).with_name("trial")
    if trial.exists():
        minutes, run = int(trial.read_text().strip() or 100), args.number == 1
        print(f"Trial run: pass {args.number} {'runs' if run else 'does not run'} for {minutes} minutes")
    else:
        now = datetime.datetime.now(ZONE)
        if args.number == 1 and args.start_at != "now":
            start = at(now, args.start_at)
            wait = 0 if now >= start or now.hour < 6 else (start - now).total_seconds()
            if wait > 5.75 * 3600:
                raise SystemExit(f"::error::{start.isoformat()} is {wait / 3600:.1f} h away; a job may wait at most six hours")
            print(f"Waiting {wait / 3600:.2f} h until {start.isoformat()}", flush=True)
            time.sleep(wait)
            now = datetime.datetime.now(ZONE)

        end = at(now, args.end_at)
        if end <= now:
            end += datetime.timedelta(days=1)
        minutes = min(args.minutes, int((end - now).total_seconds() // 60) - SETUP_MINUTES)
        run = minutes >= 60
        print(f"Pass {args.number}: {minutes} minutes until {end.isoformat()}, {'runs' if run else 'does not run'}")

    with open(os.environ["GITHUB_OUTPUT"], "a") as output:
        output.write(f"minutes={minutes}\nrun={'true' if run else 'false'}\n")


if __name__ == "__main__":
    main()
