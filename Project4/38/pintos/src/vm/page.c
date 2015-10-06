#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "vm/page.h"



// install_page 사용
// frame 을 얻어서 page에 연결시키는역할
// writable 은 true였고 
// 아 그리고 stackgrowth니까 cnt를 구한다 
// 그만큼 foR문돌리고 frame을 userPOOL에서 얻어서 vaddr이랑 연결시키는것.
// 넘어온 page / frame 할당 / true

void stack_growth(void* faultAddr, void* stackPtr){
	
	void *kpage, *upage;
	bool writable = true;
	int i,growCnt;

	upage = faultAddr;
	kpage = palloc_get_page (PAL_USER);
	install_page (upage, kpage, writable);
}

static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *th = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  /* virtual page가 unmapped 되었다는 것 확인한 뒤 kpage를 upage가 가르키게 한다. */
  
  return (pagedir_get_page (th->pagedir, upage) == NULL
          && pagedir_set_page (th->pagedir, upage, kpage, writable));
}

// 아 다시짜야돼 망할. ..
