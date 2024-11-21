#include "hash.h"
#include "log.h"

#define ERR_OK 0

struct hash_table_s {
	char name[HASH_NAME_LEN_00];
    hash_key_type_t key_type;
   	size_t mem_key_len;     /* 内存键的长度 */

	size_t node_ttl;    /* 当前已存放的节点数 */
	size_t height;	    /* 桶高 */
    hash_cal_handler hash_cal_fun;  /* 计算hash的函数 */
	hash_node_t *nodes[];
};


struct hash_node_s {
	struct hash_node_s *next;

	void *val;

    /* 各种常见类型的 KEY */
    hash_key_t keys;
    /********************************
     ********************************
     *** MAYBE MEMORY FOR MEM_KEY ***
     ********************************
     ********************************/
};

/* 开启内测测试 */
//#define HASH_MEM_TEST_0X21E7477F 0
static inline void *hash_mem_malloc(size_t size);
static inline void *hash_mem_calloc(size_t size);
static inline void *hash_mem_realloc(void *ptr, size_t size);
static inline void hash_mem_free(void *ptr);
size_t hash_mem_used_memory(void);
static size_t used_memory = 0;


/* 申请内存, 自带size.head
 * mem: size.head + free.mem */
static inline void *hash_mem_malloc(size_t size)
{
#ifndef HASH_MEM_TEST_0X21E7477F
    return malloc(size);
#endif

	/* The  malloc()  function  allocates  size bytes and returns a pointer to the allocated memory.  
	 * The memory is not initialized */
    void *ptr = malloc(sizeof(size_t) + size);

    if (!ptr) {
        return NULL;
    }

    *((size_t*)ptr) = size;
    used_memory += (sizeof(size_t) + size);
    return (char*)ptr + sizeof(size_t);
}

static inline void *hash_mem_calloc(size_t size)
{
#ifndef HASH_MEM_TEST_0X21E7477F
    return calloc(1, size);
#endif

    /* The  malloc()  function  allocates  size bytes and returns a pointer to the allocated memory.  
	 * The memory is not initialized */
    void *ptr = calloc(1, size + sizeof(size_t));

    if (!ptr) {
        return NULL;
    }
    *((size_t*)ptr) = size;
    used_memory += (sizeof(size_t) + size);
    return (char*)ptr + sizeof(size_t);
}

/* 调整已申请的内存块 */
static void *hash_mem_realloc(void *ptr, size_t size)
{
#ifndef HASH_MEM_TEST_0X21E7477F
    return realloc(ptr, size);
#endif

    void *realptr = NULL;
    size_t oldsize = 0;
    void *newptr = NULL;

    if (ptr == NULL) {
        return hash_mem_malloc(size);	/* 申请崭新的MEM */
    }

    realptr = (char*)ptr - sizeof(size_t);
    oldsize = *((size_t*)realptr);
    newptr = realloc(realptr, size + sizeof(size_t));
    if (!newptr) {
        return NULL;				/* 调整失败则直接返回NULL */
    }

    *((size_t*)newptr) = size;
	/* 更新内存使用量 */
    used_memory -= oldsize;
    used_memory += size;
    return  (char*)newptr + sizeof(size_t);	/* 返回 */
}

/* 释放内存 */
static inline void hash_mem_free(void *ptr)
{
#ifndef HASH_MEM_TEST_0X21E7477F
    free(ptr);
	return;
#endif

    void *realptr = NULL;
    size_t oldsize = 0;

    if (ptr == NULL) {
        return;
    }
    realptr = (char*)ptr - sizeof(size_t);
    oldsize = *((size_t*)realptr);
    used_memory -= (oldsize + sizeof(size_t));
    free(realptr);
}

/* 返回已消耗的内存总量 */
extern size_t hash_test_mem_used(void)
{
    return used_memory;
}

extern void *hash_test_mem_malloc(size_t sz)
{
    return hash_mem_malloc(sz);
}

