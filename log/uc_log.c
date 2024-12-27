#include "uc_common.h"
#include "uc_time.h"

void uc_log(char *format, ...)
{
    char buf[2048];
    char out[2048+1024];
	va_list args;

	va_start(args, format);

    vsprintf(buf, format, args);

	va_end(args);

    snprintf(out, sizeof(out), "ms: %ld, %s\n", uc_time_ms(), buf);	/* 加上标识符和换行符 */
    printf("%s\n", out);
    //write(fd, out);	/* 输出到指定 fd */
}
