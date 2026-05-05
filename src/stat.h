#ifndef _STAT_H_DEFINED
#define _STAT_H_DEFINED

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct MediaStat {
    bool is_animated;
    uint64_t frames;
    int32_t width;
    int32_t height;
    int32_t duration_num;
    int32_t duration_den;
} MediaStat;

static inline void print_stat(const MediaStat* stat)
{
    printf("%d %lu %d %d %d %d\n",
        stat->is_animated,
        stat->frames,
        stat->width,
        stat->height,
        stat->duration_num,
        stat->duration_den);
}

#endif
