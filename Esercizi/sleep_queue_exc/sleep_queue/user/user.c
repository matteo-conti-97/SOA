#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

//#define CODE 134 //this depends on what the kernel tells you when mounting the printk-example module
//#define CODE 156 //this depends on what the kernel tells you when mounting the printk-example module


int main(int argc, char** argv){
	if(argc < 2){
		printf("Usage: %s <syscall number> \n", argv[0]);
		return 0;
	}
	int code = atoi(argv[1]);
	syscall(code);	
	return 0;
}

