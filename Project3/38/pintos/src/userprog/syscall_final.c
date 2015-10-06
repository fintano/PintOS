#include "userprog/syscall.h"
#include "userprog/synch.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"




static void syscall_handler (struct intr_frame *);
static struct lock sync_key;


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
	//int arg_cnt = *(ptr+11);
	//char **arg_addr = *(ptr+12);

	char* cmd_line;
	void* buffer;
	int status;
	pid_t pid;
	int fd;
	unsigned size;

	//project 2
	char *file;
	unsigned position;

	switch(*ptr){
		
		case SYS_HALT :
			user_halt();
		  	break;
		
		case SYS_EXEC:
			cmd_line=(char*)*(ptr+1);
			f->eax=exec(cmd_line);
			break;
		
		case SYS_READ:
			fd=(int)*(ptr+1);
			buffer=(void*)*(ptr+2);
			size=(unsigned)*(ptr+3);
			f->eax = read(fd, buffer, size);
			break;			
		
		case SYS_EXIT :
			status=(int)*(ptr+1);
			exit(status);	
			break;
		
		case SYS_WAIT :
			pid=(pid_t)*(ptr+1);
			f->eax=wait(pid);
			break;
		
		case SYS_WRITE :
			fd=(int)*(ptr+1);
			buffer=(void*)*(ptr+2);
			size=(unsigned)*(ptr+3);
			f->eax=write(fd, buffer, size);
			break;
	
		case SYS_PIBO:
			f->eax = pibonacci(*(int *)(f->esp+4));
			break;
	
		case SYS_SUM:
			f->eax = sum_of_four_integers(*(int *)(f->esp+4), *(int *)(f->esp+8), *(int *)(f->esp+12), *(int *)(f->esp+16));
			break;
		
		//project 2
		case SYS_CREATE :
			file=(char*)*(ptr+1);
			size=(unsigned)*(ptr+2);
			f->eax = create(file, size);
			break;

		case SYS_REMOVE :
			file=(char*)*(ptr+1);
			f->eax = remove(file);
			break;

		case SYS_OPEN :
			file=(char*)*(ptr+1);
			f->eax = open(file);
			break;

		case SYS_FILESIZE :
			fd = (int)*(ptr+1);
			f->eax = filesize(fd);
			break;

		case SYS_SEEK :
			fd = (int)*(ptr+1);
			position = (unsigned)*(ptr+2);
			seek(fd, position);
			break;
		case SYS_TELL :
			fd = (int)*(ptr+1);
			f->eax = tell(fd);
			break;
		case SYS_CLOSE :
			fd = (int)*(ptr+1);
			close(fd);
			break;

  }
	
}

void user_halt(){
	shutdown_power_off();
}

int exec(const char *cmd_line){

	int tid;
	tid = process_execute(cmd_line);
	
	return tid;
}
//___sungchul
void exit(int status)
{
		struct thread* cur=thread_current();
		struct thread* cur_parent=cur->parent;
		char* tmp;
		char* cur_name=strtok_r(cur->name," ",&tmp);

		///////////////////////////////////////////
		struct list_elem *e;
		struct fdSet_elem *fe;
		struct file *fp;
		
	acquireUser(&cur->lock2User);	
		//lock_acquire(&cur->lock2);
		
		if(cur_parent!=NULL) // parents are not orphan
		{
				if(status<=-1)
					cur->exit_status = -1;
				else 
					cur->exit_status = status;
		}
		else
				cur->exit_status = -1;
		
		//lock_release(&cur->lock1);
		releaseUser(&(cur->lockUser));
		
		//printf("EXIT RETURE VALUE %d : %d ", cur_name,cur->exit_status);
		printf("%s: exit(%d)\n",cur_name,cur->exit_status);
	/*	
		for(e=list_begin(&cur->list_fdSet) ; e!=list_end(&cur->list_fdSet) ; e=list_next(e))
		{
			fe=list_entry(e, struct fdSet_elem, elem);
			if(fe->fd<2)
				break;
			close(fe->fd);

		}
*/
		thread_exit();
}

int wait(pid_t pid)
{

	return process_wait(pid);
}

int read(int fd, void *buffer, unsigned size){
	
	char c;
	unsigned int cnt=0;
	struct list_elem* e;
	struct thread* t = thread_current();
	struct file* fp=NULL;
	struct fdSet_elem* f;
	int temp;

	unsigned int tmp;
	struct lock readLock;
	lock_init(&readLock);
	
	if(size == 0 )
			return 0;

	if(t == NULL || fd == 1 ||size<0 || buffer == NULL)
			return -1;
	 
	if( (int*)buffer > 0xc0000000)
		   exit(-1);	

	if(fd==0){
		while(cnt<size)
		{
			c=input_getc();
			*((char*)buffer+cnt)=c;
			cnt++;
		}
		return cnt;
	}
	else{
		for(e=list_begin(&t->list_fdSet) ; e!=list_end(&t->list_fdSet) ;
			e=list_next(e))
		{
			f = list_entry(e, struct fdSet_elem, elem);
			if(f->fd == fd)
			{
				fp=f->fp;
				break;
			}
		}
		if(fp!=NULL)
		{
			lock_acquire(&readLock);
			temp = file_read(fp,buffer,size);
			lock_release(&readLock);
			return temp;
		}
		else
			return -1;
	}
	return -1;
}

