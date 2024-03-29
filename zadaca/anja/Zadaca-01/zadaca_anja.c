#include <stdio.h>

int main(int argc, char* argv[])
{
	int x = 10;    // STACK segment
	int* ptr = &x; // STACK segment

	ptr++;
	printf("%x   %d \n",  ptr,  *ptr);  
} 

