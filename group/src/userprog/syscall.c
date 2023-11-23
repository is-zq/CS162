#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"

static void syscall_handler(struct intr_frame*);
static void syscall_exit(int status);
static unsigned syscall_write(int fd,const void *buffer,unsigned size);
static int syscall_practice(int i);

void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }

static void syscall_handler(struct intr_frame* f UNUSED) 
{
	uint32_t* args = ((uint32_t*)f->esp);

	/*
	* The following print statement, if uncommented, will print out the syscall
	* number whenever a process enters a system call. You might find it useful
	* when debugging. It will cause tests to fail, however, so you should not
	* include it in your final submission.
	*/

	/* printf("System call number: %d\n", args[0]); */

	switch(args[0])
	{
	case SYS_EXIT:
		f->eax = args[1];
		syscall_exit(args[1]);
		break;

	case SYS_WRITE:
		syscall_write(args[1],args[2],args[3]);
		break;

	case SYS_PRACTICE:
		f->eax = syscall_practice(args[1]);	
		break;
	default:
		break;
	}
}

static void syscall_exit(int status)
{
	printf("%s: exit(%d)\n", thread_current()->pcb->process_name, status);
	process_exit();
}

static unsigned syscall_write(int fd,const void *buffer,unsigned size)
{
	if(fd == STDOUT_FILENO)
	{
		putbuf(buffer,size);
		return size;
	}
}


static int syscall_practice(int i)
{
	return i+1;
}
