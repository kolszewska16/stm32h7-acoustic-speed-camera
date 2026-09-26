# JPEG encoder - PC-side verification
Standalone test harness to verify the RGB565->RGB888->JPEG encoding logic (using stb_image_write) on a PC before integrating it into the STM32 firmware. Useful for isolating logic bugs from hardware/memory when debugging on target.

## Requirements
- C compiler (gcc/clang/MSVC) supporting C99
- CMake ≥ 3.10
- Python3 with the Pillow library, for the image conversion

Install Python dependencies (recommend: use a virtual environment):

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```
Remember to activate the virtual environment (`source .venv/bin/activate`) each time before running the Python scripts in this folder.

## Build 
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Generate a test RGB frame
```bash
python3 generate_rgb565_example.py
```

## Generate a test frame from an image
```bash
python3 convert_jpg_to_rgb565.py <input_image> <output_image> [width height]
```
Defaults to 320x240 if width and height are not given.

Example:
```bash
python3 convert_jpg_to_rgb565.py test_details_lines.jpg test_details_lines.rgb565
python3 convert_jpg_to_rgb565.py test_gradient_sky.jpg test_gradient_sky.rgb565
```

## Run the encoder
Example (run from the root directory)
```bash
./build/encode_jpeg test_frame.rgb565 output_frame.jpg
./build/encode_jpeg test_details_lines.rgb565 output_details_lines.jpg
./build/encode_jpeg test_gradient_sky.rgb565 output_gradient_sky.jpg
```

Example (run from the `build/` directory, so paths to input/output files are relative to `build/` unless given an absolute or `../` paths):
```bash
cd build
./encode_jpeg ../test_frame.rgb565 ../output_frame.jpg
./encode_jpeg ../test_details_lines.rgb565 ../output_details_lines.jpg
./encode_jpeg ../test_gradient_sky.rgb565 ../output_gradient_sky.jpg
```

## Notes
- The internal RGB888 conversion buffer is currently sized for 320x240. Using other dimensions requires updating `jpeg_encoder.c` to allocate this buffer dynamically.
- JPEG quality is currently fixed at 75 in `main.c`. Lower/higher values trade off file size against visual quality.