static const uint64_t hash_crc64_tab[256] = {
    UINT64_C(0x0000000000000000), UINT64_C(0x7ad870c830358979),
    UINT64_C(0xf5b0e190606b12f2), UINT64_C(0x8f689158505e9b8b),
    UINT64_C(0xc038e5739841b68f), UINT64_C(0xbae095bba8743ff6),
    UINT64_C(0x358804e3f82aa47d), UINT64_C(0x4f50742bc81f2d04),
    UINT64_C(0xab28ecb46814fe75), UINT64_C(0xd1f09c7c5821770c),
    UINT64_C(0x5e980d24087fec87), UINT64_C(0x24407dec384a65fe),
    UINT64_C(0x6b1009c7f05548fa), UINT64_C(0x11c8790fc060c183),
    UINT64_C(0x9ea0e857903e5a08), UINT64_C(0xe478989fa00bd371),
    UINT64_C(0x7d08ff3b88be6f81), UINT64_C(0x07d08ff3b88be6f8),
    UINT64_C(0x88b81eabe8d57d73), UINT64_C(0xf2606e63d8e0f40a),
    UINT64_C(0xbd301a4810ffd90e), UINT64_C(0xc7e86a8020ca5077),
    UINT64_C(0x4880fbd87094cbfc), UINT64_C(0x32588b1040a14285),
    UINT64_C(0xd620138fe0aa91f4), UINT64_C(0xacf86347d09f188d),
    UINT64_C(0x2390f21f80c18306), UINT64_C(0x594882d7b0f40a7f),
    UINT64_C(0x1618f6fc78eb277b), UINT64_C(0x6cc0863448deae02),
    UINT64_C(0xe3a8176c18803589), UINT64_C(0x997067a428b5bcf0),
    UINT64_C(0xfa11fe77117cdf02), UINT64_C(0x80c98ebf2149567b),
    UINT64_C(0x0fa11fe77117cdf0), UINT64_C(0x75796f2f41224489),
    UINT64_C(0x3a291b04893d698d), UINT64_C(0x40f16bccb908e0f4),
    UINT64_C(0xcf99fa94e9567b7f), UINT64_C(0xb5418a5cd963f206),
    UINT64_C(0x513912c379682177), UINT64_C(0x2be1620b495da80e),
    UINT64_C(0xa489f35319033385), UINT64_C(0xde51839b2936bafc),
    UINT64_C(0x9101f7b0e12997f8), UINT64_C(0xebd98778d11c1e81),
    UINT64_C(0x64b116208142850a), UINT64_C(0x1e6966e8b1770c73),
    UINT64_C(0x8719014c99c2b083), UINT64_C(0xfdc17184a9f739fa),
    UINT64_C(0x72a9e0dcf9a9a271), UINT64_C(0x08719014c99c2b08),
    UINT64_C(0x4721e43f0183060c), UINT64_C(0x3df994f731b68f75),
    UINT64_C(0xb29105af61e814fe), UINT64_C(0xc849756751dd9d87),
    UINT64_C(0x2c31edf8f1d64ef6), UINT64_C(0x56e99d30c1e3c78f),
    UINT64_C(0xd9810c6891bd5c04), UINT64_C(0xa3597ca0a188d57d),
    UINT64_C(0xec09088b6997f879), UINT64_C(0x96d1784359a27100),
    UINT64_C(0x19b9e91b09fcea8b), UINT64_C(0x636199d339c963f2),
    UINT64_C(0xdf7adabd7a6e2d6f), UINT64_C(0xa5a2aa754a5ba416),
    UINT64_C(0x2aca3b2d1a053f9d), UINT64_C(0x50124be52a30b6e4),
    UINT64_C(0x1f423fcee22f9be0), UINT64_C(0x659a4f06d21a1299),
    UINT64_C(0xeaf2de5e82448912), UINT64_C(0x902aae96b271006b),
    UINT64_C(0x74523609127ad31a), UINT64_C(0x0e8a46c1224f5a63),
    UINT64_C(0x81e2d7997211c1e8), UINT64_C(0xfb3aa75142244891),
    UINT64_C(0xb46ad37a8a3b6595), UINT64_C(0xceb2a3b2ba0eecec),
    UINT64_C(0x41da32eaea507767), UINT64_C(0x3b024222da65fe1e),
    UINT64_C(0xa2722586f2d042ee), UINT64_C(0xd8aa554ec2e5cb97),
    UINT64_C(0x57c2c41692bb501c), UINT64_C(0x2d1ab4dea28ed965),
    UINT64_C(0x624ac0f56a91f461), UINT64_C(0x1892b03d5aa47d18),
    UINT64_C(0x97fa21650afae693), UINT64_C(0xed2251ad3acf6fea),
    UINT64_C(0x095ac9329ac4bc9b), UINT64_C(0x7382b9faaaf135e2),
    UINT64_C(0xfcea28a2faafae69), UINT64_C(0x8632586aca9a2710),
    UINT64_C(0xc9622c4102850a14), UINT64_C(0xb3ba5c8932b0836d),
    UINT64_C(0x3cd2cdd162ee18e6), UINT64_C(0x460abd1952db919f),
    UINT64_C(0x256b24ca6b12f26d), UINT64_C(0x5fb354025b277b14),
    UINT64_C(0xd0dbc55a0b79e09f), UINT64_C(0xaa03b5923b4c69e6),
    UINT64_C(0xe553c1b9f35344e2), UINT64_C(0x9f8bb171c366cd9b),
    UINT64_C(0x10e3202993385610), UINT64_C(0x6a3b50e1a30ddf69),
    UINT64_C(0x8e43c87e03060c18), UINT64_C(0xf49bb8b633338561),
    UINT64_C(0x7bf329ee636d1eea), UINT64_C(0x012b592653589793),
    UINT64_C(0x4e7b2d0d9b47ba97), UINT64_C(0x34a35dc5ab7233ee),
    UINT64_C(0xbbcbcc9dfb2ca865), UINT64_C(0xc113bc55cb19211c),
    UINT64_C(0x5863dbf1e3ac9dec), UINT64_C(0x22bbab39d3991495),
    UINT64_C(0xadd33a6183c78f1e), UINT64_C(0xd70b4aa9b3f20667),
    UINT64_C(0x985b3e827bed2b63), UINT64_C(0xe2834e4a4bd8a21a),
    UINT64_C(0x6debdf121b863991), UINT64_C(0x1733afda2bb3b0e8),
    UINT64_C(0xf34b37458bb86399), UINT64_C(0x8993478dbb8deae0),
    UINT64_C(0x06fbd6d5ebd3716b), UINT64_C(0x7c23a61ddbe6f812),
    UINT64_C(0x3373d23613f9d516), UINT64_C(0x49aba2fe23cc5c6f),
    UINT64_C(0xc6c333a67392c7e4), UINT64_C(0xbc1b436e43a74e9d),
    UINT64_C(0x95ac9329ac4bc9b5), UINT64_C(0xef74e3e19c7e40cc),
    UINT64_C(0x601c72b9cc20db47), UINT64_C(0x1ac40271fc15523e),
    UINT64_C(0x5594765a340a7f3a), UINT64_C(0x2f4c0692043ff643),
    UINT64_C(0xa02497ca54616dc8), UINT64_C(0xdafce7026454e4b1),
    UINT64_C(0x3e847f9dc45f37c0), UINT64_C(0x445c0f55f46abeb9),
    UINT64_C(0xcb349e0da4342532), UINT64_C(0xb1eceec59401ac4b),
    UINT64_C(0xfebc9aee5c1e814f), UINT64_C(0x8464ea266c2b0836),
    UINT64_C(0x0b0c7b7e3c7593bd), UINT64_C(0x71d40bb60c401ac4),
    UINT64_C(0xe8a46c1224f5a634), UINT64_C(0x927c1cda14c02f4d),
    UINT64_C(0x1d148d82449eb4c6), UINT64_C(0x67ccfd4a74ab3dbf),
    UINT64_C(0x289c8961bcb410bb), UINT64_C(0x5244f9a98c8199c2),
    UINT64_C(0xdd2c68f1dcdf0249), UINT64_C(0xa7f41839ecea8b30),
    UINT64_C(0x438c80a64ce15841), UINT64_C(0x3954f06e7cd4d138),
    UINT64_C(0xb63c61362c8a4ab3), UINT64_C(0xcce411fe1cbfc3ca),
    UINT64_C(0x83b465d5d4a0eece), UINT64_C(0xf96c151de49567b7),
    UINT64_C(0x76048445b4cbfc3c), UINT64_C(0x0cdcf48d84fe7545),
    UINT64_C(0x6fbd6d5ebd3716b7), UINT64_C(0x15651d968d029fce),
    UINT64_C(0x9a0d8ccedd5c0445), UINT64_C(0xe0d5fc06ed698d3c),
    UINT64_C(0xaf85882d2576a038), UINT64_C(0xd55df8e515432941),
    UINT64_C(0x5a3569bd451db2ca), UINT64_C(0x20ed197575283bb3),
    UINT64_C(0xc49581ead523e8c2), UINT64_C(0xbe4df122e51661bb),
    UINT64_C(0x3125607ab548fa30), UINT64_C(0x4bfd10b2857d7349),
    UINT64_C(0x04ad64994d625e4d), UINT64_C(0x7e7514517d57d734),
    UINT64_C(0xf11d85092d094cbf), UINT64_C(0x8bc5f5c11d3cc5c6),
    UINT64_C(0x12b5926535897936), UINT64_C(0x686de2ad05bcf04f),
    UINT64_C(0xe70573f555e26bc4), UINT64_C(0x9ddd033d65d7e2bd),
    UINT64_C(0xd28d7716adc8cfb9), UINT64_C(0xa85507de9dfd46c0),
    UINT64_C(0x273d9686cda3dd4b), UINT64_C(0x5de5e64efd965432),
    UINT64_C(0xb99d7ed15d9d8743), UINT64_C(0xc3450e196da80e3a),
    UINT64_C(0x4c2d9f413df695b1), UINT64_C(0x36f5ef890dc31cc8),
    UINT64_C(0x79a59ba2c5dc31cc), UINT64_C(0x037deb6af5e9b8b5),
    UINT64_C(0x8c157a32a5b7233e), UINT64_C(0xf6cd0afa9582aa47),
    UINT64_C(0x4ad64994d625e4da), UINT64_C(0x300e395ce6106da3),
    UINT64_C(0xbf66a804b64ef628), UINT64_C(0xc5bed8cc867b7f51),
    UINT64_C(0x8aeeace74e645255), UINT64_C(0xf036dc2f7e51db2c),
    UINT64_C(0x7f5e4d772e0f40a7), UINT64_C(0x05863dbf1e3ac9de),
    UINT64_C(0xe1fea520be311aaf), UINT64_C(0x9b26d5e88e0493d6),
    UINT64_C(0x144e44b0de5a085d), UINT64_C(0x6e963478ee6f8124),
    UINT64_C(0x21c640532670ac20), UINT64_C(0x5b1e309b16452559),
    UINT64_C(0xd476a1c3461bbed2), UINT64_C(0xaeaed10b762e37ab),
    UINT64_C(0x37deb6af5e9b8b5b), UINT64_C(0x4d06c6676eae0222),
    UINT64_C(0xc26e573f3ef099a9), UINT64_C(0xb8b627f70ec510d0),
    UINT64_C(0xf7e653dcc6da3dd4), UINT64_C(0x8d3e2314f6efb4ad),
    UINT64_C(0x0256b24ca6b12f26), UINT64_C(0x788ec2849684a65f),
    UINT64_C(0x9cf65a1b368f752e), UINT64_C(0xe62e2ad306bafc57),
    UINT64_C(0x6946bb8b56e467dc), UINT64_C(0x139ecb4366d1eea5),
    UINT64_C(0x5ccebf68aecec3a1), UINT64_C(0x2616cfa09efb4ad8),
    UINT64_C(0xa97e5ef8cea5d153), UINT64_C(0xd3a62e30fe90582a),
    UINT64_C(0xb0c7b7e3c7593bd8), UINT64_C(0xca1fc72bf76cb2a1),
    UINT64_C(0x45775673a732292a), UINT64_C(0x3faf26bb9707a053),
    UINT64_C(0x70ff52905f188d57), UINT64_C(0x0a2722586f2d042e),
    UINT64_C(0x854fb3003f739fa5), UINT64_C(0xff97c3c80f4616dc),
    UINT64_C(0x1bef5b57af4dc5ad), UINT64_C(0x61372b9f9f784cd4),
    UINT64_C(0xee5fbac7cf26d75f), UINT64_C(0x9487ca0fff135e26),
    UINT64_C(0xdbd7be24370c7322), UINT64_C(0xa10fceec0739fa5b),
    UINT64_C(0x2e675fb4576761d0), UINT64_C(0x54bf2f7c6752e8a9),
    UINT64_C(0xcdcf48d84fe75459), UINT64_C(0xb71738107fd2dd20),
    UINT64_C(0x387fa9482f8c46ab), UINT64_C(0x42a7d9801fb9cfd2),
    UINT64_C(0x0df7adabd7a6e2d6), UINT64_C(0x772fdd63e7936baf),
    UINT64_C(0xf8474c3bb7cdf024), UINT64_C(0x829f3cf387f8795d),
    UINT64_C(0x66e7a46c27f3aa2c), UINT64_C(0x1c3fd4a417c62355),
    UINT64_C(0x935745fc4798b8de), UINT64_C(0xe98f353477ad31a7),
    UINT64_C(0xa6df411fbfb21ca3), UINT64_C(0xdc0731d78f8795da),
    UINT64_C(0x536fa08fdfd90e51), UINT64_C(0x29b7d047efec8728),
};

