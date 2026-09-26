#!/usr/bin/env python3
# generate_rgb565_example.py

import struct

width, height = 320, 240

with open("test_frame.rgb565", "wb") as f:
    for y in range(height):
        for x in range(width):
            if x < width // 3:
                r, g, b = 31, 0, 0  # red (5-bit)
            elif x < 2 * width // 3:
                r, g, b = 0, 63, 0  # green (6-bit)
            else:
                r, g, b = 0, 0, 31  # blue (5-bit)

            pixel = (r << 11) | (g << 5) | b
            f.write(struct.pack("<H", pixel))
print("Saved test_frame.rgb565: ", width * height * 2, "bytes")
