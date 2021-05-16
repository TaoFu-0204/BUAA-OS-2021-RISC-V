#ifndef _PMAP_H_
#define _PMAP_H_

#include "types.h"
#include "queue.h"
#include "mmu.h"
#include "printf.h"

LIST_HEAD(Page_list, Page);
typedef LIST_ENTRY(Page) Page_LIST_entry_t;

struct Page {
	Page_LIST_entry_t pp_link;	/* free list link */

	// Ref is the count of pointers (usually in page table entries)
	// to this page.  This only holds for pages allocated using
	// page_alloc.  Pages allocated at boot time using pmap.c's "alloc"
	// do not have valid reference count fields.

	u_short pp_ref;
};

extern struct Page *pages;
extern struct Page *pages_paddr;

static inline u_int64_t
page2ppn(struct Page *pp)
{
	return pp - pages;
}

/* Get the physical address of Page 'pp'.
 */
static inline u_int64_t
page2pa(struct Page *pp)
{
	return (page2ppn(pp) << PGSHIFT) + 0x80000000;
}

/* Get the Page struct whose physical address is 'pa'.
 */
static inline struct Page *
pa2page(u_int64_t pa)
{
	if (PPN(PADDR2ACTMEM(pa)) >= npage) {
		panic("pa2page called with invalid pa: %x", pa);
	}

	return &pages[PPN(PADDR2ACTMEM(pa))];
}

/* Get the kernel virtual address of Page 'pp'.
 */
static inline u_int64_t
page2kva(struct Page *pp)
{
	return KADDR(page2pa(pp));
}

/* Transform the virtual address 'va' to physical address.
 */
static inline u_long
va2pa(Pde *pgdir, u_long va)
{
	Pte *p;

	pgdir = &pgdir[PDX(va)];

	if (!(*pgdir & PTE_V)) {
		return ~0;
	}

	p = (Pte *)KADDR(PTE_ADDR(*pgdir));

	if (!(p[PTX(va)]&PTE_V)) {
		return ~0;
	}

	return PTE_ADDR(p[PTX(va)]);
}

/********** functions for memory management(see implementation in mm/pmap.c). ***********/

void riscv_detect_memory();

void riscv_vm_init();

void riscv_init();
void page_init(void);
void page_check();
int page_alloc(struct Page **pp);
void page_free(struct Page *pp);
void page_decref(struct Page *pp);
int pgdir_walk(Pte *vpt2, u_int64_t va, int create, Pte **vpt0e);
int page_insert(Pte *vpt2, struct Page *pp, u_int64_t va, u_int perm);
struct Page *page_lookup(Pte *vpt2, u_int64_t va, Pte **vpt0e);
void page_remove(Pte *vpt2, u_int64_t va) ;
void tlb_invalidate(Pte *vpt2, u_int64_t va);

void boot_map_segment(Pde *pgdir, u_long va, u_long size, u_long pa, u_int64_t perm);

extern struct Page *pages;

void asmprint(u_int64_t x);

#endif /* _PMAP_H_ */
