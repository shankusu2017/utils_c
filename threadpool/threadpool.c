#include "common.h"
#include "threadpool.h"

/* 任务 */
typedef struct threadpool_pid_s {
    struct list_head  list;
    pthread_t pid;
} threadpool_pid_t;

/* 线程池对象 */
struct threadpool_s {
    pthread_mutex_t lock;  /* 同步锁 */

    /* 任务队列信息 */
    pthread_cond_t cond_task;
    struct list_head list_task_head;
    size_t list_task_ttl;

    size_t list_task_max;    /* 任务队列最大的排队数，超过则阻塞到任务数量下降后添加才返回 */

    /* 任务的大小，
     * 0：没有固定值，task 无法重复利用
     * others; 重用 task, 避免频繁的内存分配
     */
    size_t task_size;
    struct list_head list_free_head;    /* 空闲队列，避免频繁分配内存*/

    /* 线程池信息 */
    int thread_min;		/* 线程池中最小、最大、存活的线程数 */
    int thread_max;
    int thread_alive;
    struct list_head list_pid_head; /* 存活的线程的 pid 组成的队列 */

    int shutdown;       /* 0：不用关闭, others: 需要关闭 */
};

/* 工作线程流 */
static void *threadpool_thread_work(void *threadpool);

/*创建线程池*/
threadpool_t *
threadpool_create(uint32_t thr_num_min, uint32_t thr_num_max, size_t task_size)
{
    threadpool_t *pool = NULL;
    do {
        if ((pool = (threadpool_t *)calloc(1, sizeof(threadpool_t))) == NULL) {
            printf("malloc threadpool false; \n");
            break;
        }

        /* 初始化同步锁 */
        if (pthread_mutex_init(&(pool->lock), NULL) != 0) {
            printf("init lock or cond false;\n");
            break;
        }

        /* 初始化任务队列 */
        if (pthread_cond_init(&(pool->cond_task), NULL) != 0) {
            printf("init lock or cond false;\n");
            break;
        }
        INIT_LIST_HEAD(&pool->list_task_head);
        pool->list_task_ttl = 0;

        pool->list_task_max = 0;

        pool->task_size = task_size;
        INIT_LIST_HEAD(&pool->list_free_head);


        /* 初始化线程变量 */
        if ((thr_num_max == 0) || (thr_num_max < thr_num_min)) {
            printf("thr num invalid, min: %u, max: %u", thr_num_min, thr_num_max);
            break;
        }
        pool->thread_min = thr_num_min;
        pool->thread_max = thr_num_max;
        pool->thread_alive = 0;
        INIT_LIST_HEAD(&pool->list_pid_head);

        pool->shutdown = 0;

        /* 启动工作线程 */
        for (int i = 0; i < thr_num_min; i++) {
            threadpool_pid_t *pinfo = (threadpool_pid_t *)malloc(sizeof(threadpool_pid_t));
            if (pinfo == NULL) {
                break;
            }

            int ret = pthread_create(&pinfo->pid, NULL, threadpool_thread_work, (void *)pool);
			if (ret != 0) {
                break;  /* 初始化阶段，创建失败，可以直接退出 */
			}
            pool->thread_alive++;
            list_add_tail(&pinfo->list, &pool->list_pid_head);
            printf("new pthread.id: %ld\n", pinfo->pid);
        }

        return pool;
    } while (0);

    /* 释放pool的空间 */
    threadpool_close(pool);
    return NULL;
}

