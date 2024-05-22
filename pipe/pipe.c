// O_NOATIME 宏定义的扩展要求
#define _GNU_SOURCE 1
#include <fcntl.h>
#undef _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/unistd.h>
#include <assert.h>
#include <limits.h>

static int pip[2];

void sleep_select(int sec)
{
    struct timeval timeout;

    timeout.tv_usec = 0;
    timeout.tv_sec = sec;

    select(0, NULL, NULL, NULL, &timeout);
}

static void *thread_read(void *userdata)
{
    char data[5] = {0};
    int ret;

    printf("p0: %d\n", pip[0]);
    printf("p1: %d\n", pip[1]);

    while (1)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(pip[0], &fds);
        struct timeval timeout = {10, 0};
        ret = select(pip[0] + 1, &fds, NULL, NULL, &timeout);
        if (ret < 0)
        {
            printf("select error\n");
            break;
        }
        else if (ret == 0)
        {
            // printf("no fd ready\n");
            continue;
        }
        else
        {
            if (FD_ISSET(pip[0], &fds) > 0)
            {
                read(pip[0], data, 5);
                printf("data: %s\n", data);
            }
        }
    }
}

void *
thread_write(void)
{
    printf("thread_write\n");
    char data[] = "pipe-data";
    while (1)
    {
        sleep_select(1);
        write(pip[1], data, sizeof(data));
        /* 单次写入的数据长度超过 PIPE_BUF 时， 内核不保证写入的原子性*/
        if (sizeof(data) > PIPE_BUF)
        {
            printf("warn data is too long\n");
        }
    }
}

int main(int argc, char *argv[])
{
    char data[] = "data";
    pthread_t id;
    if (pipe(pip))
    {
        printf("pipe error\n");
        exit(-1);
    }

    printf("p0: %d\n", pip[0]);
    printf("p1: %d\n", pip[1]);

    fcntl(pip[0], F_SETFL, O_NOATIME); /* 提高性能 */
    fcntl(pip[1], F_SETFL, O_NOATIME);

    { // 获取 pipe 容量
        int cap = fcntl(pip[0], F_GETPIPE_SZ);
        printf("pipe.cap:%d\n", cap);
    }

    if (pthread_create(&id, NULL, thread_read, NULL))
    {
        printf("create thread error\n");
        exit(-1);
    }

    printf("pthread id = %x\n", id);
    pthread_create(&id, NULL, thread_write, NULL);

    // signal(SIGUSR1, mysignal);
    while (1)
    {

        sleep_select(2);
        // sleep(2);
        // write(pip[1], data, 5);
    }

    pthread_join(id, NULL);
}
