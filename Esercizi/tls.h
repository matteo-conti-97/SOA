#include <asm/prctl.h>
#include <sys/prctl.h>
#include <sys/mman.h>

#define PER_THREAD_MEMORY_START typedef struct _TLS_ZONE{\
				unsigned long _tls_address;
#define PER_THREAD_MEMORY_END }TLS_ZONE;

extern int arch_prctl(int code, unsigned long addr);

unsigned long get_gs_pos(){
	unsigned long gs;
	arch_prctl(ARCH_GET_GS, gs);
	return gs;
}

#define READ_THREAD_VARIABLE(name)\
				((TLS_ZONE *)(get_gs_pos()))->name;

#define WRITE_THREAD_VARIABLE(name, value)\
				((TLS_ZONE *)(get_gs_pos()))->name=value;