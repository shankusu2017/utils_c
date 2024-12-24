#!/bin/sh

rm -rf bin
mkdir bin

gcc tcp_ip/tcp_ip.c ipc/test_client.c hash/hash.c  ipc/ipc_client.c ipc/ipc_io.c ipc/ipc_server.c random/random.c \
threadpool/threadpool.c time/time.c util/util.c log/log.c -Iipc -Iinclude -lpthread -g -o bin/ipc_client

gcc tcp_ip/tcp_ip.c ipc/test_server.c hash/hash.c  ipc/ipc_client.c ipc/ipc_io.c ipc/ipc_server.c random/random.c \
threadpool/threadpool.c time/time.c util/util.c log/log.c -Iipc -Iinclude -lpthread -g -o bin/ipc_server

gcc log/log.c util/util.c time/time.c threadpool/threadpool.c hash/hash.c hash/hash_test.c  \
-Iinclude -lpthread -g -o bin/hash_test

gcc log/log.c util/util.c time/time.c threadpool/threadpool.c hash/hash.c threadpool/thread_test.c  \
-Iinclude -lpthread -g -o bin/thread_test

gcc time/time_test.c log/log.c util/util.c time/time.c threadpool/threadpool.c \
-Iinclude -lpthread -g -o bin/time_test
