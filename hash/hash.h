#ifndef HASH_H_0X3D11AEFC
#define HASH_H_0X3D11AEFC

/*
 * 链式 哈希表，如果 node_ttl / bucket_height 过大，则查询效率低，
 */

#ifdef __cplusplus 
extern "C" {
#endif

#include <stdint.h>

typedef struct hash_node_s {
	struct hash_node_s *next;	
	void *val;
	char key[];
} hash_node_t;

typedef struct hash_table_s {
	size_t key_len;

	size_t node_ttl;		/* 当前存的节点数 */	
	size_t bucket_height;	/* 哈希桶高 */
	hash_node_t *nodes[];
} hash_table_t;

extern hash_table_t *hash_create(int bucket_height, size_t key_len);
extern void hash_free(hash_table_t *tbl);

/* 单独复制一份 key */
extern int hash_insert(hash_table_t *tbl, void *key, void *val);
extern hash_node_t *hash_find(hash_table_t *tbl, void *key);
extern int hash_delete(hash_table_t *tbl, void *key);
extern hash_node_t *hash_next(hash_table_t *tbl, void *key);
extern size_t hash_node_ttl(hash_table_t *tbl);

#ifdef __cplusplus
}
#endif

#endif /* HASH_H_0X3D11AEFC */
