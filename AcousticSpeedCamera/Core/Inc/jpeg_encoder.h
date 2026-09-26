#ifndef INC_JPEG_ENCODER_H_
#define INC_JPEG_ENCODER_H_

#include <stdint.h>

int32_t JPEG_Encode(const uint8_t *rgb565_buf, int width, int height,
	int quality, uint8_t **out_buf, uint32_t *out_len);

#endif /* INC_JPEG_ENCODER_H_ */
