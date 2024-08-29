#include "time.h"
#include "common.h"

#if 0
/* 毫秒级别定时器 */
void utils_msleep(unsigned long milli_second)
{
    struct timeval tv;

    tv.tv_sec = milli_second / 1000;
    tv.tv_usec = (milli_second % 1000) * 1000;

    int err = 0;
    do{
       err = select(0, NULL, NULL, NULL, &tv);
    } while (err < 0 && errno == EINTR);
}

/* 微妙级别定时器 */
void utils_usleep(long micro_second)
{
	struct timeval tv;

    tv.tv_sec = micro_second / 1000000;
    tv.tv_usec = micro_second % 1000000;
    int err = 0;

    do {
       	err = select(0, NULL, NULL, NULL, &tv);
    } while (err < 0 && errno == EINTR);
}
#endif

int64_t utils_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);

    int s = tv.tv_sec;
    long long ms = ((long long )tv.tv_sec)*1000 + tv.tv_usec/1000;
    //printf("millisecond: %lld, sec: %d\n", ms, s);
    //printf("second: %lld, sec: %d\n", ms, s);
    return ms;
    // printf("microsecond:%ld\n",tv.tv_sec*1000000 + tv.tv_usec);  //微秒
}

int64_t utils_printfms(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);

    int s = tv.tv_sec;
    long long ms = ((long long )tv.tv_sec)*1000 + tv.tv_usec/1000;
    printf("millisecond: %lld\t", ms);
    //printf("second: %lld, sec: %d\n", ms, s);
    return ms;
    // printf("microsecond:%ld\n",tv.tv_sec*1000000 + tv.tv_usec);  //微秒
}

int64_t utils_us(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);

    int s = tv.tv_sec;
    long long us = ((long long )tv.tv_sec)*1000*1000 + tv.tv_usec;
    //printf("millisecond: %lld, sec: %d\n", ms, s);
    //printf("second: %lld, sec: %d\n", ms, s);
    return us;
    // printf("microsecond:%ld\n",tv.tv_sec*1000000 + tv.tv_usec);  //微秒
}

int64_t utils_printfus(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);

    int s = tv.tv_sec;
    long long us = ((long long )tv.tv_sec)*1000*1000 + tv.tv_usec;
    printf("us: %lld\t", us);
    //printf("second: %lld, sec: %d\n", ms, s);
    return us;
    // printf("microsecond:%ld\n",tv.tv_sec*1000000 + tv.tv_usec);  //微秒
}

