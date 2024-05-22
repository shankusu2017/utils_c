#ifndef THREADPOOL_H_0X2D46A9D7
#define THREADPOOL_H_0X2D46A9D7

typedef struct threadpool_t threadpool_t;

/*创建线程池*/
extern threadpool_t *threadpool_create(int min_thr_num, int max_thr_num, int queue_max_size);
/*销毁线程池*/
extern int threadpool_destroy(threadpool_t *pool);
/*向线程池的任务队列中添加一个任务*/
extern int threadpool_add_task(threadpool_t *pool, void *(*function)(void *arg), void *arg);

#endif	/* THREADPOOL_H_0X2D46A9D7 */