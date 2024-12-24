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

static int test_thread_number(void)
{
    uc_threadpool_t *th = uc_threadpool_create(1, 128);
    if (!th) {
        printf("create threadpool fail\n");
        exit(-1);
    }

    for (int i = 0; i < 512; ++i) {
        int *task = malloc(sizeof(*task));
        if (NULL == task) {
            printf("malloc task fail\n");
            exit(-1);
        }
        *task = i;
        uc_threadpool_add_void_task(th, print_arg, task);
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

int main(int argc, char *argv[])
{
    int ret = test_thread_number();

    return ret;
}
