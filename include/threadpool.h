#ifndef THREADPOOL_H_0X2D46A9D7
#define THREADPOOL_H_0X2D46A9D7

#include <stdint.h>
#include "list.h"

typedef struct threadpool_s threadpool_t;

/* 任务 */
typedef struct threadpool_task_s {
    struct list_head  list;
    void *(*fun)(void *);
    void *arg;
} threadpool_task_t;

/* 创建线程池
 * task_size: 任务的固定大小，若不固定则填0
*/
extern threadpool_t *threadpool_create(uint32_t thr_num_min, uint32_t thr_num_max, size_t task_size);
/*销毁线程池*/
//extern int threadpool_destroy(threadpool_t *pool);

/* 申请一个 task(task_size不为0的pool才可调用)
 * 需要先处理 func 函数指针 和 拷贝 buf，才能调用 add_task
 */
extern threadpool_task_t *malloc_task(threadpool_t *pool);

/*
 * 添加任务到线程池任务队列末尾
*/
extern int threadpool_add_task(threadpool_t *pool, threadpool_task_t *task);

/*
 * 向线程池的任务队列中添加一个任务
 * function 需要自己处理 arg 参数的生存周期
*/
extern int threadpool_add_call(threadpool_t *pool, void *(*function)(void *arg), void *arg);

extern int threadpool_free(threadpool_t *pool);
#endif	/* THREADPOOL_H_0X2D46A9D7 */
