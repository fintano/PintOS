#include "lib/stdbool.h"
#include "userprog/process.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "lib/kernel/console.h"
#include "lib/string.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
//___sungchul
typedef int pid_t;
//___end

struct fdSet_elem{ // for storing fd in thread
	struct list_elem elem;
	int fd;
	struct file* fp;
};


void open_update(struct file* fp);
int open_thread(struct file* fp);

void syscall_init (void);
unsigned int read_error(void *buffer);

void user_halt(void);
int exec(const char *cmd_line);
int read(int fd,void *buffer, unsigned size);

//___sungchul
void exit(int status);
int wait(pid_t pid);
int write(int fd, const void *buffer, unsigned int size);

int pibonacci(int num);
int sum_of_four_integers(int a, int b, int c, int d);

//project 2
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int open_thread(struct file* fp);
int filesize(int fd);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);

//___end

#endif /* userprog/syscall.h */
