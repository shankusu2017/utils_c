#ifndef HASH_H_2024_07_19_0X3D11AEFC
#define HASH_H_2024_07_19_0X3D11AEFC

/*
 * 链式 哈希表，如果 node_ttl / bucket_height 过大，则查询效率低，
 */

#include "uc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UC_HASH_NAME_LEN_00
#define UC_HASH_NAME_LEN_00 (257)
#endif

typedef struct uc_mac_bytes_s {
    unsigned char bytes[UC_MAC_LEN_BYTES];
} __attribute__((packed)) uc_mac_bytes_t;

typedef enum uc_hash_key_type_e {
    // hash_key_void = 0,          /* 自定义长度的  void * 类型 */
    uc_hash_key_char = 100,
    uc_hash_key_uchar,
    uc_hash_key_int16,
    uc_hash_key_uint16,
    uc_hash_key_int32,
    uc_hash_key_uint32,
    uc_hash_key_int64,
    uc_hash_key_uint64,
    uc_hash_key_pointer,   /* 指针 */
    uc_hash_key_mac,       /* 6 Byte 的 MAC */
    uc_hash_key_mac_str,   /* "D8-F2-CA-56-6F-7B" */
    uc_hash_key_ip_str,    /* "192.168.123.213" */
    uc_hash_key_id_char32_str, /* 86c14d9ebf144b84b06abf0f73cc82a6 */
    uc_hash_key_id_char64_str, /* 3dc5a14de0befc15f58947f8fcad708a19d6b4600cc681d69e4b57fce7e9e45f */
    uc_hash_key_mem = 200,       /* 一片内存 */
} uc_hash_key_type_t;

typedef union uc_hash_key_u {
    char c;
    unsigned char uc;
    int16_t i16;
    uint16_t u16;
    int32_t i32;
    uint32_t u32;
    int64_t i64;
    uint64_t u64;
    uc_mac_bytes_t mac;
    char mac_str[UC_MAC_LEN_STR_00];
    char ip_str[UC_IPV4_LEN_STR_00];
    char id_char32_str[UC_ID_CHAR_32_STR_00];
    char id_char64_str[UC_ID_CHAR_64_STR_00];
    void *ptr;
    void *mem;  /* 通用类型 key */
} uc_hash_key_t;


typedef struct uc_hash_table_s uc_hash_table_t;
typedef struct uc_hash_node_s uc_hash_node_t;
typedef size_t (*uc_hash_cal_handler)(void *addr, size_t len);

extern uc_hash_table_t *uc_hash_create(int bucket_height, uc_hash_key_type_t key_type, size_t key_mem_len);
/* 删除整个表，包含表自身 */
extern void uc_hash_free(uc_hash_table_t *tbl);
/* 释放表内所有元素，保留表结构 */
extern int uc_hash_reset(uc_hash_table_t *tbl);

/* void *的 key 会单独复制一份，其它类型的 key 进行值拷贝
 * val 会被值拷贝，释放 tbl 时会被一起释放
 */
extern int uc_hash_insert(uc_hash_table_t *tbl, uc_hash_key_t key, void *val);

/* 参考代码
 *   uc_hash_node_t *item = uc_hash_next(tbl, NULL);
 *   uc_hash_node_t *next = NULL;
 *   while (item) {
 *       next = uc_hash_next(tbl, uc_hash_key_addr(item));
 *       uc_hash_delete(tbl, uc_hash_key_addr(item));
 *       item = next;
 *   }
*/
extern uc_hash_node_t *uc_hash_find(uc_hash_table_t *tbl, uc_hash_key_t key);
extern int uc_hash_delete(uc_hash_table_t *tbl, uc_hash_key_t key);

/*
 * key 为空则从 head 开始找第一个item
 * 否则找到 key 后的第一个 item
 */
extern uc_hash_node_t *uc_hash_next(uc_hash_table_t *tbl, uc_hash_key_t *key);

/* 更新 key 对应的值为 val
 * RETURNS: 若 key 对应的 node 不存在则直接返回 val
 * 若 key 对应的 node 存在则返回 node->val 上的旧值
 */
extern void *uc_hash_update(uc_hash_table_t *tbl, uc_hash_key_t key, void *val);

extern void *uc_hash_key_addr(uc_hash_node_t *node);

extern void *uc_hash_node_val(uc_hash_node_t *node);

/* tbl 已存放元素个数 */
extern size_t uc_hash_node_ttl(uc_hash_table_t *tbl);
/* 设置独立的 tbl hash 计算函数 */
extern void uc_hash_set_cal_hash_handler(uc_hash_table_t *tbl, uc_hash_cal_handler handler);

extern void uc_hash_set_name(uc_hash_table_t *tbl, const char *name);

extern uc_hash_node_t *uc_hash_random_node(uc_hash_table_t *tbl, size_t rdm_idx);

extern void uc_hash_test_print_table(uc_hash_table_t *tbl, void fun(void *node, char *node_str, size_t node_str_len));
extern size_t uc_hash_test_mem_used(void);
extern void *uc_hash_test_mem_malloc(size_t sz);



#ifdef __cplusplus
}
#endif

#endif /* HASH_H_2024_07_19_0X3D11AEFC */
