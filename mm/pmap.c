#include "mmu.h"
#include "pmap.h"
#include "printf.h"
#include "env.h"
#include "error.h"



/* These variables are set by i386_detect_memory() */
u_int64_t maxpa;            /* Maximum physical address */
u_int64_t npage;            /* Amount of memory(in pages) */
u_int64_t basemem;          /* Amount of base memory(in bytes) */
u_int64_t extmem;           /* Amount of extended memory(in bytes) */

Pte *boot_vpt2;

struct Page *pages;
struct Page *pages_paddr;
static u_int64_t freemem;

static struct Page_list page_free_list;	/* Free list of physical pages */


// Overview:
// 	Initialize basemem and npage.
// 	Set basemem to be 64MB, and calculate corresponding npage value.
void riscv_detect_memory()
{
    /* Step 1: Initialize basemem.
     * (When use real computer, CMOS tells us how many kilobytes there are). */
    maxpa = 0x088000000; // 2^26 Bytes, 64MB
    //maxpa = 0x1000000;
    // npage = maxpa >> PGSHIFT; // 2^12 Bytes, 4KB per page
    basemem = maxpa - 0x080000000;
    npage = basemem / BY2PG;
    extmem = 0;
    // Step 2: Calculate corresponding npage value.

    printf("Physical memory: %dK available, ", (int)(maxpa / 1024));
    printf("base = %dK, extended = %dK\n", (int)(basemem / 1024),
           (int)(extmem / 1024));
}

// Overview:
// 	Allocate `n` bytes physical memory with alignment `align`, if `clear` is set, clear the
// 	allocated memory. 
// 	This allocator is used only while setting up virtual memory system.
// 
// Post-Condition:
//	If we're out of memory, should panic, else return this address of memory we have allocated.
static void *alloc(u_int64_t n, u_int64_t align, int clear)
{
    extern char end[];
    u_int64_t alloced_mem;

    /* Initialize `freemem` if this is the first time. The first virtual address that the
     * linker did *not* assign to any kernel code or global variables. */
    if (freemem == 0) {
        freemem = (u_int64_t)end; // end
    }
    /* Step 1: Round up `freemem` up to be aligned properly */
    freemem = ROUND(freemem, align);

    /* Step 2: Save current value of `freemem` as allocated chunk. */
    alloced_mem = freemem;

    /* Step 3: Increase `freemem` to record allocation. */
    freemem = freemem + n;

    // We're out of memory, PANIC !!
    if (PADDR(freemem) >= maxpa) {
        panic("out of memorty\n");
        return (void *)-E_NO_MEM;
    }

    /* Step 4: Clear allocated chunk if parameter `clear` is set. */

    if (clear) {
        bzero((void *)alloced_mem, n);
    }

    /* Step 5: return allocated chunk. */
//printf("alloc() : alloced mem is %lx, freemem is %lx\n", alloced_mem, freemem);
    return (void *)alloced_mem;
}

static Pte *boot_vpt1_walk(Pte *vpt1, u_int64_t va, int create)
{

    Pte *vpt1_entry, *vpt0, *vpt0_entry;

    vpt1_entry = vpt1 + VPN1(va);
    vpt0 = (Pte *)PTE_TO_PADDR(*vpt1_entry);

    if (((*vpt1_entry) & PTE_V) != PTE_V) {
        if (create == 1) {
            vpt0 = (Pte *)alloc(BY2PG, BY2PG, 1);
	    *vpt1_entry = PADDR_TO_PTE(vpt0) | PTE_V ;//| PTE_R;
	}
    }

    vpt0_entry = vpt0 + VPN0(va);
    return vpt0_entry;

}

// Overview:
// 	Get the page table entry for virtual address `va` in the given
// 	page directory `pgdir`.
//	If the page table is not exist and the parameter `create` is set to 1, 
//	then create it.
static Pte *boot_vpt2_walk(Pte *vpt2, u_int64_t va, int create)
{

    Pte *vpt2_entryp;
    Pte *vpt1, *vpt1_entry;
    u_int64_t i;

    /* Step 1: Get the corresponding page directory entry and page table. */
    /* Hint: Use KADDR and PTE_ADDR to get the page table from page directory
     * entry value. */
    vpt2_entryp = vpt2 + VPN2(va);
    //printf("vpt2_ent_addr:%lx\n", vpt2_entryp);
    vpt1 = (Pte *)PTE_TO_PADDR(*vpt2_entryp);
    //printf("vpt1addr:%lx\n", vpt1);

    /* Step 2: If the corresponding page table is not exist and parameter `create`
     * is set, create one. And set the correct permission bits for this new page
     * table. */
    if (((*vpt2_entryp) & PTE_V) == 0) {
        if (create == 1) {
            // Corresponding page table doesn't exist, indicating that 'vpt1' is useless.
            // Need to create one and update vpt2.
            vpt1 = (Pte *)alloc(BY2PG, BY2PG, 1);
	    *vpt2_entryp = PADDR_TO_PTE(vpt1) | PTE_V ;//| PTE_R;
        }
    }

    /* Step 3: Get the page table entry for `va`, and return it. */
    vpt1_entry = vpt1 + VPN1(va);
//printf("Ret:%lx, value:%lx\n", vpt1_entry, *vpt1_entry);
    return vpt1_entry;

}

