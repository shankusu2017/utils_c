#include "time.h"
#include "util.h"
#include "threadpool.h"
#include "list.h"

/*
 * TODO: uc_timer_check_timeout tick的精度可以根据下一个timer_out的时间来确定，没必要每1ms检查一次（或者将时间轮改为有序的列表（eg: 跳表？））
 */

typedef struct uc_timer_manager_s {
    struct list_head ms_list_head[1000];
    struct list_head s_list_head[60];
    struct list_head m_list_head[60];
    struct list_head h_list_head[24];
    struct list_head d_list_head[1];    /* 最后一个刻度轮了，size==1 */

    /* 时间轮当前指向的刻度 */
    uint16_t ms_idx;
    uint16_t s_idx;
    uint16_t m_idx;
    uint16_t h_idx;
    uint16_t d_idx;

    uc_threadpool_t *threadpool;
} uc_timer_manager_t;

typedef struct uc_timer_item_s {
    uint64_t interval;      /* 超时时间间隔(毫秒) */
    uint64_t ttl;         /* 超时剩余次数，0：无限，others：剩余次数 */

    /* 下一次超时的时间戳 */
    uint16_t ms;    /* 毫秒 [0,999] */
    uint16_t s;     /* 秒 [0, 59]   */
    uint16_t m;     /* 分钟 [0, 59] */
    uint16_t h;     /* 小时 [0, 23] */
    uint16_t d;     /* 天 [0, ) */

    uc_timer_cb cb;
    void *arg;

    struct list_head  list;	/* 按惯例放最后 */
} uc_timer_item_t;

static uc_timer_manager_t *uc_timer_mgr = NULL;
static pthread_mutex_t uc_timer_mtx = PTHREAD_MUTEX_INITIALIZER;

static void uc_timer_node_reinsert(uc_timer_item_t *timer_node)
{
    uint64_t millisecond_next = (uc_timer_mgr->ms_idx
        + uc_timer_mgr->s_idx * 1000
        + uc_timer_mgr->m_idx * 1000 * 60
        + uc_timer_mgr->h_idx * 1000 * 60 * 60
        + uc_timer_mgr->d_idx * 1000 * 60 * 60 * 24)
        + timer_node->interval;

    timer_node->ms = (uint16_t)(millisecond_next % 1000);
    timer_node->s = (uint16_t)((millisecond_next / 1000) % 60);
    timer_node->m = (uint16_t)(((millisecond_next / 1000) / 60) % 60);
    timer_node->h = (uint16_t)((((millisecond_next / 1000) / 60) / 60) % 24);
    timer_node->d = (uint16_t)((((millisecond_next / 1000) / 60) / 60) / 24);

    // printf("0x56a2c36c uc_timer_mgr(ms: %u, s: %u, m: %u, h: %u, d: %u)\n",
    //         uc_timer_mgr->ms_idx, uc_timer_mgr->s_idx, uc_timer_mgr->m_idx, uc_timer_mgr->h_idx, uc_timer_mgr->d_idx);
    // printf("0x42ecb618 time_node(interval: %ld, ms: %u, s: %u, m: %u, h: %u, d: %u)\n",
    //         timer_node->interval, timer_node->ms, timer_node->s, timer_node->m, timer_node->h, timer_node->d);

    if (timer_node->d && timer_node->d != uc_timer_mgr->d_idx) {
        list_add_tail(&timer_node->list, &uc_timer_mgr->d_list_head[0]);
    } else if (timer_node->h && timer_node->h != uc_timer_mgr->h_idx) {
        list_add_tail(&timer_node->list, &uc_timer_mgr->h_list_head[timer_node->h]);
    } else if (timer_node->m && timer_node->m != uc_timer_mgr->m_idx) {
        list_add_tail(&timer_node->list, &uc_timer_mgr->m_list_head[timer_node->m]);
    } else if (timer_node->s && timer_node->s != uc_timer_mgr->s_idx) {
        list_add_tail(&timer_node->list, &uc_timer_mgr->s_list_head[timer_node->s]);
    } else {
        list_add_tail(&timer_node->list, &uc_timer_mgr->ms_list_head[timer_node->ms]);
    }
}

