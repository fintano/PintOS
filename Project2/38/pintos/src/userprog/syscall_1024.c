#include "userprog/syscall.h"
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
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	printf("@@@Syscall start!!!@@@\n"); 
		  int *ptr = f->esp;
		  int arg_cnt = *(ptr+11);
		  char **arg_addr = *(ptr+12);
		  char *buffer = (char*)malloc(sizeof(char) * 30); // temporarily size
  switch(*ptr){

		  case SYS_HALT :
				//printf("%x %d %s \n",ptr, arg_cnt,addr[0]);
				user_halt();
			  	break;
		  case SYS_EXEC:
				f->eax = exec(arg_addr[0]);
				break;
		  case SYS_READ:
				f->eax = read(arg_addr[1],buffer,arg_addr[3]);
				break;			
  }

  thread_exit ();
}

void user_halt(){
	//printf("HALT\n");	
	shutdown_power_off();
		
}


int exec(const char *cmd_line){

	int tid;
	
	
	lock_acquire(&sync_key);//signal

	tid = process_execute(cmd_line);
	//printf("%d \n", tid);
	/*if(tid == TID_ERROR){
		printf(" pid error \n");
		return -1;	
	}
	*/
	lock_release(&sync_key);//wait	
	
	return tid;
}

int read(int fd, void *buffer, unsigned size){
	
	char c;
	//char *ch_buffer;
	unsigned int cnt;
	int readcount = 0;
	struct lock mutex;
	lock_init(&mutex);

	//ch_buffer = (char*) buffer;
	cnt = 0;
	/*STDIN*/
	do{
	lock_acquire(&mutex);
	readcount++;
	if(readcount == 1) lock_acquire(&sync_key);
	lock_release(&mutex);
	if(fd == 0){
		while(cnt<size){
			c = input_getc();
			*((char*)buffer+cnt) = c;
			cnt++;
		}
	}
	lock_acquire(&mutex);
	readcount--;
	if(readcount == 0) lock_release(&sync_key);
	lock_release(&mutex);
	printf("************************%s \n",(char*)buffer);
	}while(1);
	return cnt;
}
/*
void exit(int status){


}
*/