// Overview:
// 	Map vpt to vpt.
//	table rooted at pgdir.
void boot_map_vpt(Pte *vpt2, Pte *vpt)
{
	u_int64_t va = (u_int64_t)vpt, pte;
	Pte *vpt1, *vpt1_ent, *vpt0, *vpt0_ent;
	int vpt1_new = 0, vpt0_new = 0;
	//printf("vpt2_ent:0x%lx\n", vpt2+VPN2(va));
	if ((vpt2[VPN2(va)] & PTE_V) == 0) {
		vpt1_new = 1;
		vpt1 = (Pte *)alloc(BY2PG, BY2PG, 1);
		vpt2[VPN2(va)] = PADDR_TO_PTE(vpt1) | PTE_V;
	}
	vpt1 = PTE_TO_PADDR(vpt2[VPN2(va)]);
	//printf("vpt1_ent:0x%lx\n", vpt1+VPN1(va));
	if ((vpt1[VPN1(va)] & PTE_V) == 0) {
		vpt0_new = 1;
		vpt0 = (Pte *)alloc(BY2PG, BY2PG, 1);
		vpt1[VPN1(va)] = PADDR_TO_PTE(vpt0) | PTE_V;
	}
	vpt0 = PTE_TO_PADDR(vpt1[VPN1(va)]);
	//printf("vpt0_ent:0x%lx\n", vpt0+VPN0(va));
	//printf("%d, %d, %lx\n", vpt1_new, vpt0_new, &vpt0[VPN0(va)]);
	vpt0[VPN0(va)] = PADDR_TO_PTE(va) | PTE_V | PTE_R | PTE_W;
	if (vpt1_new) {
		boot_map_vpt(vpt2, vpt1);
	}
	if (vpt0_new) {
		boot_map_vpt(vpt2, vpt0);
	}
}

// Overview:
// 	Map [va, va+size) of virtual address space to physical [pa, pa+size) in the page 
//	table rooted at pgdir. 
//	Use permission bits `perm|PTE_V` for the entries.
// 	Use permission bits `perm` for the entries.
//
// Pre-Condition:
// 	Size is a multiple of BY2PG. 
void boot_map_segment(Pte *vpt2, u_int64_t va, u_int64_t size, u_int64_t pa, u_int64_t perm)
{
    u_int64_t i=0, j=0;
    u_int64_t va_temp = va;
    va = ROUNDDOWN(va, BY2PG);
    Pte *vpt1, *vpt1_entry;
    Pte *vpt0, *vpt0_entry;

    /* Step 1: Check if `size` is a multiple of BY2PG. */
    if (size % BY2PG != 0) {
printf("Map size not 4K aligned!\n");
        return;
    }
//printf("With perm %lx to map: va from %lx to %lx, pa from %lx to %lx\n", perm, va, va + size, pa, pa+size);
    /* Step 2: Map virtual address space to physical address. */

    /* If not aligned to VPT1MAP */
    if (va % VPT1MAP != 0) {
	//printf("NOT ALIGNED TO VPT1MAP\n");
	vpt1_entry = boot_vpt2_walk(vpt2, va, 1);/*We have vpt1 now*/
	//vpt1 = vpt1_entry - (vpt1_entry % BY2PG);
	vpt1 = (u_int64_t)vpt1_entry & 0xFFFFFFFFFFFFF000;
        *(vpt2 + VPN2(va)) |= PTE_V ;//| perm;
	boot_map_vpt(vpt2, vpt1);
	if (size > (VPT1MAP - (va % VPT1MAP))) {
	    for(j = 0; j < (VPT1MAP - (va % VPT1MAP)); j += BY2PG) {
		vpt0_entry = boot_vpt1_walk(vpt1, (va + j), 1);
		vpt0 = (u_int64_t)vpt0_entry & 0xFFFFFFFFFFFFF000;
		boot_map_vpt(vpt2, vpt0);
		*vpt0_entry = PADDR_TO_PTE(pa + j) | perm | PTE_V;
		*(vpt1 + VPN1(va + j)) |= PTE_V ;//| perm;
	    }
	    size -= (VPT1MAP - (va % VPT1MAP));
	    pa += (VPT1MAP - (va % VPT1MAP));
	    va += (VPT1MAP - (va % VPT1MAP));
	}
	else {
	    for(j = 0; j < size; j += BY2PG) {
		vpt0_entry = boot_vpt1_walk(vpt1, (va + j), 1);
		vpt0 = (u_int64_t)vpt0_entry & 0xFFFFFFFFFFFFF000;
		boot_map_vpt(vpt2, vpt0);
		*vpt0_entry = PADDR_TO_PTE(pa + j) | perm | PTE_V;
		*(vpt1 + VPN1(va + j)) |= PTE_V; //| perm;
	    }
	    size = 0;
	    pa += (VPT1MAP - (va % VPT1MAP));
	    va += (VPT1MAP - (va % VPT1MAP));
	}
    }
    /* Now we are aligned to VPT1MAP. */

    for(i = 0; i < size; i += VPT1MAP) {
	//printf("Try to map 1 VPT1MAP\n");
        vpt1_entry = boot_vpt2_walk(vpt2, (va + i), 1);
	//printf("VPT1_entry value:%lx, PADDR:%lx\n", *vpt1_entry, vpt1_entry);
	vpt1 = (u_int64_t)vpt1_entry & 0xFFFFFFFFFFFFF000; //vpt1 = vpt1_entry - (vpt1_entry % BY2PG);
	//printf("vpt1:0x%lx to map!\n", vpt1);
	boot_map_vpt(vpt2, vpt1);
	//printf("vpt1 mapped!\n");
	for(j = 0; j < MIN(size - i, VPT1MAP); j += BY2PG) {
	    vpt0_entry = boot_vpt1_walk(vpt1, (va + i + j), 1);
	    vpt0 = (u_int64_t)vpt0_entry & 0xFFFFFFFFFFFFF000;
	    boot_map_vpt(vpt2, vpt0);
	    *vpt0_entry = PADDR_TO_PTE(pa + i + j) | perm | PTE_V;
	    *(vpt1 + VPN1(va + i + j)) |= PTE_V ;//| perm;
	}
	//*vpt1_entry = PADDR_TO_PTE(vpt0) | perm | PTE_V;
        *(vpt2 + VPN2(va + i)) |= PTE_V ;//| perm;
    }
    Pte *vpt2_entry;
    vpt2_entry = vpt2 + VPN2(va);
    //printf("va:%lx, VPT2_entry value:%lx, PADDR:%lx\n", va, *vpt2_entry, PTE_TO_PADDR(*vpt2_entry));
}

