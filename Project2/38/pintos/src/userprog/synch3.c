#include <stdio.h>
#include <unistd.h>
#include <windows.h>
#include "synch.h"
#include "../threads/interrupt.h"
#include "../threads/thread.h"

void initmutexUser(int *lock){
	//lock_init(lock);
	*lock = FALSE;
}

void acquireUser(int *lock){

	int a[100];
	int b[100];

	int result[100], i;

	memset(a,42957,100);
	memset(b,34525235,100);

	while(1){
			if( *lock == TRUE)
					break;
			/*
			for(i=0;i<100;i++){
				result[i] = a[i] *a[i] * b[i] * b[i];
			}
			*/
			printf("");
			//Sleep(1);
			//sleep(1);
	}
	*lock = FALSE;
		
}

void releaseUser(int *lock){
	*lock = TRUE;
}
