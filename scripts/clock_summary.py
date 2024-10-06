#!/bin/env python3
import json


def main():
    with open("runs/wokwi/final/metrics.json", "r") as f:
        data = json.load(f)

    with open("src/config_merged.json", "r") as f:
        config = json.load(f)

    clock_period = config["CLOCK_PERIOD"]

    # ss = slow-slow
    # ff = fast-fast
    # tt = typical-typical

    # ws = worst slack
    # wns = worst negative slack
    # tns = total negative slack

    corners = dict()
    for key, value in data.items():
        fields = key.split(":")
        if len(fields) != 2:
            continue
    
        sub_keys = fields[0].split("__")
        if len(sub_keys) != 4:
            continue
        if sub_keys[0] != "timing" or sub_keys[2] != "ws" or sub_keys[3] != "corner":
            continue

        key = sub_keys[1]
        corner = fields[1]

        if not corner in corners:
            corners[corner] = {}
        
        corners[corner][key] = value

    print("# Clock summary")
    print("| Corner | Hold slack | Setup slack | fMax |")
    print("|--------|-----------:|------------:|-----:|")
    for corner, slack in corners.items():
        hold_slack = slack["hold"]
        setup_slack = slack["setup"]
        min_clock_period = clock_period - setup_slack
        max_freq = 1000 / min_clock_period
        print(f"| {corner} | {hold_slack:.3f} ns | {setup_slack:.3f} ns | {max_freq:0.1f} MHz |")


if __name__ == '__main__':
    main()