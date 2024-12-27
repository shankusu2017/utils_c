#include <stdio.h>
#include <stdlib.h>
#include "uc_time.h"

static void *timeout1(void *arg)
{
    printf("0x346efa91 1ms: %ld\n", uc_time_ms());
}

static void *timeout1000(void *arg)
{
    printf("0x1c8919e6 1000ms: %ld\n", uc_time_ms());
}
static void *timeout1001(void *arg)
{
    printf("0x1c8919e6 1001ms: %ld\n", uc_time_ms());
}
static void *timeout1500(void *arg)
{
    printf("0x1c8919e6 1500ms: %ld\n", uc_time_ms());
}


static void *timeout60000(void *arg)
{
    printf("0x3415fe8b 60000ms: %ld\n", uc_time_ms());
}
static void *timeout60001(void *arg)
{
    printf("0x3415fe8b 60001ms: %ld\n", uc_time_ms());
}
static void *timeout90000(void *arg)
{
    printf("0x3415fe8b 90000ms: %ld\n", uc_time_ms());
}


static void *timeout3600000(void *arg)
{
    printf("0x5516542c 3600000ms: %ld\n", uc_time_ms());
}
static void *timeout3600001(void *arg)
{
    printf("0x5516542c 3600001ms: %ld\n", uc_time_ms());
}
static void *timeout5400000(void *arg)
{
    printf("0x5516542c 5400000ms: %ld\n", uc_time_ms());
}

/* 测试程序要比对 timeout 预期被调用次数和实际被调用次数 */
int main(char *argv[], int argc) {
    uc_timer_init();
    uc_timer_add(1, 0, timeout1, NULL);

    uc_timer_add(1000, 0, timeout1000, NULL);
    uc_timer_add(1001, 0, timeout1001, NULL);
    uc_timer_add(1500, 0, timeout1500, NULL);

    uc_timer_add(60000, 0, timeout60000, NULL);
    uc_timer_add(60001, 0, timeout60001, NULL);
    uc_timer_add(90000, 0, timeout90000, NULL);

    while(1) {
        uc_time_msleep(1000*60);
    }
}