uint64_t hash_crc64(uint64_t crc, const unsigned char *s, uint64_t l)
{
    uint64_t j = 0;

    for (j = 0; j < l; j++) {
        uint8_t byte = s[j];
        crc = hash_crc64_tab[(uint8_t)crc ^ byte] ^ (crc >> 8);
    }
    return crc;
}

/* Test main */
#ifdef TEST_MAIN
#include <stdio.h>
int main(void) {
    LOG_ERROR("e9c6d914c4b8d9ca == %016llx\n",
        (unsigned long long) crc64(0,(unsigned char*)"123456789",9));
    return 0;
}
#endif


/* 缺省的 hash 计算函数 */
static size_t hash_cal(void *buf, size_t len)
{
    return ((size_t)hash_crc64(UINT64_C(0X3C66C56408F84898), (const unsigned char *)buf, (uint64_t)len));
}

// RETURNS: 0 mean equal, others: not equal
static inline int hash_key_compare(hash_key_type_t key_type, size_t key_len, hash_key_t key_a, hash_key_t key_b)
{
    size_t i = 0;

    switch (key_type) {
    case hash_key_char:
        return (key_a.c == key_b.c ) ? 0 : 1;
    case hash_key_uchar:
        return (key_a.uc == key_b.uc ) ? 0 : 1;
    case hash_key_int16:
        return (key_a.i16 == key_b.i16) ? 0 : 1;
    case hash_key_uint16:
        return (key_a.u16 == key_b.u16) ? 0 : 1;
    case hash_key_int32:
        return (key_a.i32 == key_b.i32) ? 0 : 1;
    case hash_key_uint32:
        return (key_a.u32 == key_b.u32) ? 0 : 1;
    case hash_key_int64:
        return (key_a.i64 == key_b.i64) ? 0 : 1;
    case hash_key_uint64:
        return (key_a.u64 == key_b.u64) ? 0 : 1;
    case hash_key_pointer:
        return (key_a.ptr == key_b.ptr) ? 0 : 1;
    case hash_key_mac:
        for (i = 0; i < sizeof(key_a.mac.bytes); ++i) {
            if (key_a.mac.bytes[i] != key_b.mac.bytes[i]) {
                return 1;
            }
        }
        return 0;
    case hash_key_mac_str:
        for (i = 0; i < sizeof(key_a.mac_str); ++i) {
            if (key_a.mac_str[i] != key_b.mac_str[i]) {
                return 1;
            }
        }
        return 0;
    case hash_key_ip_str:
        for (i = 0; i < sizeof(key_a.ip_str); ++i) {
            if (key_a.ip_str[i] != key_b.ip_str[i]) {
                return 1;
            }
        }
        return 0;
    case hash_key_id_char32_str:
        return memcmp(key_a.id_char32_str, key_b.id_char32_str, sizeof(key_a.id_char32_str));
    case hash_key_id_char64_str:
        return memcmp(key_a.id_char64_str, key_b.id_char64_str, sizeof(key_a.id_char64_str));
    case hash_key_mem:
        return memcmp(key_a.mem, key_b.mem, key_len);
    default:
        LOG_ERROR("0x70e05102 key.type invalid %d", key_type);
        exit(-0x70e05102);
        break;
    }
}

