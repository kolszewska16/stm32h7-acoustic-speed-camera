#include <stdio.h>
#include <stdlib.h>
#include "jpeg_encoder.h"

#define DEFAULT_WIDTH 320
#define DEFAULT_HEIGHT 240

int main(int argc, char *argv[]) {
	if(argc != 3 && argc != 5) {
		printf("Usage: %s <input.rgb565> <output.jpg> [width, height]\n", argv[0]);
		printf("Default %dx%d, unless otherwise specified\n", DEFAULT_WIDTH, DEFAULT_HEIGHT);
		return -1;
	}
	const char *input_path = argv[1];
	const char *output_path = argv[2];
	int width = DEFAULT_WIDTH;
	int height = DEFAULT_HEIGHT;

	if(argc == 5) {
		width = atoi(argv[3]);
		height = atoi(argv[4]);
	}

	if(width <= 0 || height <= 0) {
		printf("ERROR: incorrect dimensions\n");
		return -1;
	}

	FILE *f = fopen(input_path, "rb");
	if(!f) {
		printf("Cannot open %s file\n", input_path);
		return -1;
	}

	size_t expected_size = width * height * 2;
	uint8_t *rgb565_buf = malloc(expected_size);
	if(!rgb565_buf) {
		printf("ERROR: insufficient memory");
		fclose(f);
		return -1;
	}

	size_t read_bytes = fread(rgb565_buf, 1, expected_size, f);
	fclose(f);
	if(read_bytes != expected_size) {
		printf("WARNING: expected %lu bytes, read %lu bytes\n", expected_size, read_bytes);
	}

	uint8_t *jpeg_data;
	uint32_t jpeg_len;
	
	int result = JPEG_Encode(rgb565_buf, width, height, 75, &jpeg_data, &jpeg_len);
	if(result != 0) {
		printf("JPEG compression failed\n");
		free(rgb565_buf);
		return -1;
	}

	printf("JPEG compression successful. JPEG size: %u bytes\n", jpeg_len);
	FILE *out = fopen(output_path, "wb");
	if(!out) {
		printf("Cannot save %s file\n", output_path);
		free(rgb565_buf);
		return -1;
	}

	fwrite(jpeg_data, 1, jpeg_len, out);
	fclose(out);
	printf("Saved %s file\n", output_path);

	free(rgb565_buf);
	return 0;
}
