#include "time.h"
#include "common.h"
#include "util.h"
#include "list.h"

typedef struct timer_manager_s {
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

    threadpool_t *threadpool;
} timer_manager_t;

typedef struct timer_item_s {
    uint64_t interval;  /* 超时时间间隔(毫秒) */
    int once;           /* 是否重复，0：不重复，others：重复 */

    /* 下一次超时的时间戳 */
    uint16_t ms;    /* 毫秒 [0,999] */
    uint16_t s;     /* 秒 [0, 59]   */
    uint16_t m;     /* 分钟 [0, 59] */
    uint16_t h;     /* 小时 [0, 23] */
    uint16_t d;     /* 天 [0, ) */

    timer_cb cb;
    void *arg;

    struct list_head  list;	/* 按惯例放最后 */
} timer_item_t;

static timer_manager_t *timer_mgr = NULL;
static pthread_mutex_t timer_mtx = PTHREAD_MUTEX_INITIALIZER;

static void timer_node_reinsert(timer_item_t *timer_node)
{
    uint64_t millisecond_next = (timer_mgr->ms_idx 
        + timer_mgr->s_idx * 1000
        + timer_mgr->m_idx * 1000 * 60
        + timer_mgr->h_idx * 1000 * 60 * 60
        + timer_mgr->d_idx * 1000 * 60 * 60 * 24)
        + timer_node->interval;

    timer_node->ms = (uint16_t)(millisecond_next % 1000);
    timer_node->s = (uint16_t)((millisecond_next / 1000) % 60);
    timer_node->m = (uint16_t)(((millisecond_next / 1000) / 60) % 60);
    timer_node->h = (uint16_t)((((millisecond_next / 1000) / 60) / 60) % 24);
    timer_node->d = (uint16_t)((((millisecond_next / 1000) / 60) / 60) / 24);

    // printf("0x56a2c36c timer_mgr(ms: %u, s: %u, m: %u, h: %u, d: %u)\n",
    //         timer_mgr->ms_idx, timer_mgr->s_idx, timer_mgr->m_idx, timer_mgr->h_idx, timer_mgr->d_idx);
    // printf("0x42ecb618 time_node(interval: %ld, ms: %u, s: %u, m: %u, h: %u, d: %u)\n",
    //         timer_node->interval, timer_node->ms, timer_node->s, timer_node->m, timer_node->h, timer_node->d);

    if (timer_node->d && timer_node->d != timer_mgr->d_idx) {
        list_add_tail(&timer_node->list, &timer_mgr->d_list_head[0]);
    } else if (timer_node->h && timer_node->h != timer_mgr->h_idx) {
        list_add_tail(&timer_node->list, &timer_mgr->h_list_head[timer_node->h]);
    } else if (timer_node->m && timer_node->m != timer_mgr->m_idx) {
        list_add_tail(&timer_node->list, &timer_mgr->m_list_head[timer_node->m]);
    } else if (timer_node->s && timer_node->s != timer_mgr->s_idx) {
        list_add_tail(&timer_node->list, &timer_mgr->s_list_head[timer_node->s]);
    } else {
        list_add_tail(&timer_node->list, &timer_mgr->ms_list_head[timer_node->ms]);
    }
}