// RETURNS: 0 mean equal, others: not equal
static inline void hash_key_copy(hash_key_type_t key_type, size_t key_len, hash_key_t *key_a, hash_key_t key_b)
{
    size_t i = 0;

    switch (key_type) {
    case hash_key_char:
        key_a->c = key_b.c;
        break;
    case hash_key_uchar:
        key_a->uc = key_b.uc;
        break;
    case hash_key_int16:
        key_a->i16 = key_b.i16;
        break;
    case hash_key_uint16:
        key_a->u16 = key_b.u16;
        break;
    case hash_key_int32:
        key_a->i32 = key_b.i32;
        break;
    case hash_key_uint32:
        key_a->u32 = key_b.u32;
        break;
    case hash_key_int64:
        key_a->i64 = key_b.i64;
        break;
    case hash_key_uint64:
        key_a->u64 = key_b.u64;
        break;
    case hash_key_pointer:
        key_a->ptr = key_b.ptr;
        break;
    case hash_key_mac:
        for (i = 0; i < sizeof(key_a->mac.bytes); ++i) {
            key_a->mac.bytes[i] = key_b.mac.bytes[i];
        }
        break;
    case hash_key_mac_str:
        for (i = 0; i < sizeof(key_a->mac_str); ++i) {
            key_a->mac_str[i] = key_b.mac_str[i];
        }
        break;
    case hash_key_ip_str:
        for (i = 0; i < sizeof(key_a->ip_str); ++i) {
            key_a->ip_str[i] = key_b.ip_str[i];
        }
        break;
    case hash_key_id_char32_str:
        memcpy(key_a->id_char32_str, key_b.id_char32_str, sizeof(key_a->id_char32_str));
		break;
    case hash_key_id_char64_str:
        memcpy(key_a->id_char64_str, key_b.id_char64_str, sizeof(key_a->id_char64_str));
		break;
    case hash_key_mem:
        memcpy(key_a->mem, key_b.mem, key_len);
        break;
    default:
        LOG_ERROR("0x70e05102 key.type invalid %d", key_type);
        exit(-0x70e05102);
        break;
    }
}

