#!/bin/sh


rm -rf client server
gcc tcp_ip/tcp_ip.c ipc/test_client.c hash/hash.c ipc/ipc_client.c ipc/ipc_io.c ipc/ipc_server.c random/random.c \
threadpool/threadpool.c time/time.c util/util.c log/log.c -Iipc -Iinclude -lpthread -g -o client

gcc tcp_ip/tcp_ip.c ipc/test_server.c hash/hash.c ipc/ipc_client.c ipc/ipc_io.c ipc/ipc_server.c random/random.c \
threadpool/threadpool.c time/time.c util/util.c log/log.c -Iipc -Iinclude -lpthread -g -o server
