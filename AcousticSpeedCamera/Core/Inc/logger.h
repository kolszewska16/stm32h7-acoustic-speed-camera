#ifndef INC_LOGGER_H_
#define INC_LOGGER_H_

#include <stdarg.h>

typedef enum {
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_ERROR,
} log_level_t;

void log_msg(log_level_t level, const char *msg, ...);

#define LOG_INFO(...)		log_msg(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_WARNING(...)	log_msg(LOG_LEVEL_WARNING, __VA_ARGS__)
#define LOG_ERROR(...)		log_msg(LOG_LEVEL_ERROR, __VA_ARGS__)

#endif /* INC_LOGGER_H_ */
