#include <asm/asm.h>
//#include <pmap.h>
//#include <env.h>
#include <printf.h>
//#include <kclock.h>
//#include <trap.h>
#include <types.h>
//#include <mmu.h>

extern char aoutcode[];
extern char boutcode[];

void riscv_init()
{
	printf("init.c:\tmips_init() is called\n");
	u_int64_t a,b,c;
	//a = 0x10086;
	//b = 10086;
	//printf("a = %x, b = %d\n", a,b);
	riscv_detect_memory();
	printf("mem dect success!\n");
	riscv_vm_init();
	a = 0x7f800000;
	printf("UPAGES:%lx, ", a);
	printf("value:%lx\n", *((u_int64_t *)a));
	page_init();
	
	//env_init();
	
	//ENV_CREATE(user_fktest);
	//ENV_CREATE(user_pingpong);
	
	//trap_init();
	//kclock_init();

	
	//while(1);
	panic("init.c:\tend of mips_init() reached!");
}

void bcopy(const void *src, void *dst, size_t len)
{
	void *max;

	max = dst + len;
	// copy machine words while possible
	while (dst + 3 < max)
	{
		*(int *)dst = *(int *)src;
		dst+=4;
		src+=4;
	}
	// finish remaining 0-3 bytes
	while (dst < max)
	{
		*(char *)dst = *(char *)src;
		dst+=1;
		src+=1;
	}
}

void bzero(void *b, size_t len)
{
	void *max;

	max = b + len;

	//printf("init.c:\tzero from %lx to %lx\n",(u_int64_t)b,(u_int64_t)max);
	
	// zero machine words while possible

	while ((u_int64_t)(b + 3) < (u_int64_t)max)
	{
		*(int *)b = 0;
		b+=4;
	}
	
	// finish remaining 0-3 bytes
	while ((u_int64_t)b < (u_int64_t)max)
	{
		*(char *)b++ = 0;
	}		
	
}
