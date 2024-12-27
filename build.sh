#!/bin/sh

rm -rf lib
mkdir lib

rm -rf bin
mkdir bin

gcc -shared -g  -o lib/libuc.so hash/uc_hash.c ipc/uc_ipc_client.c ipc/uc_ipc_io.c ipc/uc_ipc_server.c log/uc_log.c \
random/uc_random.c tcp_ip/uc_tcp_ip.c threadpool/uc_threadpool.c time/uc_time.c util/uc_util.c -Iinclude -Iinc

gcc tcp_ip/uc_tcp_ip.c ipc/uc_test_client.c hash/uc_hash.c  ipc/uc_ipc_client.c ipc/uc_ipc_io.c ipc/uc_ipc_server.c random/uc_random.c \
threadpool/uc_threadpool.c time/uc_time.c util/uc_util.c log/uc_log.c -Iipc -Iinclude -Iinc -lpthread -g -o bin/uc_ipc_client

gcc tcp_ip/uc_tcp_ip.c ipc/uc_test_server.c hash/uc_hash.c  ipc/uc_ipc_client.c ipc/uc_ipc_io.c ipc/uc_ipc_server.c random/uc_random.c \
threadpool/uc_threadpool.c time/uc_time.c util/uc_util.c log/uc_log.c -Iipc -Iinclude -Iinc -lpthread -g -o bin/uc_ipc_server

gcc log/uc_log.c util/uc_util.c time/uc_time.c threadpool/uc_threadpool.c hash/uc_hash.c hash/uc_hash_test.c  \
-Iinclude -Iinc -lpthread -g -o bin/uc_hash_test

gcc log/uc_log.c util/uc_util.c time/uc_time.c threadpool/uc_threadpool.c hash/uc_hash.c threadpool/uc_thread_test.c  \
-Iinclude -Iinc -lpthread -g -o bin/uc_thread_test

gcc time/uc_time_test.c log/uc_log.c util/uc_util.c time/uc_time.c threadpool/uc_threadpool.c \
-Iinclude -Iinc -lpthread -g -o bin/uc_time_test
