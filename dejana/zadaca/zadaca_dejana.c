#include <stdio.h> 
#include <malloc.h> 
int main(void) 
{ 
    int* p = malloc(8); 
    p = 100; 
    free(p); 
    *p = 110; 
    return 0; 
} 