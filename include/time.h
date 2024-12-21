#ifndef TIME_H_2024_07_19_0X34B32598
#define TIME_H_2024_07_19_0X34B32598

#include <stdint.h>

typedef void* (*timer_cb) (void *);

#ifndef TIMER_INTERVAL_MAX
#define TIMER_INTERVAL_MAX (20*365*24*60*60*1000ULL)
#endif /* TIMER_INTERVAL_MAX */

int timer_init(void);

/*
 * millisecond： 超时时间间隔（毫秒）
 * once: 是否仅仅执行一次
 * RETURNS: 0 失败， others: 定时器句柄
 */
void *timer_add(uint64_t millisecond, int once, timer_cb cb, void *arg);

int timer_delete(void *hdl);


/* 毫秒级别定时器 */
void utils_msleep(unsigned long milli_second);

/* 微妙级别定时器 */
void utils_usleep(long micro_second);

/* 返回毫秒 */
int64_t utils_ms(void);
/* 打印并返回毫秒 */
int64_t utils_printfms(void);

/* 返回微秒 */
int64_t utils_us(void);
/* 打印并返回微秒 */
int64_t utils_printfus(void);

#endif /* TIME_H_2024_07_19_0X34B32598 */
