#ifndef LOG_H_20240910203709_0X3F3C46DC
#define LOG_H_20240910203709_0X3F3C46DC

#ifdef __cplusplus
extern "C" {
#endif

extern void dlog(char *format, ...);

#define log(format, ...) dlog("%-7s %-10s %-3d "format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_ERROR(format, ...) dlog("%-7s %-10s %-3d "format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_WARN(format, ...) dlog("%-7s %-10s %-3d "format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_NOTICE(format, ...) dlog("%-7s %-10s %-3d "format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_DEBUG(format, ...) dlog("%-7s %-10s %-3d "format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);

#ifdef __cplusplus
}
#endif

#endif /* LOG_H_20240910203709_0X3F3C46DC */
