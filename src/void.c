#include <stdio.h>
#include <stdlib.h>

int main()
{
    void* x;
    int* y;

    x = malloc(10);
    y = x;
    printf("0x%x\n", x);
    printf("0x%x\n", y);
    printf("0x%x\n", ++x);
    printf("0x%x\n", ++y);
    printf("%.2f\n", 3233.4234233);
    return 0;
}