static inline size_t cal_key_len(hash_table_t *tbl)
{
    switch (tbl->key_type) {
    case hash_key_char:
    case hash_key_uchar:
        return sizeof(char);
    case hash_key_int16:
    case hash_key_uint16:
        return sizeof(uint16_t);
    case hash_key_int32:
    case hash_key_uint32:
        return sizeof(uint32_t);
    case hash_key_int64:
    case hash_key_uint64:
        return sizeof(uint64_t);
    case hash_key_pointer:
        return sizeof(void *);
    case hash_key_mac:
        return sizeof(mac_bytes_t);
    case hash_key_mac_str:
        return MAC_LEN_STR_00;
    case hash_key_ip_str:
        return IPV4_LEN_STR_00;
    case hash_key_id_char32_str:
        return ID_CHAR_32_STR_00;
    case hash_key_id_char64_str:
        return ID_CHAR_64_STR_00;
    case hash_key_mem:
        return tbl->mem_key_len;
    default:
        LOG_ERROR("0x70e05102 table[%s], key.type invalid %d", tbl->name, tbl->key_type);
        exit(-0x70e05102);
        break;
    }
}

static inline void *cal_key_addr(hash_key_type_t key_type, hash_key_t *key)
{
    switch (key_type) {
    case hash_key_char:
        return &key->c;
    case hash_key_uchar:
        return &key->uc;
    case hash_key_int16:
        return &key->i16;
    case hash_key_uint16:
        return &key->u16;
    case hash_key_int32:
        return &key->i32;
   case hash_key_uint32:
        return &key->u32;
    case hash_key_int64:
        return &key->i64;
    case hash_key_uint64:
        return &key->u64;
    case hash_key_pointer:
        return &key->ptr;
    case hash_key_mac:
        return &key->mac;
    case hash_key_mac_str:
        return key->mac_str;
    case hash_key_ip_str:
        return key->ip_str;
    case hash_key_id_char32_str:
        return key->id_char32_str;
    case hash_key_id_char64_str:
        return key->id_char64_str;
    case hash_key_mem:
        return key->mem;    /* 直接返回指向的地址 */
    default:
        LOG_ERROR("0x11c9cb57 key.type invalid %d", key_type);
        exit(-0x11c9cb57);
        break;
    }
}


