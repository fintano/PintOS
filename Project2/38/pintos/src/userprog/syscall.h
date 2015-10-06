#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

//___sungchul
typedef int pid_t;
//___end

void syscall_init (void);

void user_halt(void);
int exec(const char *cmd_line);
int read(int fd,void *buffer, unsigned size);

//___sungchul
void exit(int status);
int wait(pid_t pid);
int write(int fd, const void *buffer, unsigned int size);

int pibonacci(int num);
int sum_of_four_integers(int a, int b, int c, int d);

//___end

#endif /* userprog/syscall.h */
