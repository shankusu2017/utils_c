#ifndef LOG_H_20240910203709_0X3F3C46DC
#define LOG_H_20240910203709_0X3F3C46DC

#ifdef __cplusplus
extern "C" {
#endif

extern void uc_log(char *format, ...);

#define log(format, ...) uc_log("%-7s %-10s %-3d "format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_ERROR(format, ...) uc_log("%-7s %-10s %-3d "format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_WARN(format, ...) uc_log("%-7s %-10s %-3d "format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_NOTICE(format, ...) uc_log("%-7s %-10s %-3d "format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_DEBUG(format, ...) uc_log("%-7s %-10s %-3d "format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);

#ifdef __cplusplus
}
#endif

#endif /* LOG_H_20240910203709_0X3F3C46DC */
