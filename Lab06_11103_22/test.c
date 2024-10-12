#include<stdio.h>
#include<stdlib.h>

int main() {
	FILE *fptr;
	fptr = fopen("/dev/stat_proc", "rw");
	fprintf(fptr, "LUKA");
	fclose(fptr);
	return 1;
}
