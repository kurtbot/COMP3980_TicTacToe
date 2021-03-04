#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
    char*a = "hello";
    char*b = "1 2 3";
    int value = atoi(a);
    int value2 = atoi(b);
    printf("%d", value);
    printf("%d", value2);
    value2 = atoi(b);
    printf("%d", value2);
    return 0;
}