static void *uc_timer_check_timeout(void *arg)
{
    UNUSED(arg);


    while (1) {
        uint64_t us_begin = uc_time_us();
        uint64_t us_end = us_begin + 1 * 1000;

        pthread_mutex_lock(&uc_timer_mtx);

        /* 毫秒是最低刻度了，每次 嘀嗒都往前走一格 */
        {
            // 记录当前时间
            uc_timer_mgr->ms_idx++;
            uc_timer_mgr->ms_idx %= 1000;

            struct list_head *node;
            struct list_head *node_n;

            uc_timer_item_t *timer_node;
            list_for_each_safe (node, node_n, &uc_timer_mgr->ms_list_head[uc_timer_mgr->ms_idx]) {
                timer_node = list_entry(node, uc_timer_item_t, list);
                uc_threadpool_add_void_task(uc_timer_mgr->threadpool, timer_node->cb, timer_node->arg);

                /* 无限次数 */
                if (0 == timer_node->ttl) {
                    list_del(node);
                    uc_timer_node_reinsert(timer_node);
                } else {
                    timer_node->ttl--;
                    /* 剩余次数耗尽 */
                    if (0 == timer_node->ttl) {
                        list_del(node);
                        // free(timer_node);
                        // timer_node = NULL;
                    } else {
                        list_del(node);
                        uc_timer_node_reinsert(timer_node);
                    }
                }
            }
        }
        if (0 == uc_timer_mgr->ms_idx) {
            uc_timer_mgr->s_idx++;
            uc_timer_mgr->s_idx %= 60;

            struct list_head *node;
            struct list_head *node_n;

            uc_timer_item_t *timer_node;
            list_for_each_safe (node, node_n, &uc_timer_mgr->s_list_head[uc_timer_mgr->s_idx]) {
                timer_node = list_entry(node, uc_timer_item_t, list);
                if (0 != timer_node->ms) {  /* 尝试从 高层的轮往低一层的轮 移动*/
                    list_del(node);
                    list_add_tail(&timer_node->list, &uc_timer_mgr->ms_list_head[timer_node->ms]);
                } else {
                    uc_threadpool_add_void_task(uc_timer_mgr->threadpool, timer_node->cb, timer_node->arg);
                    if (0 == timer_node->ttl) {
                        list_del(node);
                        uc_timer_node_reinsert(timer_node);
                    } else {
                        timer_node->ttl--;
                        if (0 == timer_node->ttl) {
                            list_del(node);
                            // free(timer_node);
                            // timer_node = NULL;
                        } else {
                            list_del(node);
                            uc_timer_node_reinsert(timer_node);
                        }
                    }
                }
            }
        }
        if (0 == uc_timer_mgr->ms_idx &&
            0 == uc_timer_mgr->s_idx) {
            uc_timer_mgr->m_idx++;
            uc_timer_mgr->m_idx %= 60;

            struct list_head *node;
            struct list_head *node_n;

            uc_timer_item_t *timer_node;
            list_for_each_safe (node, node_n, &uc_timer_mgr->m_list_head[uc_timer_mgr->m_idx]) {
                timer_node = list_entry(node, uc_timer_item_t, list);
                if (0 != timer_node->s) {
                    list_del(node);
                    list_add_tail(&timer_node->list, &uc_timer_mgr->s_list_head[timer_node->s]);
                } else if (0 != timer_node->ms) {
                    list_del(node);
                    list_add_tail(&timer_node->list, &uc_timer_mgr->ms_list_head[timer_node->ms]);
                } else {
                    uc_threadpool_add_void_task(uc_timer_mgr->threadpool, timer_node->cb, timer_node->arg);
                    if (0 == timer_node->ttl) {
                        list_del(node);
                        uc_timer_node_reinsert(timer_node);
                    } else {
                        timer_node->ttl--;
                        if (0 == timer_node->ttl) {
                            list_del(node);
                            // free(timer_node);
                            // timer_node = NULL;
                        } else {
                            list_del(node);
                            uc_timer_node_reinsert(timer_node);
                        }
                    }
                }
            }
        }
        if (0 == uc_timer_mgr->ms_idx &&
            0 == uc_timer_mgr->s_idx &&
            0 == uc_timer_mgr->m_idx) {
            uc_timer_mgr->h_idx++;
            uc_timer_mgr->h_idx %= 24;

            struct list_head *node;
            struct list_head *node_n;

            uc_timer_item_t *timer_node;
            list_for_each_safe (node, node_n, &uc_timer_mgr->h_list_head[uc_timer_mgr->h_idx]) {
                timer_node = list_entry(node, uc_timer_item_t, list);
                if (0 != timer_node->m) {   /* 尝试从 高层的轮往低一层的轮 移动*/
                    list_del(node);
                    list_add_tail(&timer_node->list, &uc_timer_mgr->m_list_head[timer_node->m]);
                } else if (0 != timer_node->s) {    /* 尝试从 高层的轮往低二层的轮 移动*/
                    list_del(node);
                    list_add_tail(&timer_node->list, &uc_timer_mgr->s_list_head[timer_node->s]);
                } else if (0 != timer_node->ms) {   /* 尝试从 高层的轮往低三层的轮 移动*/
                    list_del(node);
                    list_add_tail(&timer_node->list, &uc_timer_mgr->ms_list_head[timer_node->ms]);
                } else {   /* 刚好所有低级的轮的刻度都为0，当前表上所有低级别的刻度也是0，
                            * 最高一级轮的时间也相同， 触发了 timeout, eg: timer上的刻度是"9:00:00:000", 当前表的指针也是 9:00:00:000。
                            * 上下注释相同
                            */
                    uc_threadpool_add_void_task(uc_timer_mgr->threadpool, timer_node->cb, timer_node->arg);
                    if (0 == timer_node->ttl) {
                        list_del(node);
                        uc_timer_node_reinsert(timer_node);
                    } else {
                        timer_node->ttl--;
                        if (0 == timer_node->ttl) {
                            list_del(node);
                            // free(timer_node);
                            // timer_node = NULL;
                        } else {
                            list_del(node);
                            uc_timer_node_reinsert(timer_node);
                        }
                    }
                }
            }
        }
        if (0 == uc_timer_mgr->ms_idx &&
            0 == uc_timer_mgr->s_idx &&
            0 == uc_timer_mgr->m_idx &&
            0 == uc_timer_mgr->h_idx) {
            uc_timer_mgr->d_idx++;

            struct list_head *node;
            struct list_head *node_n;

            uc_timer_item_t *timer_node;
            list_for_each_safe (node, node_n, &uc_timer_mgr->d_list_head[0]) {
                timer_node = list_entry(node, uc_timer_item_t, list);
                if (timer_node->d == uc_timer_mgr->d_idx) {
                    if (0 != timer_node->h) {
                        list_del(node);
                        list_add_tail(&timer_node->list, &uc_timer_mgr->h_list_head[timer_node->h]);
                    } else if (0 != timer_node->m) {
                        list_del(node);
                        list_add_tail(&timer_node->list, &uc_timer_mgr->m_list_head[timer_node->m]);
                    } else if (0 != timer_node->s) {
                        list_del(node);
                        list_add_tail(&timer_node->list, &uc_timer_mgr->s_list_head[timer_node->s]);
                    } else if (0 != timer_node->ms) {
                        list_del(node);
                        list_add_tail(&timer_node->list, &uc_timer_mgr->ms_list_head[timer_node->ms]);
                    } else {
                        uc_threadpool_add_void_task(uc_timer_mgr->threadpool, timer_node->cb, timer_node->arg);
                        if (0 == timer_node->ttl) {
                            list_del(node);
                            uc_timer_node_reinsert(timer_node);
                        } else {
                            timer_node->ttl--;
                            if (0 == timer_node->ttl) {
                                list_del(node);
                                // free(timer_node);
                                // timer_node = NULL;
                            } else {
                                list_del(node);
                                uc_timer_node_reinsert(timer_node);
                            }
                        }
                    }
                }
            }
        }

        pthread_mutex_unlock(&uc_timer_mtx);

        /* 计算下次的 checkpoint */
        uint64_t us_mid = uc_time_us();
        /* 上面的代码执行耗时既然超过了1ms，或者中途系统时间被调整到了未来某个点 */
        if (us_mid >= us_end) {
            continue;
        } else if (us_mid < us_begin) { /* 时间被调整到了过去 */
            continue;
        } else {
            uint64_t us_left = us_end - us_mid;
            uc_time_usleep(us_left);
            continue;
        }
    }

    return NULL;
}

