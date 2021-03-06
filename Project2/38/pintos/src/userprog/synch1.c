#include <stdio.h>
#include "synch.h"

void initmutexUser(struct lock *lock){
	lock_init(lock);
}

void acquireUser(struct lock *lock){
	lock_acquire(lock);
}

void releaseUser(struct lock *lock){
	lock_release(lock);
}
