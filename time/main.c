
#include <stdio.h>
#include <stdlib.h>


int pp(char ***p, int *ttl)
{
    int max = 10;
    *p = (char **)calloc(max, sizeof(char *));
    for (int i =0; i < max; ++i) {
        if (i == 0) {
            (*p)[i] = "hello";
        } else if (i == 1) {
            (*p)[i] = "world";
        }
    }
    *ttl = 2;
    return 0;
}

// https://wangdoc.com/clang/lib/time.h#localtimegmtime
int main(char *argv[], int argc) {
    {
        char **p;
        int ttl;
        pp(&p, &ttl);
        for (int i = 0; i < ttl; ++i) {
            printf("--- %s\n", p[i]);
        }
        free(p);
        p = NULL;
    }


  char s[128];
  //time_t now = time(NULL);
  time_t now = 0;

  // %c: 本地时间
  strftime(s, sizeof s, "%c", localtime(&now));
  puts(s);   // Sun Feb 28 22:29:00 2021

  // %A: 完整的星期日期的名称
  // %B: 完整的月份名称
  // %d: 月份的天数
  strftime(s, sizeof s, "%A, %B %d", localtime(&now));
  puts(s);   // Sunday, February 28

  // %I: 小时（12小时制）
  // %M: 分钟
  // %S: 秒数
  // %p: AM 或 PM
  strftime(s, sizeof s, "It's  %p", localtime(&now));
  puts(s);   // It's 10:29:00 PM

 printf("hello world\n");
  // %F: ISO 8601 yyyy-mm-dd 格式
  // %T: ISO 8601 hh:mm:ss 格式
  // %z: ISO 8601 时区
  //strftime(s, sizeof s, "%d%I:%M:%S", localtime(&now));
  strftime(s, sizeof s, "%Y%m%d %X", localtime(&now));
  puts(s);   // ISO 8601: 2021-02-28T22:29:00-0800
}
