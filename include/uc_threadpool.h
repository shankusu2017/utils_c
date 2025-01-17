#ifndef THREADPOOL_H_2024_07_19_0X2D46A9D7
#define THREADPOOL_H_2024_07_19_0X2D46A9D7


#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct uc_threadpool_s uc_threadpool_t;

/* 创建线程池
 * task_size: 任务的固定大小，若不固定则填0
*/
extern uc_threadpool_t *uc_threadpool_create(uint32_t thr_num_min, uint32_t thr_num_max);

/*
 * 向线程池的任务队列中添加一个任务(task_size==0(尺寸不固定)),放到队列的最后面
 * function 需要自己处理 arg 参数的生存周期
*/
extern int uc_threadpool_add_void_task(uc_threadpool_t *pool, void *(*function)(void *arg), void *arg);
/* 放到队列的最前面 */
extern int uc_threadpool_insert_void_task(uc_threadpool_t *pool, void *(*function)(void *arg), void *arg);

/*
 * 等待任务队列被取空
 * WARN: 不要同时在其它的线程中添加新的任务
 */
extern void uc_threadpool_wait_task_done(uc_threadpool_t *pool);

extern size_t uc_threadpool_set_task_max(uc_threadpool_t *pool, size_t max);

/* 关闭运行的子线程，销毁未被执行的任务，释放 pool 结构 */
extern int uc_threadpool_close(uc_threadpool_t *pool);

#ifdef __cplusplus
}
#endif


#endif	/* THREADPOOL_H_2024_07_19_0X2D46A9D7 */