/* Overview:
 * 	Unmap va, set its leaf PTE to invalid.
 */
void boot_unmap(Pte *vpt2, u_int64_t va)
{
//printf("boot_unmap!\n");
	Pte *vpt1, *vpt0;
	u_int64_t pte;
	pte = (u_int64_t)vpt2[VPN2(va)];
	if (pte & PTE_V == 0) {
		return;
	}
	if (((pte & PTE_R) == 0) && ((pte & PTE_W) == 1)) {
		panic("ERR: VPT2 ENTRY ENT NOT READABLE but WRITABLE!!!");
	}

	vpt1 = (Pte *)PTE_TO_PADDR(pte);
	pte = vpt1[VPN1(va)];
	if (pte & PTE_V == 0) {
		return;
	}
	if (((pte & PTE_R) == 0) && ((pte & PTE_W) == 1)) {
		panic("ERR: VPT1 ENTRY ENT NOT READABLE but WRITABLE!!!");
	}
	
	vpt0 = (Pte *)PTE_TO_PADDR(pte);
	pte = vpt0[VPN0(va)];
	if (pte & PTE_V == 0) {
		return;
	} else {
		vpt0[VPN0(va)] = 0;
	}
}


void test_vaddr_map(Pte *vpt2, u_int64_t va, u_int64_t pa);
// Overview:
// 	Set up two-level page table.
// 
// Hint:  You can get more details about `UPAGES` and `UENVS` in include/mmu.h. */
void riscv_vm_init()
{
    extern char end[];
    extern u_int64_t mCONTEXT;
    extern struct Env *envs;
    extern char start_text[], end_text[], start_bss[], end_bss[], start_data[], end_data[], start_kern_stk[], end_kern_stk[];

    Pte *vpt2;
    u_int64_t n;
printf("sizeof u_int64_t:%d, sizeof Pte *:%d, sizeof void *:%d\n", sizeof(u_int64_t), sizeof(Pte *), sizeof(void *));

    /* Step 1: Allocate a page for page directory(first level page table). */

    vpt2 = alloc(BY2PG, BY2PG, 1);
    printf("to memory %lx for struct page directory.\n", freemem);
    mCONTEXT = (u_int64_t)vpt2;
printf("Segments:\n.text:\tfrom:%lxto:%lx\n", start_text, end_text);
printf(".bss:\tfrom:%lxto:%lx\n", start_bss, end_bss);
printf(".data:\tfrom:%lxto:%lx\n", start_data, end_data);
printf(".kern_stk:\tfrom:%lxto:%lx\n", start_kern_stk, end_kern_stk);
    boot_vpt2 = vpt2;
    //boot_map_segment(vpt2, vpt2, BY2PG, vpt2, PTE_R);
    boot_map_vpt(vpt2, vpt2);

    printf(".text need:0x%lx B\n", (u_int64_t)end_text - (u_int64_t)start_text);
    boot_map_segment(vpt2, start_text, (u_int64_t)end_text - (u_int64_t)start_text, start_text, PTE_R | PTE_X);
    printf(".text mapped!\n");

printf(".bss need:0x%lx B\n", (u_int64_t)end_bss - (u_int64_t)start_bss);
    boot_map_segment(vpt2, start_bss, (u_int64_t)end_bss - (u_int64_t)start_bss, start_bss, PTE_R | PTE_W);
    printf(".bss mapped!\n");

printf(".data need:0x%lx B\n", (u_int64_t)end_data - (u_int64_t)start_data);
    boot_map_segment(vpt2, start_data, (u_int64_t)end_data - (u_int64_t)start_data, start_data, PTE_R | PTE_W);
    printf(".data mapped!\n");

    printf(".kern_stk need:0x%lx B\n", (u_int64_t)end_kern_stk - (u_int64_t)start_kern_stk);
    boot_map_segment(vpt2, start_kern_stk, (u_int64_t)end_kern_stk - (u_int64_t)start_kern_stk, start_kern_stk, PTE_R | PTE_W);
    printf(".kern_stk mapped!\n");

    /************* Page directory set, address below are virtual address **************/

    /* Step 2: Allocate proper size of physical memory for global array `pages`,
     * for physical memory management. Then, map virtual address `UPAGES` to
     * physical address `pages` allocated before. For consideration of alignment,
     * you should round up the memory size before map. */
    pages = (struct Page *)alloc(npage * sizeof(struct Page), BY2PG, 1);
    printf("to memory %lx for struct Pages.\n", freemem);
    n = ROUND(npage * sizeof(struct Page), BY2PG);
    boot_map_segment(vpt2, UPAGES, n, pages, PTE_R | PTE_W);
    pages_paddr = pages;
    pages = UPAGES;

    /* Step 3, Allocate proper size of physical memory for global array `envs`,
     * for process management. Then map the physical address to `UENVS`. */
    envs = (struct Env *)alloc(NENV * sizeof(struct Env), BY2PG, 1);
    n = ROUND(NENV * sizeof(struct Env), BY2PG);
    boot_map_segment(vpt2, UENVS, n, envs, PTE_R | PTE_W);
    envs_paddr = envs;
    envs = UENVS;

	u_int64_t tmpva = 0x090000000;
    boot_map_segment(vpt2, tmpva, BY2PG, tmpva, PTE_R | PTE_W);
	printf("TEST:%lx->%lx\n", tmpva, tmpva);
	test_vaddr_map(vpt2, tmpva, tmpva);
    boot_unmap(vpt2, tmpva);
	printf("TEST:%lx->%lx\n", tmpva, tmpva);
	test_vaddr_map(vpt2, tmpva, tmpva);

printf("ready to set vpt at vpt2: %lx, ppn: %lx\n", vpt2, PPN(vpt2));

	printf("TEST:%lx->%lx\n", start_text, start_text);
	test_vaddr_map(vpt2, start_text, start_text);
//	printf("TEST:%lx->%lx\n", start_data, start_data);
//	test_vaddr_map(vpt2, start_data, start_data);
//	u_int64_t testdata = 0x0000000080203886;
//	printf("TEST:%lx->%lx\n", testdata, testdata);
//	test_vaddr_map(vpt2, testdata, testdata);
    /* Set up VPT register. */
    n = set_vpt2(MODE_SV39, 0, PPN(vpt2));

    printf("pmap.c:\t risc-v vm init success\n");
	printf("TEST:%lx->%lx\n", start_text, start_text);
	test_vaddr_map(vpt2, start_text, start_text);
}