/* 关闭并销毁线程池 */
int
threadpool_close(threadpool_t *pool)
{
    if (pool == NULL) {
        return -0x61729e08;
    }

    /* 通知所属线程，执行完任务后退出线程 */
    pthread_mutex_lock(&(pool->lock));
    pool->shutdown = 1;
    pthread_cond_broadcast(&(pool->cond_task));
    pthread_mutex_unlock(&(pool->lock));


    /* 等待线程结束 */
    int close_thread_ttl = 0;
    int again = 1;
    while (again) {
        again = 0;
        pthread_mutex_lock(&(pool->lock));
        if (list_empty(&pool->list_pid_head)) {
            pthread_mutex_unlock(&(pool->lock));
        } else {
            again = 1;
            struct list_head *node = pool->list_pid_head.next;
            threadpool_pid_t *pid = list_entry(node, threadpool_pid_t, list);
            list_del(&pid->list);
            pthread_mutex_unlock(&(pool->lock));
            printf("wait thread: %ld exit\n", pid->pid);
            pthread_join(pid->pid, NULL);
            free(pid);
            close_thread_ttl++;
        }
    }

    int close_free_task_ttl = 0;
    pthread_mutex_lock(&(pool->lock));
    again = 1;
    while (again) {
        again = 0;
        struct list_head *node = NULL;
        list_for_each (node, &pool->list_free_head) {
            again = 1;
            threadpool_task_t *task = list_entry(node, threadpool_task_t, list);
            list_del(&task->list);
            free(task);
            close_free_task_ttl++;
            break;
        }
    }
    int close_task_ttl = 0;
    again = 1;
    while (again) {
        again = 0;
        struct list_head *node = NULL;
        list_for_each (node, &pool->list_task_head) {
            again = 1;
            threadpool_task_t *task = list_entry(node, threadpool_task_t, list);
            list_del(&task->list);
            free(task);
            close_task_ttl++;
            break;
        }
    }
    pthread_mutex_unlock(&(pool->lock));
    printf("0x07f7c8ae close thread: %d, close free task: %d, close task: %d\n",
        close_thread_ttl, close_free_task_ttl, close_task_ttl);

    return 0;
}

/* 某个任务处理完后，进行收尾处理 */
static void threadpool_task_done(threadpool_t *pool)
{
    (void *)pool;
}

