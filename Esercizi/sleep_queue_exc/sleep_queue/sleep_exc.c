/*
Blocking Queuing Service (BQS)
This homework deals with the implementation of a Linux kernel subsystem dealing with thread management. 
The subsystem should implement a blocking FIFO-queuing service. It is based on two system calls:
1) int goto_sleep(void) used to make a thread sleep at the tail of the queue.
2) int awake(void) used to awake the thread currently standing at the head of the queue. 
Threads could also be awaken in non-FIFO order because of Posix signals.
*/

#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/apic.h>
#include <asm/io.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include "lib/include/scth.h"
#include "lib/include/node_list.h"




MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matteo Conti <matteo.conti.97@students.uniroma2.eu>");

#define MODNAME "sleep_exercise"

unsigned long the_syscall_table = 0x0;
module_param(the_syscall_table, ulong, 0660);


unsigned long the_ni_syscall;

unsigned long new_sys_call_array[] = {0x0, 0x1};//please set to sys_vtpmo at startup
#define HACKED_ENTRIES (int)(sizeof(new_sys_call_array)/sizeof(unsigned long))
//#define HACKED_ENTRIES 2
int restore[HACKED_ENTRIES] = {[0 ... (HACKED_ENTRIES-1)] -1};

node head = {NULL, -1, -1, NULL, NULL};
node tail ={NULL, -1, -1, NULL, NULL};
spinlock_t queue_lock;

#define AUDIT if(1)


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(1, _goto_sleep, unsigned long, none){
#else
asmlinkage long sys_goto_sleep(void){
#endif

        DECLARE_WAIT_QUEUE_HEAD(queue);
	node *new_node = (node*)kmalloc(sizeof(node), GFP_KERNEL);
        new_node->task = current;
        new_node->pid = current->pid;
        new_node->awake = 0;

        //Add the new node to the queue
        preempt_disable();
        spin_lock(&queue_lock);
        printk("%s: thread %d inserted in queue\n",MODNAME,current->pid);
        insert_node(&tail, new_node);
        spin_unlock(&queue_lock);
        preempt_enable();

        //Sleep until awake is set to 0
        printk("%s: thread %d going to sleep\n",MODNAME,current->pid);
        wait_event_interruptible(queue, new_node->awake == 1);
        
        //After wakeup, remove the node from the queue
        preempt_disable();
        spin_lock(&queue_lock);
        printk("%s: thread %d removed from queue\n",MODNAME,current->pid);
        remove_node(current->pid, &head);
        spin_unlock(&queue_lock);
        preempt_enable();
        
	return 0;
	
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
__SYSCALL_DEFINEx(1, _awake, unsigned long, none){
#else
asmlinkage long sys_awake(void){
#endif

        //Add the new node to the queue
        preempt_disable();
        spin_lock(&queue_lock);
        if(head.next == &tail){
                  printk("%s: queue is empty\n",MODNAME);
                  spin_unlock(&queue_lock);
                  preempt_enable();
                  return -1;
        }
        head.next->awake = 1;
        wake_up_process(head.next->task);
        spin_unlock(&queue_lock);
        preempt_enable();

	return 0;
	
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
long sys_goto_sleep = (unsigned long) __x64_sys_goto_sleep;       
#else
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
long sys_awake = (unsigned long) __x64_sys_awake;       
#else
#endif


int init_module(void) {

        int i;
        int ret;
        head.next = &tail;
        tail.prev = &head;

        if (the_syscall_table == 0x0){
           printk("%s: cannot manage sys_call_table address set to 0x0\n",MODNAME);
           return -1;
        }

	AUDIT{
	   printk("%s: printk-example received sys_call_table address %px\n",MODNAME,(void*)the_syscall_table);
     	   printk("%s: initializing - hacked entries %d\n",MODNAME,HACKED_ENTRIES);
	}

	new_sys_call_array[0] = (unsigned long)sys_goto_sleep;
        new_sys_call_array[1] = (unsigned long)sys_awake;

        ret = get_entries(restore,HACKED_ENTRIES,(unsigned long*)the_syscall_table,&the_ni_syscall);

        if (ret != HACKED_ENTRIES){
                printk("%s: could not hack %d entries (just %d)\n",MODNAME,HACKED_ENTRIES,ret); 
                return -1;      
        }

	unprotect_memory();

        for(i=0;i<HACKED_ENTRIES;i++){
                ((unsigned long *)the_syscall_table)[restore[i]] = (unsigned long)new_sys_call_array[i];
        }

	protect_memory();

        printk("%s: all new system-calls correctly installed on sys-call table\n",MODNAME);

        return 0;

}

void cleanup_module(void) {

        int i;
                
        printk("%s: shutting down\n",MODNAME);

	unprotect_memory();
        for(i=0;i<HACKED_ENTRIES;i++){
                ((unsigned long *)the_syscall_table)[restore[i]] = the_ni_syscall;
        }
	protect_memory();
        printk("%s: sys-call table restored to its original content\n",MODNAME);
        
}