static inline void hash_free_node(hash_node_t *node)
{
    hash_mem_free(node->val);
	node->val = NULL;
	node->next = NULL;
    hash_mem_free(node);
    node = NULL;
}

static hash_table_t *hash_create_do(size_t height, hash_key_type_t key_type, size_t mem_key_len)
{
	if (height == 0) {
		LOG_ERROR("0x2f670a85 args valid(%ld:%d:%ld) for hash_create!", height, (int)key_type, mem_key_len);
		return NULL;
	}
	hash_table_t *tbl = hash_mem_calloc(sizeof(struct hash_table_s) + sizeof(struct hash_node_s*) * height);
	if (NULL == tbl) {
		LOG_ERROR("0x3f98e3c9 calloc mem fail for hash_create!");
		return NULL;
	}

	tbl->name[0] = 0;

    if (key_type == hash_key_mem) {
        tbl->mem_key_len = mem_key_len;
    } else {
        tbl->mem_key_len = 0;
    }

    tbl->key_type = key_type;
	tbl->node_ttl = 0;
	tbl->height = height;
    tbl->hash_cal_fun = hash_cal;    /* 设置缺省的 hash 计算函数 */

	/* calloc 申请的内存，所以 nodes 无需再初始化 */

	return tbl;
}

hash_table_t *hash_create(int height, hash_key_type_t key_type, size_t key_void_len)
{
    return hash_create_do(height, key_type, key_void_len);
}

