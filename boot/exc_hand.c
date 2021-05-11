#include <print.h>
#include <printf.h>
#include <types.h>
#include <mmu.h>

void exc_handler(void)
{
	printf("Exception!!!\n");
	printf("Ni zai yan wo!!!\n");
	panic("Exception!!!!!!!!!!!!!");
}
