#!/usr/bin/env python3
# convert_jpg_to_rgb565.py
# Usage: python convert_jpg_to_rgb565.py <input_image> <output.rgb565>

import sys
from PIL import Image
import struct

DEFAULT_WIDTH = 320
DEFAULT_HEIGHT = 240

def main():
    if len(sys.argv) != 3 and len(sys.argv) != 5:
        print(f"Usage: {sys.argv[0]} <input_image> <output.rgb565> [width height]")
        print(f"Default: {DEFAULT_WIDTH}x{DEFAULT_HEIGHT}, unless otherwise specified")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    if len(sys.argv) == 5:
        width = int(sys.argv[3])
        height = int(sys.argv[4])
    else:
        width = DEFAULT_WIDTH
        height = DEFAULT_HEIGHT

    if width <= 0 or height <= 0:
        print(f"ERROR: incorrect dimensions")
        sys.exit(1)

    try:
        img = Image.open(input_path).convert("RGB").resize((width, height))
    except FileNotFoundError:
        print(f"Cannot open the file {input_path}")

    with open(output_path, "wb") as f:
        for y in range(height):
            for x in range(width):
                r, g, b = img.getpixel((x, y))
                r5 = r >> 3
                g6 = g >> 2
                b5 = b >> 3
                pixel = (r5 << 11) | (g6 << 5) | b5
                f.write(struct.pack("<H", pixel))

    print(f"Saved {output_path}: {width}x{height}, {width * height * 2} bytes")

if __name__ == "__main__":
    main()