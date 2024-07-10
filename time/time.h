#ifndef TIME_H_2024_07_19_0X34B32598
#define TIME_H_2024_07_19_0X34B32598

#include <stdint.h>

/* 毫秒级别定时器 */
void utils_msleep(unsigned long milli_second);

/* 微妙级别定时器 */
void utils_usleep(long micro_second);

/* 打印毫秒 */
int64_t utils_printfms(void);

#endif /* TIME_H_2024_07_19_0X34B32598 */
