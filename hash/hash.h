#ifndef HASH_H_0X3D11AEFC
#define HASH_H_0X3D11AEFC

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>


typedef struct hash_node_s {
	struct hash_node_s *next;
	void *val;
	char key[0];
} hash_node_t;

typedef struct hash_table_s {
	pthread_mutex_t mutex;
	size_t key_len;

	size_t node_count;		/* 当前存的元素个数 */	
	size_t tbl_len;
	hash_node_t *nodes[0];
} hash_table_t;

extern hash_table_t *hash_create(int tbl_len, size_t key_len);
extern void hash_free(hash_table_t *tbl);

/* 单独复制一份 key */
extern hash_node_t *hash_insert(hash_table_t *tbl, void *key, void *val);
extern hash_node_t *hash_find(hash_table_t *tbl, void *key);
extern int hash_delete(hash_table_t *tbl, void *key);
size_t hash_node_count(hash_table_t *tbl);

#ifdef __cplusplus
}
#endif

#endif /* HASH_H_0X3D11AEFC */