static void *timer_check_timeout(void *arg)
{
    UNUSED(arg);

    uint64_t us_next = utils_us() + 1000 * 1;   /* 微秒 */

    while (1) {        
        pthread_mutex_lock(&timer_mtx);

        /* 毫秒是最低刻度了，每次 嘀嗒都往前走一格 */
        {
            // 记录当前时间
            timer_mgr->ms_idx++;
            timer_mgr->ms_idx %= 1000;

            struct list_head *node;
            struct list_head *node_n;

            timer_item_t *timer_node;
            list_for_each_safe (node, node_n, &timer_mgr->ms_list_head[timer_mgr->ms_idx]) {
                timer_node = list_entry(node, timer_item_t, list);
                threadpool_add_void_task(timer_mgr->threadpool, timer_node->cb, timer_node->arg);

                if (0 != timer_node->once) {
                    list_del(node);
                    timer_node_reinsert(timer_node);
                } else {
                    free(timer_node);
                    timer_node = NULL;
                }
            }
        }
        if (0 == timer_mgr->ms_idx) {
            timer_mgr->s_idx++;
            timer_mgr->s_idx %= 60;

            struct list_head *node;
            struct list_head *node_n;

            timer_item_t *timer_node;
            list_for_each_safe (node, node_n, &timer_mgr->s_list_head[timer_mgr->s_idx]) {
                timer_node = list_entry(node, timer_item_t, list);
                if (0 != timer_node->ms) {  /* 尝试从 高层的轮往低一层的轮 移动*/
                    list_del(node);
                    list_add_tail(&timer_node->list, &timer_mgr->ms_list_head[timer_node->ms]);
                } else {
                    threadpool_add_void_task(timer_mgr->threadpool, timer_node->cb, timer_node->arg);
                    if (0 != timer_node->once) {
                        list_del(node);
                        timer_node_reinsert(timer_node);
                    } else {
                        free(timer_node);
                        timer_node = NULL;
                    }
                }
            }
        }
        if (0 == timer_mgr->ms_idx &&
            0 == timer_mgr->s_idx) {
            timer_mgr->m_idx++;
            timer_mgr->m_idx %= 60;

            struct list_head *node;
            struct list_head *node_n;

            timer_item_t *timer_node;
            list_for_each_safe (node, node_n, &timer_mgr->m_list_head[timer_mgr->m_idx]) {
                timer_node = list_entry(node, timer_item_t, list);
                if (0 != timer_node->s) {
                    list_del(node);
                    list_add_tail(&timer_node->list, &timer_mgr->s_list_head[timer_node->s]);
                } else if (0 != timer_node->ms) {
                    list_del(node);
                    list_add_tail(&timer_node->list, &timer_mgr->ms_list_head[timer_node->ms]);
                } else {
                    threadpool_add_void_task(timer_mgr->threadpool, timer_node->cb, timer_node->arg);
                    if (0 != timer_node->once) {
                        list_del(node);
                        timer_node_reinsert(timer_node);
                    } else {
                        free(timer_node);
                        timer_node = NULL;
                    }
                }
            }
        }
        if (0 == timer_mgr->ms_idx &&
            0 == timer_mgr->s_idx &&
            0 == timer_mgr->m_idx) {
            timer_mgr->h_idx++;
            timer_mgr->h_idx %= 24;

            struct list_head *node;
            struct list_head *node_n;

            timer_item_t *timer_node;
            list_for_each_safe (node, node_n, &timer_mgr->h_list_head[timer_mgr->h_idx]) {
                timer_node = list_entry(node, timer_item_t, list);
                if (0 != timer_node->m) {   /* 尝试从 高层的轮往低一层的轮 移动*/
                    list_del(node);
                    list_add_tail(&timer_node->list, &timer_mgr->m_list_head[timer_node->m]);
                } else if (0 != timer_node->s) {    /* 尝试从 高层的轮往低二层的轮 移动*/
                    list_del(node);
                    list_add_tail(&timer_node->list, &timer_mgr->s_list_head[timer_node->s]);
                } else if (0 != timer_node->ms) {   /* 尝试从 高层的轮往低三层的轮 移动*/
                    list_del(node);
                    list_add_tail(&timer_node->list, &timer_mgr->ms_list_head[timer_node->ms]);
                } else {   /* 刚好所有低级的轮的刻度都为0，当前表上所有低级别的刻度也是0，
                            * 最高一级轮的时间也相同， 触发了 timeout, eg: timer上的刻度是"9:00:00:000", 当前表的指针也是 9:00:00:000。
                            * 上下注释相同
                            */
                    threadpool_add_void_task(timer_mgr->threadpool, timer_node->cb, timer_node->arg);
                    if (0 != timer_node->once) {
                        list_del(node);
                        timer_node_reinsert(timer_node);
                    } else {
                        free(timer_node);
                        timer_node = NULL;
                    }
                }
            }
        }  
        if (0 == timer_mgr->ms_idx &&
            0 == timer_mgr->s_idx &&
            0 == timer_mgr->m_idx &&
            0 == timer_mgr->h_idx) {
            timer_mgr->d_idx++;

            struct list_head *node;
            struct list_head *node_n;

            timer_item_t *timer_node;
            list_for_each_safe (node, node_n, &timer_mgr->d_list_head[0]) {
                timer_node = list_entry(node, timer_item_t, list);
                if (timer_node->d == timer_mgr->d_idx) {
                    if (0 != timer_node->h) {
                        list_del(node);
                        list_add_tail(&timer_node->list, &timer_mgr->h_list_head[timer_node->h]);
                    } else if (0 != timer_node->m) {
                        list_del(node);
                        list_add_tail(&timer_node->list, &timer_mgr->m_list_head[timer_node->m]);
                    } else if (0 != timer_node->s) {
                        list_del(node);
                        list_add_tail(&timer_node->list, &timer_mgr->s_list_head[timer_node->s]);
                    } else if (0 != timer_node->ms) {
                        list_del(node);
                        list_add_tail(&timer_node->list, &timer_mgr->ms_list_head[timer_node->ms]);
                    } else {
                        threadpool_add_void_task(timer_mgr->threadpool, timer_node->cb, timer_node->arg);
                        if (0 != timer_node->once) {
                            list_del(node);
                            timer_node_reinsert(timer_node);
                        } else {
                            free(timer_node);
                            timer_node = NULL;
                        }
                    }
                }
            }
        }

        pthread_mutex_unlock(&timer_mtx);

        /* 计算下次的 checkpoint */
        uint64_t us_now = utils_us();
        /* s上面的代码执行耗时既然超过了1ms*/
        if (us_now > us_next) {
            us_next += 1000;
            continue;   
        } else {
            uint64_t us_left = us_next - us_now;
            us_next += 1000;
            utils_usleep(us_left);
            continue;
        }
    }

    return NULL;
}

