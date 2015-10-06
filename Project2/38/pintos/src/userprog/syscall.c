#include "userprog/syscall.h"
#include "userprog/synch.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"

static void syscall_handler (struct intr_frame *);
static struct lock sync_key;//정현이가 전역변수로 집어넣음

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&sync_key);
	// initmutexUser(&lockUser);
	// initmutexUser(&lock2User);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	//printf("@@@Syscall start!!!@@@\n"); 
	int *ptr = (int*)f->esp;
	int arg_cnt = *(ptr+11);
	char **arg_addr = *(ptr+12);
	//char *buffer = (char*)malloc(sizeof(char) * 30); // temporarily size

	
	//___sungchul's value
	char* cmd_line;
	void* buffer;
	int status;
	pid_t pid;
	int fd,i;
	unsigned size;
	//___end

	//printf("What is the number of system call?/ %d \n", *ptr);
	/*
	for(i=0;i<30;i++){
		printf("%s ", (char*)(ptr+i));
	}
	*/

	switch(*ptr){
		case SYS_HALT :
		user_halt();
	  	break;
		
		case SYS_EXEC:
			//f->eax = exec(arg_addr[0]);  // TODO

			//sungchul
			cmd_line=(char*)*(ptr+1);
			
			//printf("sungchul : exec -%s-\n", cmd_line);
			f->eax=exec(cmd_line);
		break;
		
		case SYS_READ:
			fd=(int)*(ptr+1);
			buffer=(void*)*(ptr+2);
			size=(unsigned)*(ptr+3);
			f->eax = read(fd, buffer, size);
		break;			
		
		//___sungchul
		case SYS_EXIT :
			status=(int)*(ptr+1);
			exit(status);	
			break;
		
		case SYS_WAIT :
			pid=(pid_t)*(ptr+1);
			//printf(" when SYS_WAIT was called, what is the pid value? %d \n",pid);
			f->eax=wait(pid);
			break;
		
		case SYS_WRITE :
			fd=(int)*(ptr+1);
			buffer=(const void*)*(ptr+2);
			size=(unsigned)*(ptr+3);
			f->eax=write(fd, buffer, size);
			break;
		//___end
		case SYS_PIBO:
			f->eax = pibonacci(*(int *)(f->esp+4));
			break;
		case SYS_SUM:
			f->eax = sum_of_four_integers(*(int *)(f->esp+4), *(int *)(f->esp+8), *(int *)(f->esp+12), *(int *)(f->esp+16));
			break;
  }
	
	//printf("@@@syscall end!!!@@@\n");
  //thread_exit ();
}

void user_halt(){
	shutdown_power_off();
}

int exec(const char *cmd_line){

	int tid;
	
	
	if(*cmd_line == NULL){
			//printf(" ERROR \n");
			return -1;
	}

	//printf(" %s EXEC \n", cmd_line);
	if(cmd_line == NULL){
			//printf(" no such file \n");
			return -1;
	}
	
	tid = process_execute(cmd_line);
	

	return tid;
}

int read(int fd, void *buffer, unsigned size){
	
	char c;
	//char *ch_buffer;
	unsigned int cnt=0;
	int readcount = 0;

	if(fd==0){
		while(cnt<size)
		{
			c=input_getc();
			//(char*)buffer[cnt] = c;
			*((char*)buffer+cnt)=c;
			cnt++;
		}
	}

	return cnt;
}

//___sungchul
void exit(int status)
{
		struct thread* cur=thread_current();
		struct thread* cur_parent=cur->parent;
		char* tmp;
		char* cur_name=strtok_r(cur->name," ",&tmp);
		
	//printf(" CHILD PID : %d \n", cur->tid);	
	acquireUser(&cur_parent->lock2User);	
	if(cur_parent!=NULL) // parents are not orphan
		{
				if(status<=-1)
					cur_parent->exit_status = -1;
				else 
					cur_parent->exit_status = status;
		}
		else
				cur_parent->exit_status = -1;
		
	//	printf(" exit _ status %d \n", cur_parent->exit_status);

		releaseUser(&(cur->lockUser));
		//printf(" EXIT Complete !! \n");

		printf("%s: exit(%d)\n",cur_name,status);
		thread_exit();
}

int wait(pid_t pid)
{

	//printf(" Test Wait Start \n");
	return process_wait(pid);
	//returin 0;	
}

int write(int fd, const void *buffer, unsigned size)
{
	if(fd==1)
		putbuf(buffer, size);

	return -1;
}

int pibonacci (int num)
{
	int prv;
	int i, prv1=0, prv2=1;

	if(num==0)
			return 0;
	else if(num==1 || num==2)
			return 1;
	else
	{
		for(i=1;i<num;i++)
		{
			prv=prv1+prv2;
			prv1=prv2;
			prv2=prv;
		}
		return prv;
	}

}
int sum_of_four_integers(int a, int b, int c, int d)
{
	return a+b+c+d;
}
