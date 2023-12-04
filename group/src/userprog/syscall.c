#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "pagedir.h"

static void syscall_handler(struct intr_frame*);
static void syscall_exit(int status);
static unsigned syscall_write(int fd,const void *buffer,unsigned size);
static int syscall_practice(int i);
static void syscall_halt(void);
static pid_t syscall_exec(const char *cmd_line);
static int syscall_wait(pid_t pid);

static void validate_byte(const char* byte)
{
	if(byte == NULL || !is_user_vaddr(byte)
			|| pagedir_get_page(thread_current()->pcb->pagedir,byte) == NULL)
	{
		syscall_exit(-1);
	}
}
static void validate_string(const char* string)
{
	char* ch = string;
	while(true)
	{
		validate_byte(ch);
		if(*ch == '\0')
			break;
		++ch;
	}
}
static void validate_buffer(const void* buffer,size_t len)
{
	char* addr_b = (char*)buffer;
	for(size_t i=0;i<len;i++)
		validate_byte(addr_b++);
}
static void validate_args(const uint32_t* argv,int count)
{
	validate_buffer(argv,count * sizeof(uint32_t));
}

void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }

static void syscall_handler(struct intr_frame* f) 
{
	uint32_t* args = ((uint32_t*)f->esp);

	/*
	* The following print statement, if uncommented, will print out the syscall
	* number whenever a process enters a system call. You might find it useful
	* when debugging. It will cause tests to fail, however, so you should not
	* include it in your final submission.
	*/

	/* printf("System call number: %d\n", args[0]); */
	validate_args(args,1);

	switch(args[0])
	{
	case SYS_EXIT:
		validate_args(args+1, 1);
		f->eax = (int)args[1];
		syscall_exit((int)args[1]);
		break;

	case SYS_WRITE:
		validate_args(args+1, 3);
		syscall_write((int)args[1],(void*)args[2],(unsigned)args[3]);
		break;

	case SYS_PRACTICE:
		validate_args(args+1, 1);
		f->eax = syscall_practice((int)args[1]);	
		break;
	case SYS_HALT:
		syscall_halt();
		break;
	case SYS_EXEC:
		validate_args(args+1, 1);
		validate_string((char*)args[1]);
		f->eax = syscall_exec((char*)args[1]);
		break;
	case SYS_WAIT:
		validate_args(args+1, 1);
		f->eax = syscall_wait((pid_t)args[1]);
	default:
		break;
	}
}

static void syscall_exit(int status)
{
	struct thread* t = thread_current();
	if(t->pcb->ppcb != NULL)
		childList_get(&t->pcb->ppcb->child_list,t->tid)->exit_status = status;
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

static void syscall_halt(void)
{
	shutdown_power_off();
}

static pid_t syscall_exec(const char *cmd_line)
{
	return process_execute(cmd_line);
}

static int syscall_wait(pid_t pid)
{
	return process_wait(pid);
}
