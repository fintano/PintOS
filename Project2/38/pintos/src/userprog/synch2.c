#include <stdio.h>
#include "synch.h"
#include "../threads/interrupt.h"
#include "../threads/thread.h"

void initmutexUser(int *lock){
	//lock_init(lock);
	*lock = FALSE;
}

void acquireUser(){
	/*while(!(*lock)){
		printf("");
	}
	*lock = FALSE;*/
		enum intr_level lv;

		lv = intr_disable();
		thread_block();
		intr_set_level(lv);
}

void releaseUser(struct thread *thd){
	thread_unblock(thd);
}