// Overview: 
// 	Initialize page structure and memory free list.
// 	The `pages` array has one `struct Page` entry per physical page. Pages 
//	are reference counted, and free pages are kept on a linked list.
// Hint:
//	Use `LIST_INSERT_HEAD` to insert something to list.
void
page_init(void)
{
    /* Step 1: Initialize page_free_list. */
    /* Hint: Use macro `LIST_INIT` defined in include/queue.h. */
printf("Enter page_init!\n");
    LIST_INIT(&page_free_list);
printf("Page_init list_init end!\n");
    /* Step 2: Align `freemem` up to multiple of BY2PG. */
    freemem = ROUND(freemem, BY2PG);
printf("Page_init freemem_round end!\n");
    /* Step 3: Mark all memory blow `freemem` as used(set `pp_ref`
     * filed to 1) */
    int cur;
    for (cur = 0; cur < PPN(PADDR2ACTMEM(freemem)); cur++) {
        pages[cur].pp_ref = 1;
    }
printf("Page_init used pages[] init end!\n");
    /* Step 4: Mark the other memory as free. */
    for (cur = PPN(PADDR2ACTMEM(freemem)); cur < npage; cur++) {
        pages[cur].pp_ref = 0;
        LIST_INSERT_HEAD(&page_free_list, &pages[cur], pp_link);
    }
printf("End of page_init!\n");
}

// Overview:
//	Allocates a physical page from free memory, and clear this page.
// 
// Post-Condition:
//	If failed to allocate a new page(out of memory(there's no free page)), 
// 	return -E_NO_MEM.
//	Else, set the address of allocated page to *pp, and returned 0.
// 
// Note: 
// 	Does NOT increment the reference count of the page - the caller must do 
// 	these if necessary (either explicitly or via page_insert).
//
// Hint:
//	Use LIST_FIRST and LIST_REMOVE defined in include/queue.h .
int
page_alloc(struct Page **pp)
{
    struct Page *ppage_temp;
printf("page_alloc() start!\n");

    /* Step 1: Get a page from free memory. If fails, return the error code.*/
    if (LIST_EMPTY(&page_free_list)) {
//      panic("mei you kong ye le !!!!!!");
        return -E_NO_MEM;
    }
//printf("1");
    ppage_temp = LIST_FIRST(&page_free_list);
//printf("2");
    LIST_REMOVE(ppage_temp, pp_link);
//printf("3");
    *pp = ppage_temp;
//printf("4\n");
    /* Step 2: Initialize this page.
     * Hint: use `bzero`. */
    /* Note: pa == va. */
    void *page_pa, *page_va = 0x090000000;
    page_pa = (void *)page2pa(ppage_temp);
    boot_map_segment(boot_vpt2, page_va, BY2PG, page_pa, PTE_R | PTE_W);
//printf("Map temp addr %lx to paddr %lx succ!\n", page_va, page_pa);
    tlb_invalidate(boot_vpt2, page_va);
	//printf("TEST:%lx->%lx\n", page_va, page_pa);
	//test_vaddr_map(boot_vpt2, page_va, page_pa);
//printf("start:%08lx, end:%08lx\n", page_pa, page_pa+BY2PG);
//u_int64_t *tp = 0x090000008;
//page_pa = *tp;
//*tp = page_pa;
//printf("*tp=%lx\n", page_pa);

    bzero(page_va, BY2PG);
    boot_unmap(boot_vpt2, page_va);
printf("page_alloc() success with page paddr: %lx, struct va:%lx!\n", page2pa(*pp), *pp);
    return 0;

}

// Overview:
//	Release a page, mark it as free if it's `pp_ref` reaches 0.
// Hint:
//	When to free a page, just insert it to the page_free_list.
void
page_free(struct Page *pp)
{
    /* Step 1: If there's still virtual address refers to this page, do nothing. */
    if (pp->pp_ref > 0) {
        return;
    }

    /* Step 2: If the `pp_ref` reaches to 0, mark this page as free and return. */
    if (pp->pp_ref == 0) {
        LIST_INSERT_HEAD(&page_free_list, pp, pp_link);
        return;
    }

    /* If the value of `pp_ref` less than 0, some error must occurred before,
     * so PANIC !!! */
    panic("cgh:pp->pp_ref is less than zero\n");
}

