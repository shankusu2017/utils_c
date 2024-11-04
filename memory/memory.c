#include "memory.h"

/* zmalloc - total amount of allocated memory aware version of malloc()
 *
 * Copyright (c) 2006-2009, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>

static size_t used_memory = 0;

/* 申请内存, 自带size.head
 * mem: size.head + free.mem */
void *util_malloc(size_t size)
{
	/* The  malloc()  function  allocates  size bytes and returns a pointer to the allocated memory.  
	 * The memory is not initialized */
    void *ptr = malloc(size+sizeof(size_t));

    if (!ptr) return NULL;
    *((size_t*)ptr) = size;
    used_memory += size+sizeof(size_t);
    return (char*)ptr+sizeof(size_t);
}

void *util_calloc(size_t size)
{
    /* The  malloc()  function  allocates  size bytes and returns a pointer to the allocated memory.  
	 * The memory is not initialized */
    void *ptr = calloc(1, size+sizeof(size_t));

    if (!ptr) return NULL;
    *((size_t*)ptr) = size;
    used_memory += size+sizeof(size_t);
    return (char*)ptr+sizeof(size_t);
}

/* 调整已申请的内存块 */
void *util_realloc(void *ptr, size_t size)
{
    void *realptr;
    size_t oldsize;
    void *newptr;

    if (ptr == NULL) return util_malloc(size);	/* 申请崭新的MEM */
	
    realptr = (char*)ptr-sizeof(size_t);
    oldsize = *((size_t*)realptr);
    newptr = realloc(realptr,size+sizeof(size_t));
    if (!newptr) return NULL;				/* 调整失败则直接返回NULL */

    *((size_t*)newptr) = size;
	/* 更新内存使用量 */
    used_memory -= oldsize;
    used_memory += size;
    return (char*)newptr+sizeof(size_t);	/* 返回 */
}

/* 释放内存 */
void util_free(void *ptr)
{
    void *realptr;
    size_t oldsize;

    if (ptr == NULL) {
        return;
    }
    realptr = (char*)ptr-sizeof(size_t);
    oldsize = *((size_t*)realptr);
    used_memory -= oldsize+sizeof(size_t);
    free(realptr);
}

/* 返回已消耗的内存总量 */
size_t util_malloc_used_memory(void)
{
    return used_memory;
}
