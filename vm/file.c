/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "userprog/process.h"
#include "threads/vaddr.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);
void do_munmap (void *addr);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
}

/* Do the munmap */
/**
 * 주어진 가상 주소 addr에 대응하는 메모리 맵핑을 제거하는 함수
 * munmap 시스템 호출이 발생할 때 호출
*/
void
do_munmap (void *addr) {
		while(true){
		struct thread *curr = thread_current();
		struct page *find_page = spt_find_page(&curr->spt, addr);
		
		if (find_page == NULL) {
    		return NULL;
    	}

		struct container* container = (struct container*)find_page->uninit.aux;
		/* 페이지가 수정되었는지 (dirty bit이 설정되어 있는지) 확인  */
		if (pml4_is_dirty(curr->pml4, find_page->va)){
			/* 변경된 페이지의 내용을 파일 시스템에 다시 쓰고, 페이지의 dirty bit을 클리어 */
			file_write_at(container->file, addr, container->read_bytes, container->offset);
			pml4_set_dirty(curr->pml4,find_page->va,0);
		} 
		/*  present bit을 클리어하여, 페이지가 더 이상 메모리에 존재하지 않음을 표시 */
		pml4_clear_page(curr->pml4, find_page->va); 
		/* addr를 페이지 크기만큼 증가시켜 다음 페이지를 처리 
		로직은 while(true) 루프 내부에 있으므로, 주어진 주소 범위에 있는 모든 페이지에 대해 이 과정이 반복 */
		addr += PGSIZE;
	}
}
