#ifndef __SYNCH_H__
#define __SUNCH_H__

#include "../threads/synch.h"

#define FALSE 0
#define TRUE 1

void initmutexUser(int *lock);
void acquireUser(void);
void releaseUser(struct thread *thd);

//int lockUser;
//int lock2User;

#endif