static int timer_init_do(void)
{
    timer_mgr = (timer_manager_t *)malloc(sizeof(*timer_mgr));
    if (NULL == timer_mgr) {
        return -0x3c48acf9;
    }

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

    timer_mgr->ms_idx = timer_mgr->s_idx = timer_mgr->m_idx = timer_mgr->h_idx = timer_mgr->d_idx = 0;

    timer_mgr->threadpool = threadpool_create(4, 32, 0);
    if (NULL == timer_mgr->threadpool) {
        return -0x51a7fab4;
    }

    int ret = threadpool_add_void_task(timer_mgr->threadpool, timer_check_timeout, NULL);
    if (0 != ret) {
        return -0x583f7a7e;
    }

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

static void *timer_add_do(uint64_t millisecond, int once, timer_cb cb, void *arg)
{
    if (millisecond >= TIMER_INTERVAL_MAX || 0 == millisecond) {
        return NULL;
    }
    if (NULL == cb) {
        return NULL;
    }

    timer_item_t *item = malloc(sizeof(*item));
    if (NULL == item) {
        return NULL;
    }

    item->interval = millisecond;
    item->once = once;

    uint64_t millisecond_next = (timer_mgr->ms_idx 
                                + timer_mgr->s_idx * 1000
                                + timer_mgr->m_idx * 1000 * 60
                                + timer_mgr->h_idx * 1000 * 60 * 60
                                + timer_mgr->d_idx * 1000 * 60 * 60 * 24)
                                + millisecond;

    item->ms = (uint16_t)(millisecond_next % 1000);
    item->s = (uint16_t)((millisecond_next / 1000) % 60);
    item->m = (uint16_t)(((millisecond_next / 1000) / 60) % 60);
    item->h = (uint16_t)((((millisecond_next / 1000) / 60) / 60) % 24);
    item->d = (uint16_t)((((millisecond_next / 1000) / 60) / 60) / 24);

    item->cb = cb;
    item->arg = arg;

    if (item->d) {
       list_add_tail(&item->list, &timer_mgr->d_list_head[0]);
    } else if (item->h) {
       list_add_tail(&item->list, &timer_mgr->h_list_head[item->h]);
    } else if (item->m) {
       list_add_tail(&item->list, &timer_mgr->m_list_head[item->m]);
    } else if (item->s) {
       list_add_tail(&item->list, &timer_mgr->s_list_head[item->s]);
    } else {
       list_add_tail(&item->list, &timer_mgr->ms_list_head[item->ms]);
    }

    return item;
}

void *timer_add(uint64_t millisecond, int once, timer_cb cb, void *arg)
{
    pthread_mutex_lock(&timer_mtx);
    void *hdl = timer_add_do(millisecond, once, cb, arg);
    pthread_mutex_unlock(&timer_mtx);
    return hdl;
}

static int timer_delete_do(void *hdl)
{
    if (NULL == hdl) {
        return -0x6816560a;
    }

    timer_item_t *item = (timer_item_t*)hdl;
    list_del(&item->list);
    free(item);
    item = NULL;

    return 0;
}

int timer_delete(void *hdl)
{
    pthread_mutex_lock(&timer_mtx);
    int ret = timer_delete_do(hdl);
    pthread_mutex_unlock(&timer_mtx);
    return ret;
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

