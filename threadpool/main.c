#include <stdio.h>
#include <stdlib.h>
#include "threadpool.h"

static void *print_arg(void *arg)
{
    static int next = 0;
    int *pi = (int *)arg;
    printf("arg: %d\n", *pi);
    if (*pi != next) {
        //printf("error, want: %d, get: %d\n", next, *pi);
        //exit(-1);
    }
    ++next;
    return NULL;
}

int main(int argc, char *argv[])
{
    threadpool_t *th = threadpool_create(1256, 1256, sizeof(int));
    if (!th) {
        printf("create threadpool fail\n");
        exit(-1);
    }

    for (int i = 0; i < 30; ++i) {
        threadpool_task_t *task = malloc_task(th);
        if (NULL == task) {
            printf("malloc task fail\n");
            exit(-1);
        }
        task->fun = print_arg;
        *((int *)task->arg) = i;
        threadpool_add_task(th, task);
    }

    sleep(1);
    printf("thread pool free start\n");
    threadpool_free(th);
    printf("thread pool free done\n");

    return 0;
}