int write(int fd, const void *buffer, unsigned size)
{
	struct list_elem* e;
	struct thread* t = thread_current();
	struct file* fp=NULL;
	struct fdSet_elem* f=NULL;
	int temp;

	struct lock writeLock;
	lock_init(&writeLock);	
	
	if(fd==1)
	{
		putbuf(buffer, size);
		return size;
	}
	else{
		for(e=list_begin(&t->list_fdSet) ; e!=list_end(&t->list_fdSet) ;
			e=list_next(e))
		{
			f=list_entry(e, struct fdSet_elem, elem);
			if(f->fd == fd)
			{
				fp=f->fp;
				break;
			}
		}
		if(fp!=NULL)
		{
				lock_acquire(&writeLock);
			temp=file_write(fp, buffer, size);
			lock_release(&writeLock);
			//printf("fd : -%d- / size : -%d- / temp : -%d-\n",f->fd,size, temp);
			return temp;
		}
	}
	//printf("*** write end ***\n");
	return -1;
}

int pibonacci (int num)
{
	int i, prv=0, prv1=0, prv2=1;

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

bool create(const char *file, unsigned initial_size)
{

	if(file==NULL)
	{
		exit(-1);
		return false;
	}
	return filesys_create(file, initial_size);
}

bool remove(const char *file)
{
	return filesys_remove(file);
}

int open (const char *file)
{
	struct file* f;
	int fd;

	// for test
	if(file==NULL)
			return -1;

	f=filesys_open(file); // converting file_name to file pointer (returning)
	if(f==NULL) //error or no file
	{
		return -1;
	}
	fd=open_thread(f);

	return fd;
}

int open_thread(struct file* fp){
		struct thread *t = thread_current();
		int fd;
		struct fdSet_elem* e;  
		e=(struct fdSet_elem*)malloc(sizeof(struct fdSet_elem));
		e->fd = (t->fdCnt)+2; // 0,1 remain
		fd=e->fd;
		e->fp = fp;
		
		list_push_back(&t->list_fdSet,&e->elem);
		t->fdCnt++;
		return fd;
}



int filesize(int fd)
{
	struct list_elem* e;
	struct thread* t = thread_current();
	struct file* fp=NULL;

	for(e=list_begin(&t->list_fdSet) ; e!=list_end(&t->list_fdSet) ;
		e=list_next(e))
	{
		struct fdSet_elem* f=list_entry(e, struct fdSet_elem, elem);
		if(f->fd == fd)
		{
			fp=f->fp;
			break;
		}
	}

	if(fp!=NULL)
	{
		return  file_length(fp);
	}
	else{
			printf("FILE SIZE ERROR \n");
			return 0;
	}
}

void seek(int fd, unsigned position)
{
	struct list_elem* e;
	struct thread* t = thread_current();
	struct file* fp=NULL;
	for(e=list_begin(&t->list_fdSet) ; e!=list_end(&t->list_fdSet) ;
		e=list_next(e))
	{
		struct fdSet_elem* f=list_entry(e, struct fdSet_elem, elem);
		if(f->fd == fd)
		{
			fp=f->fp;
			break;
		}
	}
	if(fp!=NULL)
	{
		file_seek(fp, position);
	}
	else
		printf(" SEEK ERROR \n");
}

unsigned tell(int fd)
{
	struct list_elem* e;
	struct thread* t = thread_current();
	struct file* fp=NULL;
	off_t pos=0;
	for(e=list_begin(&t->list_fdSet) ; e!=list_end(&t->list_fdSet) ;
		e=list_next(e))
	{
		struct fdSet_elem* f=list_entry(e, struct fdSet_elem, elem);
		if(f->fd == fd)
		{
			fp=f->fp;
			break;
		}
	}
	if(fp!=NULL)
	{
		pos = file_tell(fp);
	}
	return pos;
}

void close(int fd)
{
	struct list_elem* e;
	struct thread* t = thread_current();
	struct file* fp=NULL;
	struct fdSet_elem* fe=NULL;
	struct fdSet_elem* f=NULL;
	
	for(e=list_begin(&t->list_fdSet) ; e!=list_end(&t->list_fdSet) ;
		e=list_next(e))
	{
		f=list_entry(e, struct fdSet_elem, elem);
		if(f->fd == fd)
		{
			fe=f;
			fp=f->fp;
			list_remove(e);
			break;
		}
	}
	
	if(fp!=NULL)
	{
		file_close(fp);
		free(fe);
	}
}


