#include "jpeg_encoder.h"
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static uint8_t jpeg_out_buf[16384];
static uint32_t jpeg_out_len;

static void jpeg_write_callback(void *context, void *data, int size) {
	if(data == NULL || size <= 0) {
		return;
	}

	if(jpeg_out_len + size <= sizeof(jpeg_out_buf)) {
		memcpy(jpeg_out_buf + jpeg_out_len, data, size);
		jpeg_out_len += size;
	}
}

static void rgb565_to_rgb888(const uint8_t *rgb565, uint8_t *rgb888,
	int width, int height)
{
	if(rgb565 == NULL || rgb888 == NULL || width <= 0 || height <= 0) {
		return;
	}

	const uint16_t *src = (const uint16_t *)rgb565;

	for(int i = 0; i < width * height; i++) {
		uint16_t pixel = src[i];
		uint8_t r = (pixel >> 11) & 0x1F;
		uint8_t g = (pixel >> 5) & 0x3F;
		uint8_t b = pixel & 0x1F;

		rgb888[i * 3 + 0] = (r << 3) | (r >> 2);	// 5-bit -> 8-bit
		rgb888[i * 3 + 1] = (g << 2) | (g >> 4);	// 6-bit -> 8-bit
		rgb888[i * 3 + 2] = (b << 3) | (b >> 2);	// 5-bit -> 8-bit
	}
}

int32_t JPEG_Encode(const uint8_t *rgb565_buf, int width, int height,
	int quality, uint8_t **out_buf, uint32_t *out_len)
{
	if(rgb565_buf == NULL || width <= 0 || height <= 0 ||
		out_buf == NULL || out_len == NULL)
	{
		return -1;
	}

	static uint8_t rgb888_buf[160 * 120 * 3];
	if((uint32_t)(width * height * 3) > sizeof(rgb888_buf)) {
		return -1;
	}

	rgb565_to_rgb888(rgb565_buf, rgb888_buf, width, height);

	jpeg_out_len = 0;
	int result = stbi_write_jpg_to_func(jpeg_write_callback, NULL,
		width, height, 3, rgb888_buf, quality);
	if(!result) {
		return -1;
	}

	*out_buf = jpeg_out_buf;
	*out_len = jpeg_out_len;

	return 0;
}
