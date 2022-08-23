#!/usr/bin/env python3

# Parses trace log of metamix run and shows events in linear timeline on stdout
# usage: util/traceparser.py < metamix.log

from collections import namedtuple
import csv
import re
import sys
from typing import List, Union

InputFound = namedtuple('InputFound', ['time', 'clock_ts', 'input', 'kind'])
OutputBegin = namedtuple('OutputBegin',
                         ['time', 'clock_ts', 'input', 'kind', 'num'])
OutputEnd = namedtuple('OutputEnd',
                       ['time', 'clock_ts', 'input', 'kind', 'num'])

Event = Union[InputFound, OutputBegin, OutputEnd]

events: List[Event] = []


def process_input_found(match):
    return [
        InputFound(match["time"], int(match["ts"]), match["input"],
                   match["kind"])
    ]


def process_output_found(match):
    return [
        OutputBegin(match["time"], int(match["start"]), match["input"],
                    match["kind"], match["num"]),
        OutputEnd(match["time"], int(match["end"]), match["input"],
                  match["kind"], match["num"])
    ]


def process_output_notfound(match):
    return [
        OutputBegin(match["time"], int(match["start"]), match["input"],
                    match["kind"], 0),
        OutputEnd(match["time"], int(match["end"]), match["input"],
                  match["kind"], 0)
    ]


processors = [
    (re.compile(
        "\[(?P<time>[^\]]+)\]\s*\[input:(?P<input>[a-z]+)\s*\].*Found (?P<kind>[A-Z ]+) at pts [0-9-]+, rescaled ClockTS\{(?P<ts>[0-9-]+)\}"
    ), process_input_found),
    (re.compile(
        "\[(?P<time>[^\]]+)\]\s*\[output\s+\].*Found (?P<num>[0-9-]+) (?P<kind>[A-Z ]+) at pts \[ClockTS\{(?P<start>[0-9-]+)\}, ClockTS\{(?P<end>[0-9-]+)\}\)\s*:(?P<input>.+)"
    ), process_output_found),
    (re.compile(
        "\[(?P<time>[^\]]+)\]\s*\[output\s+\].*No (?P<kind>[A-Z ]+) at pts \[ClockTS\{(?P<start>[0-9-]+)\}, ClockTS\{(?P<end>[0-9-]+)\}\)\s*:(?P<input>.+)"
    ), process_output_notfound)
]


def process_line(line: str):
    for (regex, fun) in processors:
        m = regex.match(line)
        if m:
            evs = fun(m)
            for ev in evs:
                events.append(ev)


try:
    for line in sys.stdin:
        process_line(line)
except BrokenPipeError:
    pass

w = csv.DictWriter(
    sys.stdout,
    fieldnames=['Time', 'Input name', 'Event', 'ClockTS', 'Kind', 'Count'])
w.writeheader()
for ev in events:
    d = {
        'Event': type(ev).__name__,
        'Time': ev.time,
        'ClockTS': ev.clock_ts,
        'Input name': ev.input,
        'Kind': ev.kind,
        'Count': getattr(ev, 'num', -1)
    }
    w.writerow(d)
