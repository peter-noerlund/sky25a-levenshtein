#!/bin/env python3


import argparse
from xml.dom.minidom import Document


def to_svg(mag_file: str, svg_file: str) -> None:
    with open(mag_file, "r") as f:
        mag_lines = f.readlines()

    if mag_lines[0].strip().split(" ")[0] != "magic":
        raise Exception("Not a MAGIC file")

    doc = Document()
    svg = doc.createElement("svg")
    svg.setAttribute("xmlns", "http://www.w3.org/2000/svg")
    doc.appendChild(svg)

    g = doc.createElement("g")
    svg.appendChild(g)

    scale = 10

    doc_width = 0
    doc_height = 0

    use_fields = None
    matrix = None
    for line in mag_lines:
        fields = line.strip().split(" ")
        if fields[0] == "use":
            use_fields = fields
        elif fields[0] == "transform":
            matrix = (
                (int(fields[1]), int(fields[4]), 0),
                (int(fields[2]), int(fields[5]), 0),
                (int(fields[3]), int(fields[6]), 1)
            )
        elif fields[0] == "box":
            rect = doc.createElement("rect")
            
            box = (int(fields[1]), int(fields[2]), int(fields[3]), int(fields[4]))

            left = box[0] + matrix[2][0]
            top = box[1] + matrix[2][1]
            right = left + box[2]
            bottom = top + box[3]

            #left = left * matrix[0][0] + top * matrix[1][0] + matrix[2][0]
            #top = left * matrix[0][1] + top * matrix[1][1] + matrix[2][1]

            #right = right * matrix[0][0] + bottom * matrix[1][0] + matrix[2][0]
            #bottom = right * matrix[0][1] + bottom * matrix[1][1] + matrix[2][1]

            #if right < left:
            #    tmp = left
            #    left = right
            #    right = left
            #if bottom < top:
            #    tmp = top
            #    top = bottom
            #    bottom = top

            #print(((left, top), (right, bottom)))

            x = left / scale
            y = top / scale
            width = (right - left) / scale
            height = (bottom - top) / scale

            name_fields = use_fields[3].split(".")
            if name_fields[0] == "levenshtein_ctrl":
                fill = "#ff7f7f"
                stroke = "#ff0000"
            elif name_fields[0] == "uart":
                fill = "#7fff7f"
                stroke = "#00ff00"
            elif name_fields[0] == "spi_ctrl":
                fill = "#7f7fff"
                stroke = "#0000ff"
            elif name_fields[0] == "intercon":
                fill = "#ffff7f"
                stroke = "#ffff00"
            elif name_fields[0][0:7] == "clkbuf_":
                fill = "#7fffff"
                stroke = "#ff0000"
            elif name_fields[0][0:7] == "FILLER_" or name_fields[0][0:4] == "TAP_":
                fill = "#cccccc"
                stroke = "#aaaaaa"
            else:
                fill = "#7f7f7f"
                stroke = "#000000"

            rect.setAttribute("x", f"{x:0.1f}")
            rect.setAttribute("y", f"{y:0.1f}")
            rect.setAttribute("width", f"{width:0.1f}")
            rect.setAttribute("height", f"{height:0.1f}")
            rect.setAttribute("fill", fill)
            rect.setAttribute("stroke", stroke)

            g.appendChild(rect)

            if x + width > doc_width:
                doc_width = x + width
            if y + height > doc_height:
                doc_height = y + height

    svg.setAttribute("width", f"{doc_width:0.1f}")
    svg.setAttribute("height", f"{doc_height:0.1f}")
    
    with open(svg_file, "w+") as f:
        doc.writexml(f)



def analyze(filename: str) -> None:
    with open(filename, "r") as f:
        lines = f.readlines()

    modules = {}

    cell_type = None
    cell_name = None
    for line in lines:
        fields = line.strip().split(" ")
        if fields[0] == "use":
            cell_type = fields[1]
            name_parts = fields[3].split(".")
            if len(name_parts) > 1:
                cell_name = ".".join(name_parts[:-1])
            else:
                cell_name = name_parts[0]
        elif fields[0] == "box" and cell_type is not None and cell_name is not None:
            units = int((int(fields[1]) + int(fields[3])) / 92)

            if cell_name not in modules:
                modules[cell_name] = {
                    "units": units,
                    "cell_count": 1,
                    "cells": {
                        cell_type: {
                            "units": units,
                            "count": 1
                        }
                    }
                }
            else:
                modules[cell_name]["units"] += units
                modules[cell_name]["cell_count"] += 1
                if cell_type not in modules[cell_name]["cells"]:
                    modules[cell_name]["cells"][cell_type] = {
                        "units": units,
                        "count": 1
                    }
                else:
                    modules[cell_name]["cells"][cell_type]["units"] += units
                    modules[cell_name]["cells"][cell_type]["count"] += 1
            
            cell_type = None
            cell_name = None

    total_units = 0
    for module_name, module in modules.items():
        total_units += module["units"]
        if module_name[0:4] == "TAP_" or module_name[0:4] == "PHY_" or module_name[0:7] == "FILLER_":
            continue
        print(f"{module_name}:")
        print(f"  Total cells: {module['cell_count']} ({module['units']} units)")
        for cell_type, cell in module["cells"].items():
            print(f"    {cell_type}: {cell['count']} cells ({cell['units']} units)")
    print(f"Total units: {total_units}")


def main():
    parser = argparse.ArgumentParser(description="Analyze MAGIC file")
    parser.add_argument("file")
    
    args = parser.parse_args()

    mag_file = args.file
    svg_file = f"{mag_file}.svg"

    to_svg(mag_file, svg_file)


if __name__ == '__main__':
    main()