#define RC_CAR_LOG_NONE 0
#define RC_CAR_LOG_ERROR 1
#define RC_CAR_LOG_INFO 2
#define RC_CAR_LOG_DEBUG 3

#ifndef RC_CAR_DEBUG_LEVEL
#define RC_CAR_DEBUG_LEVEL RC_CAR_LOG_INFO
#endif

#if RC_CAR_DEBUG_LEVEL < RC_CAR_LOG_NONE || RC_CAR_DEBUG_LEVEL > RC_CAR_LOG_DEBUG
#error "RC_CAR_DEBUG_LEVEL must be between 0 (none) and 3 (debug)"
#endif

#if RC_CAR_DEBUG_LEVEL >= RC_CAR_LOG_ERROR
#define RC_LOG_ERROR(...) do { Serial.printf(__VA_ARGS__); } while (0)
#else
#define RC_LOG_ERROR(...) do {} while (0)
#endif

#if RC_CAR_DEBUG_LEVEL >= RC_CAR_LOG_INFO
#define RC_LOG_INFO(...) do { Serial.printf(__VA_ARGS__); } while (0)
#else
#define RC_LOG_INFO(...) do {} while (0)
#endif

#if RC_CAR_DEBUG_LEVEL >= RC_CAR_LOG_DEBUG
#define RC_LOG_DEBUG(...) do { Serial.printf(__VA_ARGS__); } while (0)
#else
#define RC_LOG_DEBUG(...) do {} while (0)
#endif