// Overview:
// 	Map vpt to vpt.
//	table rooted at vpt2.
void page_map_vpt(Pte *vpt2, Pte *vpt)
{
printf("vpt map start\n");
	struct Page *ppage;
	u_int64_t va = (u_int64_t)vpt, pte, base;
	Pte *vpt1, *vpt1_ent, *vpt0, *vpt0_ent;
	int vpt1_new = 0, vpt0_new = 0;
	base = VPN2(vpt) << PT2SHIFT;
	//printf("vpt2_ent:0x%lx\n", vpt2+VPN2(va));
	if ((vpt2[VPN2(va)] & PTE_V) == 0) {
printf("vpt1 new!!!!!\n");
		vpt1_new = 1;
		if(page_alloc(&ppage) == -E_NO_MEM) {
			return -E_NO_MEM;
			// No memory.
		}
		vpt1 = page2pa(ppage);
//		page_map_vpt(vpt2, vpt1);
		vpt2[VPN2(va)] = PADDR_TO_PTE(vpt1) | PTE_V;
		tlb_invalidate(vpt2, vpt1);
		
printf("vpt:%lx, base1:%lx\n", vpt, base);
		u_int64_t down = ((u_int64_t)vpt) & 0xffffffffc0000000;
		u_int64_t up = down + 0x40000000;
		if (((VPN1(vpt) << PT1SHIFT + base) <= vpt1) &&(((VPN1(vpt) + 1) << PT1SHIFT + base) > vpt1)) {
			// Self mapping!!!
			printf("Self mapping1!\n");
			Pte *va_tmp = 0x090000000;
			page_insert(vpt2, ppage, va_tmp, PTE_R | PTE_W);
			ppage->pp_ref++;
			*(va_tmp + VPN1(vpt1)) = PADDR_TO_PTE(vpt1) | PTE_V;
			page_remove(vpt2, va_tmp);
			ppage->pp_ref--;
			tlb_invalidate(vpt2, vpt1);
		}
	}
	vpt1 = PTE_TO_PADDR(vpt2[VPN2(va)]);
	if (vpt1_new) {
		page_map_vpt(vpt2, vpt1);
	}
	//printf("vpt1_ent:0x%lx\n", vpt1+VPN1(va));
	if ((vpt1[VPN1(va)] & PTE_V) == 0) {
printf("vpt0 new!!!!!\n");
		vpt0_new = 1;
		if(page_alloc(&ppage) == -E_NO_MEM) {
			printf("NO MEMORY!!\n");
			return -E_NO_MEM;
			// No memory.
		}
		vpt0 = page2pa(ppage);
printf("vpt0 paddr:%lx\n", vpt0);
//		page_map_vpt(vpt2, vpt0);
		vpt1[VPN1(va)] = PADDR_TO_PTE(vpt0) | PTE_V;
		tlb_invalidate(vpt2, vpt0);
		printf("base:%lx\n", base);
		base += VPN1(vpt) << PT1SHIFT;
		u_int64_t down = ((u_int64_t)vpt) & 0xffffffffffe00000;
		u_int64_t up = down + 0x200000;
		if (vpt0 == 0x087fff000) {printf("0x87fff000, range:%lx - %lx\n", down, up);}
		if ((u_int64_t)vpt0 >= down) {printf(">= ");}
		if ((u_int64_t)vpt0 < up) {printf("< ");}
		if ((down <= (u_int64_t)vpt0) && (up > (u_int64_t)vpt0)) {
			// Self mapping!!!
			printf("Self mapping2!\n");
			Pte *va_tmp = 0x090000000;
			page_insert(vpt2, ppage, va_tmp, PTE_R | PTE_W);
			ppage->pp_ref++;
			*(va_tmp + VPN0(vpt0)) = PADDR_TO_PTE(vpt0) | PTE_V | PTE_R | PTE_W;
			printf("write:%lx, addr:%lx, paddr:%lx\n", *(va_tmp + VPN0(vpt0)), (va_tmp + VPN0(vpt0)), vpt0);
			page_remove(vpt2, va_tmp);
			ppage->pp_ref--;
			tlb_invalidate(vpt2, vpt0);
		}
	}
	vpt0 = PTE_TO_PADDR(vpt1[VPN1(va)]);
	if (vpt0_new) {
test_vaddr_map(vpt2, vpt0, vpt0);
		page_map_vpt(vpt2, vpt0);
	}
printf("vpt map1, vpt0:%lx\n", vpt0);
test_vaddr_map(vpt2, vpt0, vpt0);
vpt0[0]=0x10086;
printf("vpt map2, %lx\n",vpt0[0]);
	//printf("vpt0_ent:0x%lx\n", vpt0+VPN0(va));
	//printf("%d, %d, %lx\n", vpt1_new, vpt0_new, &vpt0[VPN0(va)]);
	vpt0[VPN0(va)] = PADDR_TO_PTE(va) | PTE_V | PTE_R | PTE_W;
	tlb_invalidate(vpt2, va);
	/*if (vpt1_new) {
		boot_map_vpt(vpt2, vpt1);
	}
	if (vpt0_new) {
		boot_map_vpt(vpt2, vpt0);
	}*/
printf("vpt map succ\n");
}

// Overview:
// 	Given `pgdir`, a pointer to a page directory, pgdir_walk returns a pointer 
// 	to the page table entry (with permission PTE_R|PTE_V) for virtual address 'va'.
//
// Pre-Condition:
//	The `pgdir` should be two-level page table structure.
//
// Post-Condition:
// 	If we're out of memory, return -E_NO_MEM.
//	Else, we get the page table entry successfully, store the value of page table
//	entry to *ppte, and return 0, indicating success.
// 
// Hint:
//	We use a two-level pointer to store page table entry and return a state code to indicate
//	whether this function execute successfully or not.
//  This function have something in common with function `boot_vpt2_walk`.
int
vpt2_walk(Pte *vpt2, u_int64_t va, int create, Pte **vpt0e)
{
	Pte *vpt2_ent, *vpt1, *vpt1_ent, *vpt0;
	
	Pte *pg_paddr;
	struct Page *ppage;

	vpt2_ent = vpt2 + VPN2(va);
	if ((*vpt2_ent & PTE_V) == 0) { // VPT2 invalid
		*vpt0e = NULL;
		if (create == 1) {
			if(page_alloc(&ppage) == -E_NO_MEM) {
				return -E_NO_MEM;
				// No memory.
			}
			vpt1 = page2pa(ppage); // Note: Physical address of vpt1
			// Reg vpt1 into page table
			page_map_vpt(vpt2, vpt1);
		}
		else {
			return 0;
		}
	}
	vpt1 = (Pte *)PTE_TO_PADDR(*vpt2_ent);
	
	vpt1_ent = vpt1 + VPN1(va);
	if ((*vpt1_ent & PTE_V) == 0) { // VPT1 invalid
		*vpt0e = NULL;
		if (create == 1) {
			if(page_alloc(&ppage) == -E_NO_MEM) {
				return -E_NO_MEM;
				// No memory.
			}
			vpt0 = page2pa(ppage); // Note: Physical address of vpt0
			// Reg vpt0 into page table
			page_map_vpt(vpt2, vpt0);
		}
		else {
			return 0;
		}
	}
	vpt0 = (Pte *)PTE_TO_PADDR(*vpt1_ent);
	
	*vpt0e = vpt0 + VPN0(va);
	return 0;
}

