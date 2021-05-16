#include <print.h>
#include <printf.h>
#include <types.h>
#include <mmu.h>

void exc_handler(u_int64_t cause, u_int64_t badvaddr, u_int64_t epc)
{
	printf("Exception!!!\n");
	printf("Cause:%ld, Bad vaddr:0x%lx, epc:0x%lx!!!\n", cause, badvaddr, epc);
	panic("Exception!!!!!!!!!!!!!");
}