static int uc_timer_init_do(void)
{
    /* 无需重复初始化 */
    if (NULL != uc_timer_mgr) {
        return 0;
    }

    uc_timer_mgr = (uc_timer_manager_t *)malloc(sizeof(*uc_timer_mgr));
    if (NULL == uc_timer_mgr) {
        return -0x3c48acf9;
    }

    for (int i = 0; i < ARRAY_SIZE(uc_timer_mgr->ms_list_head); ++i) {
        INIT_LIST_HEAD(&uc_timer_mgr->ms_list_head[i]);
    }
    for (int i = 0; i < ARRAY_SIZE(uc_timer_mgr->s_list_head); ++i) {
        INIT_LIST_HEAD(&uc_timer_mgr->s_list_head[i]);
    }
    for (int i = 0; i < ARRAY_SIZE(uc_timer_mgr->m_list_head); ++i) {
        INIT_LIST_HEAD(&uc_timer_mgr->m_list_head[i]);
    }
    for (int i = 0; i < ARRAY_SIZE(uc_timer_mgr->h_list_head); ++i) {
        INIT_LIST_HEAD(&uc_timer_mgr->h_list_head[i]);
    }
    for (int i = 0; i < ARRAY_SIZE(uc_timer_mgr->d_list_head); ++i) {
        INIT_LIST_HEAD(&uc_timer_mgr->d_list_head[i]);
    }

    uc_timer_mgr->ms_idx = uc_timer_mgr->s_idx = uc_timer_mgr->m_idx = uc_timer_mgr->h_idx = uc_timer_mgr->d_idx = 0;

    uc_timer_mgr->threadpool = uc_threadpool_create(4, 32);
    if (NULL == uc_timer_mgr->threadpool) {
        free(uc_timer_mgr);
        uc_timer_mgr = NULL;
        return -0x51a7fab4;
    }

    int ret = uc_threadpool_add_void_task(uc_timer_mgr->threadpool, uc_timer_check_timeout, NULL);
    if (0 != ret) {
        free(uc_timer_mgr);
        uc_timer_mgr = NULL;
        return -0x583f7a7e;
    }

    return 0;
}

