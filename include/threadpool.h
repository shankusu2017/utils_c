#ifndef THREADPOOL_H_2024_07_19_0X2D46A9D7
#define THREADPOOL_H_2024_07_19_0X2D46A9D7


#include <stdint.h>
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/*
 * 向线程池的任务队列中添加一个任务(task_size==0(尺寸不固定))
 * function 需要自己处理 arg 参数的生存周期
*/
extern int threadpool_add_void_task(threadpool_t *pool, void *(*function)(void *arg), void *arg);

/* 申请一个 task(task_size不为0的pool才可调用)
 * 需要设置 func 函数指针、准备好 arg，才能调用 add_task
 */
extern threadpool_task_t *threadpool_malloc_fixed_task(threadpool_t *pool);
/*
 * 设置好 上述 task 后，调用此函数将任务到线程池任务队列末尾
*/
extern int threadpool_add_fixed_task(threadpool_t *pool, threadpool_task_t *task);

/*
 * 等待所有任务完成
 * WARN: 不要同时在其它的线程中添加新的任务
 */
extern void threadpool_wait_task_done(threadpool_t *pool);

/* 关闭运行的子线程，销毁未被执行的任务，释放 pool 结构 */
extern int threadpool_close(threadpool_t *pool);

#ifdef __cplusplus
}
#endif


#endif	/* THREADPOOL_H_2024_07_19_0X2D46A9D7 */
