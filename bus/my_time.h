#ifndef MY_TIME_H
#define MY_TIME_H
#include <stdint.h>
#include <time.h>
#include <assert.h>

#define DATETIME_MAX_DECIMALS 6


#define MY_PACKED_TIME_GET_INT_PART(x)     ((x) >> 24)
#define MY_PACKED_TIME_GET_FRAC_PART(x)    ((x) % (1LL << 24))
#define MY_PACKED_TIME_MAKE(i, f)          ((((int64_t) (i)) << 24) + (f))
#define MY_PACKED_TIME_MAKE_INT(i)         ((((int64_t) (i)) << 24))

int64_t my_time_packed_from_binary(const char *ptr, uint32_t &pos,  uint32_t dec);

int64_t my_datetime_packed_from_binary(const char *ptr, uint32_t &pos, uint32_t dec);

void my_timestamp_from_binary(struct timeval *tm, const char *ptr, uint32_t &pos, uint32_t dec);
#endif