// Overview:
// 	Map the physical page 'pp' at virtual address 'va'.
// 	The permissions (the low 12 bits) of the page table entry should be set to 'perm|PTE_V'.
//
// Post-Condition:
//  Return 0 on success
//  Return -E_NO_MEM, if page table couldn't be allocated
// 
// Hint:
//	If there is already a page mapped at `va`, call page_remove() to release this mapping.
//	The `pp_ref` should be incremented if the insertion succeeds.
int
page_insert(Pte *vpt2, struct Page *pp, u_int64_t va, u_int perm)
{
    u_int PERM;
    Pte *vpt0_entry;
    PERM = perm | PTE_V;

    /* Step 1: Get corresponding page table entry. */
    vpt2_walk(vpt2, va, 0, &vpt0_entry);
//printf("vpt2_walk() complete!:vpt_ent:%lx, paddr:%lx\n", *vpt0_entry, PTE_TO_PADDR(*vpt0_entry));
//printf("To remove : %lx\n", va);
//printf("TEST:%lx->%lx\n", va, va);
//test_vaddr_map(vpt2, va, va);

    if ((vpt0_entry != 0) && ((*vpt0_entry & PTE_V) != 0)) {
        if (pa2page(PTE_TO_PADDR(*vpt0_entry)) != pp) {
//printf("sit1, pa2page:%lx, pp:%lx\n", pa2page(PTE_TO_PADDR(*vpt0_entry)), pp);
            page_remove(vpt2, va);
        } else  {
//printf("sit2\n");
            *vpt0_entry = (PADDR_TO_PTE(page2pa(pp)) | PERM);
	    tlb_invalidate(vpt2, va);
            return 0;
        }
    }
//printf("Validate complete!\n");

    /* Step 2: Update TLB. */

    /* hint: use tlb_invalidate function */
    tlb_invalidate(vpt2, va);

    /* Step 3: Do check, re-get page table entry to validate the insertion. */

    /* Step 3.1 Check if the page can be insert, if can't, return -E_NO_MEM */
    if (vpt2_walk(vpt2, va, 1, &vpt0_entry) == -E_NO_MEM) {
        return -E_NO_MEM;
    }
//printf("walk2 complete!\n");
    /* Step 3.2 Insert page and increment the pp_ref */
    *vpt0_entry = (PADDR_TO_PTE(page2pa(pp)) | PERM);
    tlb_invalidate(vpt2, va);
//printf("refill complete!pp:%lx, pp->ref:%lx\n", pp, &pp->pp_ref);
    pp->pp_ref += 1;
//printf("ref succ\n");
    return 0;
}

// Overview:
//	Look up the Page that virtual address `va` map to.
//
// Post-Condition:
//	Return a pointer to corresponding Page, and store it's page table entry to *ppte.
//	If `va` doesn't mapped to any Page, return NULL.
struct Page *
page_lookup(Pte *vpt2, u_int64_t va, Pte **vpt0e)
{
    struct Page *ppage;
    Pte *pte;

    /* Step 1: Get the page table entry. */
    vpt2_walk(vpt2, va, 0, &pte);

    /* Hint: Check if the page table entry doesn't exist or is not valid. */
    if (pte == NULL) {
        return NULL;
    }
    if ((*pte & PTE_V) == 0) {
        return NULL;    //the page is not in memory.
    }

    /* Step 2: Get the corresponding Page struct. */

    /* Hint: Use function `pa2page`, defined in include/pmap.h . */
//printf("pg lookup:pa:%lx\n", PTE_TO_PADDR(*pte));
    ppage = pa2page(PTE_TO_PADDR(*pte)) ;
//printf("ppage:%lx\n", ppage);
    if (vpt0e) {
        *vpt0e = pte;
    }

    return ppage;
}

// Overview:
// 	Decrease the `pp_ref` value of Page `*pp`, if `pp_ref` reaches to 0, free this page.
void page_decref(struct Page *pp) {
	if(--pp->pp_ref == 0) {
		page_free(pp);
	}
}

// Overview:
// 	Unmaps the physical page at virtual address `va`.
void
page_remove(Pte *vpt2, u_int64_t va)
{
    Pte *vpt0_entry;
    struct Page *ppage;

    /* Step 1: Get the page table entry, and check if the page table entry is valid. */
    ppage = page_lookup(vpt2, va, &vpt0_entry);

    if (ppage == NULL) {
        return;
    }

    /* Step 2: Decrease `pp_ref` and decide if it's necessary to free this page. */

    /* Hint: When there's no virtual address mapped to this page, release it. */
//printf("rm:ref, addr:%lx\n", &ppage->pp_ref);
    ppage->pp_ref--;
//printf("rm:ref ok\n");
    if (ppage->pp_ref == 0) {
        page_free(ppage);
    }

    /* Step 3: Update TLB. */
    *vpt0_entry =  (*vpt0_entry) & (~PTE_V);
    tlb_invalidate(vpt2, va);
    return;
}

