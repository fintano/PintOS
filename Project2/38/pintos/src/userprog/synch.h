#ifndef __SYNCH_H__
#define __SUNCH_H__

#include "../threads/synch.h"

#define FALSE 0
#define TRUE 1

void initmutexUser(int *lock);
void acquireUser(int *lock);
void releaseUser(int *lock);

//int lockUser;
//int lock2User;

#endif
