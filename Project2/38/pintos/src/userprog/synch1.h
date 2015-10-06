#ifndef __SYNCH_H__
#define __SUNCH_H__

#include "../threads/synch.h"

#define FALSE 0
#define TRUE 1

void initmutexUser(struct lock *lock);
void acquireUser(struct lock *lock);
void releaseUser(struct lock *lock);

//static struct lock lockUser;
//static struct lock lock2User;
#endif