void hash_free(hash_table_t *tbl)
{
	hash_reset(tbl);
	/* 数组和表一起释放 */
	hash_mem_free(tbl);
}

/* 释放表内所有元素，保留表结构 */
int hash_reset(hash_table_t *tbl)
{
	if (NULL == tbl) {
		return 0;
	}

	size_t h = 0;

	for (h = 0; h < tbl->height; ++h) {
		hash_node_t *node = tbl->nodes[h];
		while (node) {
			hash_node_t *node_next = node->next;
			hash_free_node(node);
			node = node_next;
			--tbl->node_ttl;
		}
		tbl->nodes[h] = NULL;
	}
	if (tbl->node_ttl != 0) {
		LOG_ERROR("0x58a6823d free table[%s] error, ttl: %ld", tbl->name, tbl->node_ttl);
		return -0x58a6823d;
	}
	/* 保留表结构 */
	/* hash_mem_free(tbl); */
	return 0;
}

int hash_insert(hash_table_t *tbl, hash_key_t key, void *val)
{
    size_t key_len = cal_key_len(tbl);
	size_t h = tbl->hash_cal_fun(cal_key_addr(tbl->key_type, &key), key_len) % tbl->height;
    hash_node_t *node = tbl->nodes[h];

    /* 新值替换旧值 */
    while (node) {
        if (0 == hash_key_compare(tbl->key_type, key_len, node->keys, key)) {
			hash_mem_free(node->val);
            node->val = val;
            return ERR_OK;
        }
        node = node->next;
    }

	/* 插入全新值 */
	node = (hash_node_t *)hash_mem_malloc(sizeof(hash_node_t) + tbl->mem_key_len);
	if (NULL == node) {
		LOG_ERROR("0x6488e857 calloc fail for hash[%s] insert!, node.ttl: %ld", tbl->name, tbl->node_ttl);
		return -0x6488e857;
	}
    if (0 == tbl->mem_key_len) {
        node->keys.mem = NULL;
    } else {
        node->keys.mem = node + 1;
    }
	hash_key_copy(tbl->key_type, key_len, &node->keys, key);
	node->val = val;    /* 值：浅赋值 */

	/* 加到链表 */
	node->next = tbl->nodes[h];
	tbl->nodes[h] = node;
	++tbl->node_ttl;

	return ERR_OK;
}

