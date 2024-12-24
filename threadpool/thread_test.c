#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "threadpool.h"

static int test_arr[1024*1024] = {0};
static int test_arr_seq[1024*1024] = {0};

static void *print_arg(void *arg)
{
    int *pi = (int *)arg;
    printf("arg: %d\n", *pi);
    test_arr[*pi] = *pi;    /* mark run data*/
    return NULL;
}

static void *print_arg_seq(void *arg)
{
    static int seq = 0;
    int *pi = (int *)arg;
    printf("arg: %d\n", *pi);
    assert(seq++ == *pi);       /* 任务被按序执行 */
    test_arr_seq[*pi] = *pi;    /* mark run data*/
    return NULL;
}

static int test_thread_number(void)
{
    uc_threadpool_t *th = uc_threadpool_create(1, 128, sizeof(int));
    if (!th) {
        printf("create threadpool fail\n");
        exit(-1);
    }

    for (int i = 0; i < 512; ++i) {
        uc_threadpool_task_t *task = uc_threadpool_malloc_fixed_task(th);
        if (NULL == task) {
            printf("malloc task fail\n");
            exit(-1);
        }
        task->fun = print_arg;
        *((int *)task->arg) = i;
        uc_threadpool_add_fixed_task(th, task);
    }

    uc_threadpool_wait_task_done(th);

    //sleep(1);
    printf("thread pool close start\n");
    uc_threadpool_close(th);
    printf("thread pool free done\n");
    printf("========> check thread task do start\n");
    for(int i = 0; i < 512; ++i) {
        assert(test_arr[i] == i);
    }
    printf("<======== check thread task do done\n");

    return 0;
}

static int test_thread_seq(void)
{
    uc_threadpool_t *th = uc_threadpool_create(1, 1, sizeof(int));
    if (!th) {
        printf("create threadpool fail\n");
        exit(-1);
    }

    for (int i = 0; i < 1024; ++i) {
        uc_threadpool_task_t *task = uc_threadpool_malloc_fixed_task(th);
        if (NULL == task) {
            printf("malloc task fail\n");
            exit(-1);
        }
        task->fun = print_arg_seq;
        *((int *)task->arg) = i;
        uc_threadpool_add_fixed_task(th, task);
    }

    uc_threadpool_wait_task_done(th);

    printf("thread pool close start\n");
    uc_threadpool_close(th);
    printf("thread pool close end\n");
    sleep(1);

    printf("thread pool free done\n");
    printf("========> 0x512d8088 check thread task do start\n");
    for(int i = 0; i < 1024; ++i) {
        assert(test_arr_seq[i] == i);
    }
    printf("<======== 0x512d8088 check thread task do done\n");

    return 0;
}


int main(int argc, char *argv[])
{
    int ret = test_thread_number() || test_thread_seq();

    return ret;
}
