#ifndef MEMORY_H_20241028201000_0X30DD7FC5
#define MEMORY_H_20241028201000_0X30DD7FC5

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

void *util_malloc(size_t size);
void *util_calloc(size_t size);
void *util_realloc(void *ptr, size_t size);
void util_free(void *ptr);
size_t util_malloc_used_memory(void);

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_H_20241028201000_0X30DD7FC5 */
