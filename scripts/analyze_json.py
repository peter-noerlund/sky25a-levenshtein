#!/bin/env python3


import argparse
import json


def analyze(filename: str) -> None:
    with open(filename, "r") as f:
        data = json.load(f)

    modules = {}
    for module_name, module in data["modules"].items():
        for cell_name, cell in module["cells"].items():
            path_parts = cell_name.split(".")
            path = ".".join(path_parts[:-1])
            if path not in modules:
                modules[path] = {}
            cell_type = cell["type"]
            if cell_type not in modules[path]:
                modules[path][cell_type] = 1
            else:
                modules[path][cell_type] += 1

    for path, module in modules.items():
        print(f"{path}:")
        for cell_type, count in module.items():
            print(f"    {cell_type}: {count}")


def main():
    parser = argparse.ArgumentParser(description="Analyze Yosys json file")
    parser.add_argument("file")
    
    args = parser.parse_args()

    analyze(args.file)


if __name__ == '__main__':
    main()