// 创建cb线程池+check线程
int uc_timer_init(void)
{
    pthread_mutex_lock(&uc_timer_mtx);
    int ret = uc_timer_init_do();
    pthread_mutex_unlock(&uc_timer_mtx);
    return ret;
}

static void *uc_timer_add_do(uint64_t millisecond, uint64_t ttl, uc_timer_cb cb, void *arg)
{
    if (millisecond >= TIMER_INTERVAL_MAX || 0 == millisecond) {
        return NULL;
    }
    if (NULL == cb) {
        return NULL;
    }

    uc_timer_item_t *item = malloc(sizeof(*item));
    if (NULL == item) {
        return NULL;
    }

    item->interval = millisecond;
    item->ttl = ttl;

    uint64_t millisecond_next = (uc_timer_mgr->ms_idx
                                + uc_timer_mgr->s_idx * 1000
                                + uc_timer_mgr->m_idx * 1000 * 60
                                + uc_timer_mgr->h_idx * 1000 * 60 * 60
                                + uc_timer_mgr->d_idx * 1000 * 60 * 60 * 24)
                                + millisecond;

    item->ms = (uint16_t)(millisecond_next % 1000);
    item->s = (uint16_t)((millisecond_next / 1000) % 60);
    item->m = (uint16_t)(((millisecond_next / 1000) / 60) % 60);
    item->h = (uint16_t)((((millisecond_next / 1000) / 60) / 60) % 24);
    item->d = (uint16_t)((((millisecond_next / 1000) / 60) / 60) / 24);

    item->cb = cb;
    item->arg = arg;

    if (item->d) {
       list_add_tail(&item->list, &uc_timer_mgr->d_list_head[0]);
    } else if (item->h) {
       list_add_tail(&item->list, &uc_timer_mgr->h_list_head[item->h]);
    } else if (item->m) {
       list_add_tail(&item->list, &uc_timer_mgr->m_list_head[item->m]);
    } else if (item->s) {
       list_add_tail(&item->list, &uc_timer_mgr->s_list_head[item->s]);
    } else {
       list_add_tail(&item->list, &uc_timer_mgr->ms_list_head[item->ms]);
    }

    return item;
}

void *uc_timer_add(uint64_t millisecond, uint64_t ttl, uc_timer_cb cb, void *arg)
{
    pthread_mutex_lock(&uc_timer_mtx);
    void *hdl = uc_timer_add_do(millisecond, ttl, cb, arg);
    pthread_mutex_unlock(&uc_timer_mtx);
    return hdl;
}

static int uc_timer_delete_do(void *hdl)
{
    if (NULL == hdl) {
        return -0x6816560a;
    }

    uc_timer_item_t *item = (uc_timer_item_t*)hdl;
    list_del(&item->list);
    free(item);
    item = NULL;

    return 0;
}

int uc_timer_delete(void *hdl)
{
    pthread_mutex_lock(&uc_timer_mtx);
    int ret = uc_timer_delete_do(hdl);
    pthread_mutex_unlock(&uc_timer_mtx);
    return ret;
}


#ifndef _WIN32
/* 毫秒级别定时器 */
void uc_time_msleep(unsigned long milli_second)
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
void uc_time_usleep(long micro_second)
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

int64_t uc_time_ms(void)
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

int64_t uc_time_us(void)
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


