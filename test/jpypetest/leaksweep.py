#!/usr/bin/env python
# -*- coding: utf-8 -*-
# *****************************************************************************
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   See NOTICE file for details.
#
# *****************************************************************************
"""Config-driven, time-budgeted leak-detection sweep.

Generalizes test_leak.py's proven fixed-batch LeakChecker/memTest to a
curated, time-budgeted run across multiple tests (see
plan/LeakCheckHarness.md for the full design rationale). Each target listed
in a config file (default: leak_targets.txt, next to this script) gets run
through leakharness.memTestBudget() for its own configured wall-clock
budget, in its own subrun-isolated, fresh small-heap JVM subprocess -- the
same isolation test_leak.py already uses, just driven from outside pytest
so entries can run for minutes/hours instead of a fast-suite-sized fixed
batch count.

This is deliberately NOT collected by pytest (it is not a test_*.py file
importable as a normal test module) -- it is an opt-in sweep, run directly:

    python leaksweep.py [config_file] [--jobs N]

Entries run concurrently across a process pool (default: one job per 2
cores), since each entry already isolates itself into its own subprocess.
"""
import argparse
import concurrent.futures
import importlib.util
import os
import sys
import time
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_CONFIG = os.path.join(HERE, 'leak_targets.txt')
DEFAULT_TIMEOUT_MARGIN = 30.0  # seconds of IPC slack on top of a target's own budget


def parse_config(config_path):
    """Parses `module.py:ClassName.testMethod:budget_seconds` lines.

    Returns a list of (module_file, clsname, methodname, budget_seconds).
    """
    entries = []
    with open(config_path) as f:
        for lineno, raw_line in enumerate(f, 1):
            line = raw_line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split(':')
            if len(parts) != 3:
                raise ValueError(
                    "%s:%d: malformed entry %r, expected "
                    "module.py:ClassName.testMethod:budget_seconds" %
                    (config_path, lineno, line))
            module_file, qualname, budget = parts
            qualparts = qualname.split('.')
            if len(qualparts) != 2:
                raise ValueError(
                    "%s:%d: malformed entry %r, expected "
                    "module.py:ClassName.testMethod:budget_seconds" %
                    (config_path, lineno, line))
            clsname, methodname = qualparts
            entries.append((module_file, clsname, methodname, float(budget)))
    return entries


def _import_module(module_file):
    module_name = os.path.splitext(os.path.basename(module_file))[0]
    module_path = os.path.join(HERE, module_file)
    spec = importlib.util.spec_from_file_location(
        module_name, module_path, submodule_search_locations=[HERE])
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    spec.loader.exec_module(module)
    return module


def run_entry(module_file, clsname, methodname, budget_seconds,
               timeout_margin=DEFAULT_TIMEOUT_MARGIN):
    """Runs one config entry to completion. Executes in a worker process of
    this module's own pool (see main()) -- sets the env vars test_leak.py
    (JPYPE_LEAK_BUDGET_SECONDS) and subrun.py (JPYPE_SUBRUN_TIMEOUT) read,
    then drives the named test method the same way pytest would, relying on
    the target class's own subrun.TestCase(individual=True) decoration for
    JVM isolation.
    """
    if HERE not in sys.path:
        sys.path.insert(0, HERE)
    os.environ['JPYPE_LEAK_BUDGET_SECONDS'] = str(budget_seconds)
    os.environ['JPYPE_SUBRUN_TIMEOUT'] = str(budget_seconds + timeout_margin)

    module = _import_module(module_file)
    cls = getattr(module, clsname)

    suite = unittest.TestSuite()
    suite.addTest(cls(methodname))
    result = unittest.TestResult()

    label = "%s:%s.%s" % (module_file, clsname, methodname)
    start = time.monotonic()
    suite.run(result)
    elapsed = time.monotonic() - start

    if result.wasSuccessful():
        return (label, True, elapsed, None)
    problems = result.failures + result.errors
    message = problems[0][1] if problems else "unknown failure"
    return (label, False, elapsed, message)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        'config', nargs='?', default=DEFAULT_CONFIG,
        help="path to a leak-target config file (default: %s)" % DEFAULT_CONFIG)
    parser.add_argument(
        '--jobs', type=int, default=max(1, (os.cpu_count() or 2) // 2),
        help="number of leak-checked entries to run concurrently "
             "(default: one per 2 cores)")
    args = parser.parse_args(argv)

    entries = parse_config(args.config)
    if not entries:
        print("No entries found in %s" % args.config)
        return 1

    failed = []
    with concurrent.futures.ProcessPoolExecutor(max_workers=args.jobs) as pool:
        futures = {pool.submit(run_entry, *entry): entry for entry in entries}
        for future in concurrent.futures.as_completed(futures):
            label, ok, elapsed, message = future.result()
            status = "PASS" if ok else "LEAK"
            print("[%s] %s (%.1fs)" % (status, label, elapsed))
            if not ok:
                failed.append((label, message))

    if failed:
        print()
        print("Leaky entries:")
        for label, message in failed:
            print("  %s" % label)
            if message:
                print("    %s" % message.strip().splitlines()[-1])
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