/*工作线程*/
static void *
threadpool_thread_work(void *arg)
{
    threadpool_t *pool = (threadpool_t *)arg;
    threadpool_task_t *task = NULL;
    struct list_head *node = NULL;

    while (1) {
        pthread_mutex_lock(&(pool->lock));
        while (pool->shutdown == 0 && list_empty(&pool->list_task_head)) {
            pthread_cond_wait(&(pool->cond_task), &(pool->lock));
        }

        /* 收到关闭信号, 关闭线程 */
        if (pool->shutdown != 0) {
            pool->thread_alive--;
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        /* 新的任务到来信号，执行新任务 */
        node = pool->list_task_head.next;
        list_del(node);
        pool->list_task_ttl--;
        pthread_cond_signal(&pool->cond_task);
        pthread_mutex_unlock(&(pool->lock));

        task = list_entry(node, threadpool_task_t, list);
        if (task->fun) {
            /* 执行指定任务 */
            task->fun(task->arg);
        } else {
            /* 可能是退出信号的 task */
        }
        /* 放入回收队列 */
        pthread_mutex_lock(&(pool->lock));
        if (pool->task_size == 0) {
            //free(task->arg); 函数自己释放 arg
            task->arg = NULL;
        }
        list_add_tail(node, &pool->list_free_head);
        pthread_mutex_unlock(&(pool->lock));

        /* 看看是否需要主动退出... */
        threadpool_task_done(pool);
    }
}

threadpool_task_t *threadpool_malloc_fixed_task(threadpool_t *pool)
{
    /* 任务大小不固定，无法 alloc 固定大小内存*/
    if (pool->task_size == 0) {
        return NULL;
    }

    pthread_mutex_lock(&(pool->lock));
    if (pool->shutdown) {
        printf("threadpool has closed\n");
        pthread_mutex_unlock(&(pool->lock));
        return NULL;
    }

    threadpool_task_t *task = NULL;
    /* 尝试从 free 链表中取 */
    if (!list_empty(&pool->list_free_head)) {
        //printf("0x572e7736 malloc task from free list\n");
        struct list_head *node = pool->list_free_head.next;
        list_del(node);
        pthread_mutex_unlock(&(pool->lock));
        task = list_entry(node, threadpool_task_t, list);
        return task;
    }
    pthread_mutex_unlock(&(pool->lock));

    task = malloc(sizeof(threadpool_task_t) + pool->task_size);
    if (task == NULL) {
        return NULL;
    }
    task->list.next = task->list.next = NULL;
    task->fun = NULL;
    task->arg = task + 1;   /* 指向后面的数据域 */

    return task;
}

/* 添加某个任务时，尝试创建新的线程 */
static int threadpool_activty_try_do(threadpool_t *pool)
{
    /* 线程数量达到上限 */
    if (pool->thread_alive >= pool->thread_max) {
        return 0;
    }

    /* 每个线程剩下不到一个待处理任务 */
    if (pool->thread_alive > pool->list_task_ttl) {
        return 0;
    }

    threadpool_pid_t *pinfo = (threadpool_pid_t *)malloc(sizeof(threadpool_pid_t));
    if (pinfo == NULL) {
        return -0x77094f73;
    }

    int ret = pthread_create(&pinfo->pid, NULL, threadpool_thread_work, (void *)pool);
    if (ret != 0) {
        return -0x3c5a31ed;
    }
    pool->thread_alive++;
    list_add_tail(&pinfo->list, &pool->list_pid_head);

    printf("pool->task.ttl: %ld, pool->thread.alive: %d\n", pool->list_task_ttl, pool->thread_alive);

    return 0;
}

static int threadpool_activty_try(threadpool_t *pool)
{
    pthread_mutex_lock(&(pool->lock));
    int ret = threadpool_activty_try_do(pool);
    pthread_mutex_unlock(&(pool->lock));
    return ret;
}

/*向线程池的任务队列中添加一个任务*/
int threadpool_add_fixed_task(threadpool_t *pool, threadpool_task_t *task)
{
    if (NULL == task->fun) {
        printf("func is nil\n");
        return -0x76a9ba0f;
    }

    pthread_mutex_lock(&(pool->lock));
    if (pool->shutdown) {
        printf("threadpool has closed\n");
        pthread_mutex_unlock(&(pool->lock));
        return -0x7d1a4672;
    }

    list_add_tail(&task->list, &pool->list_task_head);
    pool->list_task_ttl++;

    /* 添加完任务后,队列就不为空了,唤醒线程池中的一个线程 */
    pthread_cond_signal(&(pool->cond_task));
    pthread_mutex_unlock(&(pool->lock));

    threadpool_activty_try(pool);
    return 0;
}

static int threadpool_add_to_void_task_do(threadpool_t *pool, void *(*fun)(void *arg), void *arg, int forcetoHead)
{
    threadpool_task_t *task = NULL;
    if (pool->task_size != 0) {
        printf("must call add task\n");
        return -0x76eb870c;
    }

    pthread_mutex_lock(&(pool->lock));
    if (pool->shutdown) {
        printf("threadpool has closed\n");
        pthread_mutex_unlock(&(pool->lock));
        return -0x7d1a4672;
    }

    while (pool->list_task_max && pool->list_task_ttl >= pool->list_task_max) {
        pthread_cond_wait(&pool->cond_task, &pool->lock);
    }

    if (!list_empty(&pool->list_free_head)) {
        struct list_head *node = pool->list_free_head.next;
        list_del(node);
        task = list_entry(node, threadpool_task_t, list);
    } else {
        task = (threadpool_task_t *)malloc(sizeof(threadpool_task_t));
        if (task == NULL) {
            return -0x18aa3f60;
        }
    }

    task->fun = fun;
    task->arg = arg;
    if (forcetoHead) {
        list_add(&task->list, &pool->list_task_head);
    } else {
        list_add_tail(&task->list, &pool->list_task_head);
    }
    pool->list_task_ttl++;

    /*
     * 1： 先解锁再唤醒，那么消费者可能处于不公平地位（考虑解锁后，唤醒前又来了生产者）
     * 2： 先唤醒再解锁，那么消费者和新的生产者将处于平等的竞争地位（也更利于task被快速消耗）
     */
    pthread_cond_signal(&(pool->cond_task));
    pthread_mutex_unlock(&(pool->lock));

    threadpool_activty_try(pool);

    return 0;
}

/* 向线程池的任务队列中末尾添加一个任务 */
int threadpool_add_void_task(threadpool_t *pool, void *(*fun)(void *arg), void *arg)
{
    return threadpool_add_to_void_task_do(pool, fun, arg, 0);
}
/* 往线程池任务队列的头部插入一个任务 */
int threadpool_insert_void_task_at_head(threadpool_t *pool, void *(*fun)(void *arg), void *arg)
{
    return threadpool_add_to_void_task_do(pool, fun, arg, 1);
}

/* 等待任务队列被取空 */
extern void threadpool_wait_task_done(threadpool_t *pool)
{
    pthread_mutex_lock(&pool->lock);
    while (!list_empty(&pool->list_task_head)) {
        pthread_mutex_unlock(&pool->lock);
        sleep(1);
        pthread_mutex_lock(&(pool->lock));
    }
    pthread_mutex_unlock(&(pool->lock));
}

extern size_t threadpool_set_task_max(threadpool_t *pool, size_t max)
{
    pthread_mutex_lock(&pool->lock);
    size_t old = pool->list_task_max;
    pool->list_task_max = max;
    pthread_cond_signal(&pool->cond_task);
    pthread_mutex_unlock(&pool->lock);
    return old;
}
