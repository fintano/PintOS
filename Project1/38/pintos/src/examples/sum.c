#include <stdio.h>
#include <syscall.h>
#include <stdlib.h>
//#include "userprog/syscall.h"

int main(int argc, char **argv)
{
	int arr[4];
	int i;
	//printf("   argc : -%d-\n",argc);
	if(argc!=5)
	{
		return EXIT_FAILURE;
	}
	for(i=0;i<4;i++)
	{
		arr[i]= atoi(argv[i+1]);
		//printf("%d\n",arr[i]);
	}
//	printf(" @@@arr[0] : %d\n",arr[0]);
//	printf(" @@@arr[1] : %d\n",arr[1]);
//	printf(" @@@arr[2] : %d\n",arr[2]);
//	printf(" @@@arr[3] : %d\n",arr[3]);
	
//	printf("  pibo : %d\n",pibo);
//	printf("  sum : %d\n",sum);
	printf("%d %d\n",pibonacci(arr[0]), sum_of_four_integers(arr[0], arr[1], arr[2], arr[3]));
	
	return EXIT_SUCCESS;
	
}