hash_node_t *hash_find(hash_table_t *tbl, hash_key_t key)
{
    size_t key_len = cal_key_len(tbl);
	size_t h = tbl->hash_cal_fun(cal_key_addr(tbl->key_type, &key), key_len) % tbl->height;
    hash_node_t *node = tbl->nodes[h];

    while (node) {
        if (0 == hash_key_compare(tbl->key_type, key_len, node->keys, key)) {
            return node;
        }
        node = node->next;
    }

	return NULL;
}

int hash_delete(hash_table_t *tbl, hash_key_t key)
{
    size_t key_len = cal_key_len(tbl);
	size_t h = tbl->hash_cal_fun(cal_key_addr(tbl->key_type, &key), key_len) % tbl->height;
	hash_node_t *(*pre) = &tbl->nodes[h];
	hash_node_t *node = tbl->nodes[h];

	while (node) {
	    if (0 == hash_key_compare(tbl->key_type, key_len, node->keys, key)) {
			*pre = node->next;
			hash_free_node(node);
			--tbl->node_ttl;
			return ERR_OK;
	    } else {
	    	pre = &node->next;
			node = node->next;
	    }
	}

	LOG_WARN("0x2f62c919 hash[%s] delete fail, can't find node!\n", tbl->name);
	return ERR_OK;
}

void *hash_update(hash_table_t *tbl, hash_key_t key, void *val)
{
    hash_node_t *node = hash_find(tbl, key);
    if (NULL == node) {
        return NULL;
    }

    void *old = node->val;
    node->val = val;

    return old; /* 返回旧值，让调用者自己 free */
}

static hash_node_t *hash_next_node(hash_table_t *tbl, hash_key_t key)
{
    size_t key_len = cal_key_len(tbl);
	size_t h = tbl->hash_cal_fun(cal_key_addr(tbl->key_type, &key), key_len) % tbl->height;
    hash_node_t *node = tbl->nodes[h];
    while (node) {
        if (0 == hash_key_compare(tbl->key_type, key_len, node->keys, key)) {
            if (node->next) {
				return node->next;	/* 直接返回下一个 item */
            } else {
				break;	/* 本链表的最后一个 item    */
            }
        }
        node = node->next;
    }
	if (node == NULL) {	/* 传入的 key 非法 */
		LOG_ERROR("0x1b8539ae hash[%s], key invalid\n", tbl->name);
		return NULL;
	}

	/* 从下一链开始 */
	for (size_t i = h + 1; i < tbl->height; ++i) {
	    node = tbl->nodes[i];
		if (node) {
			return node;
		}
	}
	return NULL;
}

/* 下一个 item
 * key 为空则 从头开始
 * 如果要完成的遍历一遍 table, 则遍历的过程中在外层需要锁住表
*/
hash_node_t *hash_next(hash_table_t *tbl, hash_key_t *key)
{
	if (NULL != key) {
		return hash_next_node(tbl, *key);
	}

	for (size_t i = 0; i < tbl->height; ++i) {
	    hash_node_t *node = tbl->nodes[i];
		if (node) {
			return node;
		}
	}
	LOG_ERROR("0x1f9b7a22 table[%s] is nil", tbl->name);
	return NULL;
}

void *hash_key_addr(hash_node_t *node)
{
    return &node->keys;
}

void *hash_node_val(hash_node_t *node)
{
    return node->val;
}

size_t hash_node_ttl(hash_table_t *tbl)
{
	return tbl->node_ttl;
}

/* 设置 tbl 的hash计算函数 */
void hash_set_cal_hash_handler(hash_table_t *tbl, hash_cal_handler handler)
{
    tbl->hash_cal_fun = handler;
}

void hash_set_name(hash_table_t *tbl, const char *name)
{
	snprintf(tbl->name, sizeof(tbl->name), "%s", name);
}

