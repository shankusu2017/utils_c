#include <stdio.h>

int call_fun(int array [][128])
{
    printf("sizeof(array[i]): %d\n", sizeof(array[0]));
}

int main(int argc, char *argv[])
{
    printf("hello world\n");
    int array[64][128];
    call_fun(array);
    return 0;
}
