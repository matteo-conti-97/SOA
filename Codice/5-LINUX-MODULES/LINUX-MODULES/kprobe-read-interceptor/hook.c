#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/printk.h>      
#include <linux/ptrace.h>       
#include <linux/syscalls.h>
#include <linux/version.h>


MODULE_AUTHOR("Francesco Quaglia <francesco.quaglia@uniroma2.it>");
MODULE_DESCRIPTION("This module intecepts the return of the sys_read kernel function from standard-input\
	       		for a target process it then audits the read bytes into the dmesg buffer");

#define MODNAME "READ-INTERCEPTOR"

//what function to hook depending on the system call architecture
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
#define target_func "__x64_sys_read"
#else
#define target_func "sys_read"
#endif 

//where to look at when searching system call parmeters
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
#define get(regs)	regs = (struct pt_regs*)the_regs->di;
#else
#define get(regs)	regs = the_regs;
#endif

#ifdef CONFIG_THREAD_INFO_IN_TASK
int offset = sizeof(struct thread_info);
#else
int offset = 0;
#endif

//where to store the address of the user level buffer for the read operation depends on the CONFIG_THREAD_INFO_IN_TASK kernel compile time parameter
#define store_address(addr) *(unsigned long*)((void*)current->stack + offset) = addr
#define load_address(addr) addr = (*(unsigned long*)((void*)current->stack + offset))


unsigned long audit_counter = 0;//just to audit how manu times the interceptor hook (the return krobe) has been called
module_param(audit_counter, ulong, S_IRUSR | S_IRGRP | S_IROTH);

long target_pid = -1;//the target process id to intercetp - please setup it via the /sys file system
module_param(target_pid, long, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);


//this function is executed at the startup of the kretprobed one - we record the address of the atarget user space buffer in a per thread location on the top of the stack area
static int the_pre_hook(struct kprobe *ri, struct pt_regs *the_regs) { 

	struct pt_regs * regs;
	unsigned long addr; 

	atomic_inc((atomic_t*)&audit_counter);//just mention we passed here via the atomic counter

	get(regs);//get the actual address of the CPU image seen by the system call (or its wrapper)

	if(regs->di != 0) return 1;//not reading from standard input
	if (current->pid != (pid_t)target_pid) return 1;//read not by the target process

	addr = regs->si;
	printk("%s: pre-handler - target address is %px", MODNAME, (void*)addr);
	store_address(addr);

	return 0;//reading from standard input - the kretprobe needs to be executed
}


DEFINE_PER_CPU(unsigned long, test);//this is used for a brute force search

//we reuse in this kretprobe the address saved by the previous function to grub access to the bytes delivered to the user and log them via printk
static int the_hook(struct kretprobe_instance *ri, struct pt_regs *the_regs) { 

	unsigned long addr;
	char c;
	int  i;
	unsigned long* temp = (unsigned long) &test;


	//here we do not need to check if we are under interception of the read operation since this job has
	//already been done by the prehandler

	load_address(addr);
	if (addr == 0x0) goto end;//chek if we saved the address of the user level buffer for the I/O operation

	if (regs_return_value(the_regs) <= 0) goto end;//check if the read operation actually returned data


	while (temp > 0) {//brute force search of the current_kprobe per CPU-variable
			  //for enabling blocking execution of the kretprobe
			  //you can save this time setting up a per CPU-variable via 
			  //smp_call_function() upon module startup
                temp -= 1; 
                if ((unsigned long) __this_cpu_read(*temp) == (unsigned long) &ri->rph->rp->kp) {
                        break;
                }
        }

	if (temp == 0) goto end;

	printk("%s: thread %d got this from the read service on stdin (num bytes is %ld - target address is %px): ", MODNAME, current->pid, regs_return_value(the_regs), (void*)addr);

        __this_cpu_write(*temp, NULL);
        preempt_enable();
	//now we are safe to execute blocking services

	i = 0;
	while( !copy_from_user((void*)&c,(void*)(addr)+i,1) && (i < regs_return_value(the_regs))){
		       	printk(KERN_CONT "%c", c);
			++i;
	}
	printk("\n");

	//now we restore the kprobe status to be managed
        preempt_disable();
        __this_cpu_write(*temp, &ri->rph->rp->kp);
end:
	return 0;

}

static struct kretprobe retprobe;  

static int  hook_init(void) {

	int ret;

	if (LINUX_VERSION_CODE < KERNEL_VERSION(5,11,0)){
	 	printk("%s: unsupported kernel version", MODNAME);
		return -1; 	
	};

	retprobe.kp.symbol_name = target_func;
	retprobe.handler = (kretprobe_handler_t)the_hook;
	retprobe.entry_handler = (kretprobe_handler_t)the_pre_hook;
	retprobe.maxactive = -1; //lets' go for the default number of active kretprobes manageable by the kernel

	ret = register_kretprobe(&retprobe);
	if (ret < 0) {
		printk("%s: hook init failed, returned %d\n", MODNAME, ret);
		return ret;
	}
	printk("%s: read hook module correctly loaded\n", MODNAME);
	
	return 0;
}// hook_init

static void  hook_exit(void) {

	unregister_kretprobe(&retprobe);

	printk("%s: read hook invoked %lu times\n",MODNAME,audit_counter);
	printk("%s: hook module unloaded\n",MODNAME);

}// hook_exit

module_init(hook_init)
module_exit(hook_exit)
MODULE_LICENSE("GPL");
