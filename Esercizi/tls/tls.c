/*
Alternative Thread Local Storage (TLS)
This homework deals with the implementation of a TLS support standing aside of the original one offered by gcc. It should be based on a few macros with the following meaning:

PER_THREAD_MEMORY_START and PER_THREAD_MEMORY_END are used to delimitate the variable declarations that will be part of TLS
READ_THREAD_VARIABLE(name) is used to return the value of a TLS variable with a given name
WRITE_THREAD_VARIABLE(name, value) is used to update the TLS variable with a given name to a given value
Essentially, the homework is tailored to implementing an interface for managing per-thread memory resembling 
the one offered by the Linux kernel for managing per-CPU memory.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <asm/prctl.h>
#include <sys/prctl.h>
#include <sys/mman.h>

#include "tls.h"

PER_THREAD_MEMORY_START
    int a;
PER_THREAD_MEMORY_END


#define WORKERS 2

void *thread(void *tn) {
    TLS_ZONE *tls_addr=(TLS_ZONE *)mmap(NULL, sizeof(TLS_ZONE), PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,0,0);
    if(tls_addr==MAP_FAILED){
        perror("mmap");
        exit(1);
    }
    arch_prctl(ARCH_SET_GS, (unsigned long)tls_addr);
    if((long int)(tn)==0){
        WRITE_THREAD_VARIABLE(a, 1);
        int x=READ_THREAD_VARIABLE(a);
        printf("thread %ld has read x=%d\n", (long int)(tn),x);

    }
    else{
        WRITE_THREAD_VARIABLE(a, 2);
        int x=READ_THREAD_VARIABLE(a);
        printf("thread %ld has read  x=%d\n", (long int)(tn), x);
    }
    return 0;
}

int main(int argc, char *argv[]){
    pthread_t tid;
	long int i;

	for(i=0;i<WORKERS;i++){
		pthread_create(&tid,NULL, thread,(void*)i);
	}

	pause();

	return 0;
}