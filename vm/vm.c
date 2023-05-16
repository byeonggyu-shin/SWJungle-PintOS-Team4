/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "lib/kernel/hash.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include <stdbool.h>

/*----------------[project3]-------------------*/
static unsigned vm_hash_func(const struct hash_elem *e, void *aux);
static bool vm_less_func(const struct hash_elem *a, const struct hash_elem *b);
/*----------------[project3]-------------------*/

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void vm_init(void)
{
	vm_anon_init();
	vm_file_init();
#ifdef EFILESYS /* For project 4 */
	pagecache_init();
#endif
	register_inspect_intr();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type(struct page *page)
{
	int ty = VM_TYPE(page->operations->type);
	switch (ty)
	{
	case VM_UNINIT:
		return VM_TYPE(page->uninit.type);
	default:
		return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim(void);
static bool vm_do_claim_page(struct page *page);
static struct frame *vm_evict_frame(void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
/* 페이지 할당 및 초기화하는 함수 */
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable,
									vm_initializer *init, void *aux)
{
	ASSERT(VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current()->spt;

	/* Check whether the upage is already occupied or not. */
	/*----------------[project3]-------------------*/
	if (spt_find_page(spt, upage) == NULL)
	{
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		struct page *new_page = palloc_get_page(PAL_ZERO);
		switch (VM_TYPE(type))
		{
		case VM_ANON:
			uninit_new(new_page, upage, init, type, aux, anon_initializer);
			break;
		case VM_FILE:
			uninit_new(new_page, upage, init, type, aux, file_backed_initializer);
			break;
		default:
			break;
		}
		new_page->writable = writable;
		new_page->t = thread_current();
		/* TODO: Insert the page into the spt. */
		if (spt_insert_page(spt, new_page))
		{
			return true;
		}
		else
		{
			return false;
		}
		/*----------------[project3]-------------------*/
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED)
{ /*----------------[project3]-------------------*/

	struct page *page = NULL;
	/* pg_round_down()으로 vaddr의 페이지 번호를 얻음 */
	uint64_t va_page_num = pg_round_down(va);
	/* Create a temporary vm_entry to use for searching */
	page->va = va_page_num;
	/* Prepare a hash_elem for the search */
	struct hash_elem *temp_hash_elem = hash_find(spt->hash_table, &(page->hash_elem));
	/* Check if the element was found */
	/* 만약 존재하지 않는다면 NULL 리턴 */
	if (temp_hash_elem == NULL)
	{
		return NULL;
	}
	/* Return the vm_entry structure of the corresponding hash_elem with hash_entry() */
	/* hash_entry()로 해당 hash_elem의 page 구조체 리턴 */
	return hash_entry(temp_hash_elem, struct page, hash_elem);
	/*----------------[project3]-------------------*/
}

/* Insert PAGE into spt with validation. */
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
					 struct page *page UNUSED)
{
	/* TODO: Fill this function. */
	return (hash_insert(&spt->hash_table, &page->hash_elem)) ? false : true; /* 🤔 */
}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page)
{
	pml4_clear_page(thread_current()->pml4, page->va);
	hash_delete(&spt->hash_table, &page->hash_elem);

	if (page->frame != NULL)
	{
		page->frame->page = NULL;
	}
	vm_dealloc_page(page);
	// hash_delete(spt, &page->hash_elem);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim(void)
{
	struct frame *victim = NULL;
	/* TODO: The policy for eviction is up to you. */
	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame(void)
{
	struct frame *victim UNUSED = vm_get_victim();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame(void)
{
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	char *new_memory;

	if (new_memory = palloc_get_page(PAL_USER))
	{
		frame->kva = new_memory;
		frame->page = NULL;
	}
	else
	{
		PANIC("todo");
	}

	ASSERT(frame != NULL);
	ASSERT(frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth(void *addr UNUSED)
{
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp(struct page *page UNUSED)
{
}

/* Return true on success */
bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
						 bool user UNUSED, bool write UNUSED, bool not_present UNUSED)
{
	struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
	struct page *page;
	/*----------------[project3]-------------------*/
	/* 주소가 커널 영역일 때 */
	if (is_kernel_vaddr(addr))
	{
		return false;
	}

	/* 페이지가 메모리에 없을 때 => 찐 페이지폴트 */
	if (not_present)
	{
		return false;
	}

	/* bogus 폴트일 때 */
	if ()
	{
	}
	/*----------------[project3]-------------------*/

	return vm_do_claim_page(page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void vm_dealloc_page(struct page *page)
{
	destroy(page);
	free(page);
}

/* Claim the page that allocate on VA. */
bool vm_claim_page(void *va UNUSED)
{
	struct page *page = NULL;
	/* TODO: Fill this function */
	page->va = va;

	return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page *page)
{
	struct frame *frame = vm_get_frame();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	if (!spt_insert_page(&thread_current()->spt, page))
	{
		return false;
	}
	return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED)
{
	struct hash cur_hash = spt->hash_table;
	hash_init(&cur_hash, vm_hash_func, vm_less_func, NULL);
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
								  struct supplemental_page_table *src UNUSED)
{
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}

/*----------------[project3]-------------------*/
/* vm_entry의 vaddr을 인자값으로 hash_int() 함수를 사용하여 해시 값 반환 */
static unsigned vm_hash_func(const struct hash_elem *e, void *aux)
{
	/* hash_entry()로 element에 대한 vm_entry 구조체 검색 */
	void *hash_va = hash_entry(e, struct page, hash_elem)->va;
	/* hash_int()를 이용해서 vm_entry의 멤버 vaddr에 대한 해시값을 구하고 반환 */
	return hash_int((uint64_t)&hash_va);
}

/*
 * 입력된 두 hash_elem의 vaddr 비교
 * a의 vaddr이 b보다 작을 시 true 반환
 * a의 vaddr이 b보다 클 시 false 반환
 */
static bool vm_less_func(const struct hash_elem *a, const struct
						 hash_elem *b)
{
	/* hash_entry()로 각각의 element에 대한 vm_entry 구조체를 얻은 후
	vaddr 비교 (b가 크다면 true, a가 크다면 false */
	void *hash_A = hash_entry(a, struct page, hash_elem)->va;
	void *hash_B = hash_entry(b, struct page, hash_elem)->va;

	return (hash_A) < (hash_B);
}
/*----------------[project3]-------------------*/