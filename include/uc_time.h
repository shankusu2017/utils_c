#ifndef TIME_H_2024_07_19_0X34B32598
#define TIME_H_2024_07_19_0X34B32598

#include <stdint.h>

typedef void* (*uc_timer_cb) (void *);

#ifndef TIMER_INTERVAL_MAX
#define TIMER_INTERVAL_MAX (20*365*24*60*60*1000ULL)
#endif /* TIMER_INTERVAL_MAX */

/*
 * 初始化定时器组件
*/
int uc_timer_init(void);

/*
 * millisecond： 超时时间间隔（毫秒）
 * ttl: 定时器超时次数（0：无限制），N：n次后停止定时逻辑
 * RETURNS: 0 失败， others: 定时器句柄(需要调用timer_delete 删除定时器避免内存泄露)
 */
void *uc_timer_add(uint64_t millisecond, uint64_t ttl, uc_timer_cb cb, void *arg);

/*
 * 删除定时器
 */
int uc_timer_delete(void *hdl);


/* 毫秒级别 sleep */
void uc_time_msleep(unsigned long milli_second);
/* 微妙级别 sleep */
void uc_time_usleep(long micro_second);

/* 返回毫秒级别时间戳 */
int64_t uc_time_ms(void);
/* 返回微秒级别时间戳 */
int64_t uc_time_us(void);

#endif /* TIME_H_2024_07_19_0X34B32598 */
