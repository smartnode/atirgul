
#ifndef NK_CONFIG_H_
#define NK_CONFIG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_LIB
#define NK_INCLUDE_DEFAULT_ALLOCATOR
// #define NK_INCLUDE_FONT_BAKING
// #define NK_INCLUDE_DEFAULT_FONT
// #define NK_INCLUDE_SOFTWARE_FONT
#define NK_IMPLEMENTATION
#define NK_CAIRO_IMPLEMENTATION


#define LOG_LEVEL_FILTER LOG_LEVEL_INF

typedef enum
{
    LOG_LEVEL_NONE = 0, // No logging
    LOG_LEVEL_ERR,      // Error messages
    LOG_LEVEL_INF,      // Informational messages
    LOG_LEVEL_DBG       // Debug messages (most verbose)
} log_level_t;

#define LOG_BASE(level, prefix, fmt, ...)                                 \
    do                                                                    \
    {                                                                     \
        if (level <= LOG_LEVEL_FILTER)                                    \
        {                                                                 \
            fprintf(stderr, "%s %s:%d: %s: " fmt "\n",                    \
                    prefix, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        }                                                                 \
    } while (0)

#define DBG(fmt, ...) LOG_BASE(LOG_LEVEL_DBG, "[DBG]", fmt, ##__VA_ARGS__)
#define INF(fmt, ...) LOG_BASE(LOG_LEVEL_INF, "[INF]", fmt, ##__VA_ARGS__)
#define ERR(fmt, ...) LOG_BASE(LOG_LEVEL_ERR, "[ERR]", fmt, ##__VA_ARGS__)
#define ENT() DBG("++")
#define EXT() DBG("--")

#ifdef __cplusplus
}
#endif

#endif
