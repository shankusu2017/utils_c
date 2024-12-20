#include "time.h"
#include "common.h"
#include "util.h"
#include "list.h"

typedef struct timer_manager_s {
    struct list_head ms_list_head[1000];
    struct list_head s_list_head[60];
    struct list_head m_list_head[60];
    struct list_head h_list_head[24];
    struct list_head d_list_head[1];
    uint64_t idx;   /* 分配出去的定时器标识符，非0， 递增，唯一 */
} timer_manager_t;

typedef struct timer_item_s {
    uint16_t ms;  /* 毫秒 [0,999] */
    uint16_t s;  /* 秒 [0, 59]   */
    uint16_t m;  /* 分钟 [0, 59] */
    uint16_t h;  /* 小时 [0, 23] */
    uint16_t d;  /* 天 [0, ) */

    uint8_t once; /* 是否重复，0：不重复，others：重复 */

    timer_cb cb;

    uint64_t idx;   /* 计时器标识符 */

    struct list_head  list;	/* 必须放最后面 */
} timer_item_t;

static hash_table_t *timer_hash = NULL;
static timer_manager_t *timer_mgr = NULL;
static pthread_mutex_t timer_mtx = PTHREAD_MUTEX_INITIALIZER;


int timer_init_do(void)
{
    timer_hash = hash_create(1024*256, hash_key_uint64, 0);
    if (NULL == timer_hash) {
        return -0x3302370b;
    }

    timer_mgr = (timer_manager_t *)malloc(sizeof(*timer_mgr));
    if (NULL == timer_mgr) {
        hash_free(timer_hash);
        timer_hash = NULL;
        return -0x3c48acf9;
    }

    // TODO 确定 ARRAY_SIZE 是否对 pointer 起作用
    for (int i = 0; i < ARRAY_SIZE(timer_mgr->ms_list_head); ++i) {
        INIT_LIST_HEAD(&timer_mgr->ms_list_head[i]);
    }
    for (int i = 0; i < ARRAY_SIZE(timer_mgr->s_list_head); ++i) {
        INIT_LIST_HEAD(&timer_mgr->s_list_head[i]);
    }
    for (int i = 0; i < ARRAY_SIZE(timer_mgr->m_list_head); ++i) {
        INIT_LIST_HEAD(&timer_mgr->m_list_head[i]);
    }
    for (int i = 0; i < ARRAY_SIZE(timer_mgr->h_list_head); ++i) {
        INIT_LIST_HEAD(&timer_mgr->h_list_head[i]);
    }
    for (int i = 0; i < ARRAY_SIZE(timer_mgr->d_list_head); ++i) {
        INIT_LIST_HEAD(&timer_mgr->d_list_head[i]);
    }

    timer_mgr->idx = 1;

    return 0;
}

// 创建cb线程池+check线程
int timer_init(void)
{
    pthread_mutex_lock(&timer_mtx);
    int ret = timer_init_do();
    pthread_mutex_unlock(&timer_mtx);
    return ret;
}

static uint64_t timer_add_do(uint64_t millisecond, int once, timer_cb cb)
{
    if (millisecond >= TIMER_INTERVAL_MAX || 0 == millisecond) {
        return 0;
    }
    if (NULL == cb) {
        return 0;
    }

    timer_item_t *item = malloc(sizeof(*item));
    if (NULL == item) {
        return 0;
    }

    item->ms = (uint16_t)(millisecond % 1000);
    item->s = (uint16_t)((millisecond / 1000) % 60);
    item->m = (uint16_t)(((millisecond / 1000) / 60) % 60);
    item->h = (uint16_t)((((millisecond / 1000) / 60) / 60) % 24);
    item->d = (uint16_t)((((millisecond / 1000) / 60) / 60) / 24);

    item->once = once;

    item->cb = cb;

    hash_key_t key = {.u64 = timer_mgr->idx};
    timer_mgr->idx++;
    int ret = hash_insert(timer_hash, key, item);
    if (0 != ret) {
        free(item);
        return 0;
    }
    item->idx = key.u64;

    if (item->d == 0 && item->h == 0 && item->m == 0 && item->s == 0) {
       list_add_tail(&item->list, &timer_mgr->ms_list_head[item->ms]);
    } else if (item->d == 0 && item->h == 0 && item->m == 0) {
       list_add_tail(&item->list, &timer_mgr->s_list_head[item->s]);
    } else if (item->d == 0 && item->h == 0) {
       list_add_tail(&item->list, &timer_mgr->m_list_head[item->m]);
    } else if (item->d == 0) {
       list_add_tail(&item->list, &timer_mgr->h_list_head[item->h]);
    } else {
       list_add_tail(&item->list, &timer_mgr->d_list_head[item->d]);
    }

    return key.u64;
}

uint64_t timer_add(uint64_t millisecond, int once, timer_cb cb)
{
    pthread_mutex_lock(&timer_mtx);
    uint64_t hdl = timer_add_do(millisecond, once, cb);
    pthread_mutex_unlock(&timer_mtx);
    return hdl;
}

static int timer_delete_do(uint64_t hdl)
{
    hash_key_t key = {.u64 = hdl};

    hash_node_t *node = hash_find(timer_hash, key);
    if (NULL == node) {
        return -0x6816560a;
    }

    timer_item_t *item = (timer_item_t*)hash_node_val(node);
    list_del(&item->list);

    hash_delete(timer_hash, key);
    return 0;
}

int timer_delete(uint64_t hdl)
{
    pthread_mutex_lock(&timer_mtx);
    uint64_t hdl = timer_delete_do(hdl);
    pthread_mutex_unlock(&timer_mtx);
    return hdl;
}


#ifndef _WIN32
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