// Overview:
// 	Update TLB.
void
tlb_invalidate(Pte *vpt2, u_int64_t va)
{
	//if (curenv) {
	//	tlb_out(PTE_ADDR(va) | GET_ENV_ASID(curenv->env_id));
	//} else {
		tlb_out(va);
	//}
}

void
physical_memory_manage_check(void)
{
    struct Page *pp, *pp0, *pp1, *pp2;
    struct Page_list fl;
    int *temp;
    printf("Start physical_memory_manage_check()\n");

    // should be able to allocate three pages
    pp0 = pp1 = pp2 = 0;
    assert(page_alloc(&pp0) == 0);
    assert(page_alloc(&pp1) == 0);
    assert(page_alloc(&pp2) == 0);

    assert(pp0);
    assert(pp1 && pp1 != pp0);
    assert(pp2 && pp2 != pp1 && pp2 != pp0);

    // temporarily steal the rest of the free pages
    fl = page_free_list;
    // now this page_free list must be empty!!!!
    LIST_INIT(&page_free_list);
    // should be no free memory
    assert(page_alloc(&pp) == -E_NO_MEM);

    //temp = (int*)page2kva(pp0);
    //write 1000 to pp0
    //*temp = 1000;
    // free pp0
    page_free(pp0);
    //printf("The number in address temp is %d\n",*temp);

    // alloc again
    assert(page_alloc(&pp0) == 0);
    assert(pp0);

    // pp0 should not change
    //assert(temp == (int*)page2kva(pp0));
    // pp0 should be zero
    //assert(*temp == 0);

    page_free_list = fl;
    page_free(pp0);
    page_free(pp1);
    page_free(pp2);
    struct Page_list test_free;
    struct Page *test_pages;
printf("mark0\n");
//freemem = freemem - (pages_paddr - pages);
//freemem = ROUND(freemem, BY2PG);
struct Page *tmppage1, *tmppage2;
page_alloc(&tmppage1);
//page_alloc(&tmppage2);
printf("tmppage struct addr : %lx\n", tmppage1);
page_insert(boot_vpt2, tmppage1, page2pa(tmppage1), PTE_R|PTE_W);
//page_insert(boot_vpt2, tmppage2, page2pa(tmppage2), PTE_R|PTE_W);
test_pages = page2pa(tmppage1);
printf("mark1\n");
        //test_pages= (struct Page *)alloc(10 * sizeof(struct Page), BY2PG, 1);
        LIST_INIT(&test_free);
        //LIST_FIRST(&test_free) = &test_pages[0];
        int i,j=0;
        struct Page *p, *q;
        //test inert tail
        for(i=0;i<10;i++) {
                test_pages[i].pp_ref=i;
                //test_pages[i].pp_link=NULL;
                //printf("0x%x  0x%x\n",&test_pages[i], test_pages[i].pp_link.le_next);
                LIST_INSERT_TAIL(&test_free,&test_pages[i],pp_link);
                //printf("0x%x  0x%x\n",&test_pages[i], test_pages[i].pp_link.le_next);

        }
printf("mark2\n");
        p = LIST_FIRST(&test_free);
        int answer1[]={0,1,2,3,4,5,6,7,8,9};
        assert(p!=NULL);
        while(p!=NULL)
        {
                //printf("%d %d\n",p->pp_ref,answer1[j]);
                assert(p->pp_ref==answer1[j++]);
                //printf("ptr: 0x%x v: %d\n",(p->pp_link).le_next,((p->pp_link).le_next)->pp_ref);
                p=LIST_NEXT(p,pp_link);

        }
printf("mark3\n");
        // insert_after test
        int answer2[]={0,1,2,3,4,20,5,6,7,8,9};
        q=(struct Page *)alloc(sizeof(struct Page), BY2PG, 1);
        q->pp_ref = 20;

        //printf("---%d\n",test_pages[4].pp_ref);
        LIST_INSERT_AFTER(&test_pages[4], q, pp_link);
        //printf("---%d\n",LIST_NEXT(&test_pages[4],pp_link)->pp_ref);
        p = LIST_FIRST(&test_free);
        j=0;
        //printf("into test\n");
        while(p!=NULL){
        //      printf("%d %d\n",p->pp_ref,answer2[j]);
                        assert(p->pp_ref==answer2[j++]);
                        p=LIST_NEXT(p,pp_link);
        }



    printf("physical_memory_manage_check() succeeded\n");
}

