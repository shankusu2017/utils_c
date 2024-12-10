#!/bin/sh


rm -rf bin/ipc_server_tes
rm -rf bin/ipc_client_test
rm -rf bin/hash_test

gcc tcp_ip/tcp_ip.c ipc/test_client.c hash/hash.c  ipc/ipc_client.c ipc/ipc_io.c ipc/ipc_server.c random/random.c \
threadpool/threadpool.c time/time.c util/util.c log/log.c -Iipc -Iinclude -lpthread -g -o bin/ipc_client

gcc tcp_ip/tcp_ip.c ipc/test_server.c hash/hash.c  ipc/ipc_client.c ipc/ipc_io.c ipc/ipc_server.c random/random.c \
threadpool/threadpool.c time/time.c util/util.c log/log.c -Iipc -Iinclude -lpthread -g -o bin/ipc_server

gcc log/log.c util/util.c time/time.c hash/hash.c hash/hash_test.c  \
-Iinclude -lpthread -g -o bin/hash_test
