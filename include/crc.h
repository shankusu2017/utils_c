#ifndef CRC_H_20241028202300_0X5963BD67
#define CRC_H_20241028202300_0X5963BD67

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint64_t util_crc64(uint64_t crc, const unsigned char *s, uint64_t l);

#ifdef __cplusplus
}
#endif

#endif /* CRC_H_20241028202300_0X5963BD67 */