void
page_check(void)
{
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	LIST_INIT(&page_free_list);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// there is no free memory, so we can't allocate a page table
	assert(page_insert(boot_vpt2, pp1, 0x0, 0) < 0);

	// free pp0 and try again: pp0 should be used for page table
	page_free(pp0);
	assert(page_insert(boot_vpt2, pp1, 0x0, 0) == 0);
	assert(PTE_ADDR(boot_vpt2[0]) == page2pa(pp0));
	assert(va2pa(boot_vpt2, 0x0) == page2pa(pp1));
	assert(pp1->pp_ref == 1);

	// should be able to map pp2 at BY2PG because pp0 is already allocated for page table
	assert(page_insert(boot_vpt2, pp2, BY2PG, 0) == 0);
	assert(va2pa(boot_vpt2, BY2PG) == page2pa(pp2));
	assert(pp2->pp_ref == 1);
	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);
	//printf("why\n");
	// should be able to map pp2 at BY2PG because it's already there
	assert(page_insert(boot_vpt2, pp2, BY2PG, 0) == 0);
	assert(va2pa(boot_vpt2, BY2PG) == page2pa(pp2));
	assert(pp2->pp_ref == 1);
	//printf("It is so unbelivable\n");
	// pp2 should NOT be on the free list
	// could happen in ref counts are handled sloppily in page_insert
	assert(page_alloc(&pp) == -E_NO_MEM);

	// should not be able to map at PDMAP because need free page for page table
	assert(page_insert(boot_vpt2, pp0, PDMAP, 0) < 0);

	// insert pp1 at BY2PG (replacing pp2)
	assert(page_insert(boot_vpt2, pp1, BY2PG, 0) == 0);

	// should have pp1 at both 0 and BY2PG, pp2 nowhere, ...
	assert(va2pa(boot_vpt2, 0x0) == page2pa(pp1));
	assert(va2pa(boot_vpt2, BY2PG) == page2pa(pp1));
	// ... and ref counts should reflect this
	assert(pp1->pp_ref == 2);
	assert(pp2->pp_ref == 0);

	// pp2 should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp2);

	// unmapping pp1 at 0 should keep pp1 at BY2PG
	page_remove(boot_vpt2, 0x0);
	assert(va2pa(boot_vpt2, 0x0) == ~0);
	assert(va2pa(boot_vpt2, BY2PG) == page2pa(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp2->pp_ref == 0);

	// unmapping pp1 at BY2PG should free it
	page_remove(boot_vpt2, BY2PG);
	assert(va2pa(boot_vpt2, 0x0) == ~0);
	assert(va2pa(boot_vpt2, BY2PG) == ~0);
	assert(pp1->pp_ref == 0);
	assert(pp2->pp_ref == 0);

	// so it should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// forcibly take pp0 back
	assert(PTE_ADDR(boot_vpt2[0]) == page2pa(pp0));
	boot_vpt2[0] = 0;
	assert(pp0->pp_ref == 1);
	pp0->pp_ref = 0;

	// give free list back
	page_free_list = fl;

	// free the pages we took
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);

	printf("page_check() succeeded!\n");
}

void pageout(int va, int context)
{
	u_long r;
	struct Page *p = NULL;

	if (context < 0x80000000) {
		panic("tlb refill and alloc error!");
	}

	if ((va > 0x7f400000) && (va < 0x7f800000)) {
		panic(">>>>>>>>>>>>>>>>>>>>>>it's env's zone");
	}

	if (va < 0x10000) {
		panic("^^^^^^TOO LOW^^^^^^^^^");
	}

	if ((r = page_alloc(&p)) < 0) {
		panic ("page alloc error!");
	}

	p->pp_ref++;

	page_insert((Pde *)context, p, VA2PFN(va), PTE_R);
	printf("pageout:\t@@@___0x%x___@@@  ins a page \n", va);
}

void test_vaddr_map(Pte *vpt2, u_int64_t va, u_int64_t pa) {
	printf("TEST: 0x%lx -> 0x%lx\n", va, pa);
	Pte *vpt2_ent, *vpt1, *vpt1_ent, *vpt0, *vpt0_ent;
	u_int64_t i, pte_value;
	printf("PADDR of VPT2:%lx\n", vpt2);
	i = VPN2(va);
	vpt2_ent = vpt2 + i;
	printf("VPT2_ENT ADDR:%lx\n", vpt2_ent);
	pte_value = *vpt2_ent;
	printf("VPT2_ENTRY:%lx\n", pte_value);
	printf("PADDR of VPT1:%lx\n", PTE_TO_PADDR(pte_value));
	if ((pte_value & PTE_V) == 0) {printf("VPTENT invalid!\n");return;}
	if (((pte_value & PTE_R) == 0) && ((pte_value & PTE_W) == 1)) {
		printf("VPT ENT NOT READABLE but WRITABLE");
		return;
	}
	vpt1 = (Pte *)PTE_TO_PADDR(pte_value);
	i = VPN1(va);
	vpt1_ent = vpt1 + i;
	printf("VPT1_ENT ADDR:%lx\n", vpt1_ent);
	pte_value = *vpt1_ent;
	printf("VPT1_ENTRY:%lx\n", pte_value);
	printf("PADDR of VPT0:%lx\n", PTE_TO_PADDR(pte_value));
	if ((pte_value & PTE_V) == 0) {printf("VPTENT invalid!\n");return;}
	if (((pte_value & PTE_R) == 0) && ((pte_value & PTE_W) == 1)) {
		printf("VPT ENT NOT READABLE but WRITABLE");
		return;
	}
	vpt0 = (Pte *)(Pte *)PTE_TO_PADDR(pte_value);
	i = VPN0(va);
	vpt0_ent = vpt0 + i;
	printf("VPT0_ENT ADDR:%lx\n", vpt0_ent);
	pte_value = *vpt0_ent;
	printf("VPT0_ENTRY:%lx\n", pte_value);
	printf("Perm:");
	if ((pte_value & PTE_V)!=0){printf(" Valid ");}
	if ((pte_value & PTE_R)!=0){printf(" Read ");}
	if ((pte_value & PTE_W)!=0){printf(" Write ");}
	if ((pte_value & PTE_X)!=0){printf(" Execute ");}
	if ((pte_value & PTE_U)!=0){printf(" User ");}
	printf("\n");
	if ((pte_value & PTE_V) == 0) {printf("VPTENT invalid!\n");return;}
	if (((pte_value & PTE_R) == 0) && ((pte_value & PTE_W) == 1)) {
		printf("VPT ENT NOT READABLE but WRITABLE");
		return;
	}
	
	va = PTE_TO_PADDR(pte_value);
	pa = pa & ~0xfff;
	printf("va:%lx\npa:%lx\n", va, pa);
	if (va == pa){
		printf("TEST SUCCESS!\n");
	}
	return;
}

void asmprint(u_int64_t x)
{
	printf("arg0 is %lx\n", x);
	return;
}
