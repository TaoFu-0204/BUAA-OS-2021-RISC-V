#include "../drivers/gxconsole/dev_cons.h"
#include <mmu.h>
#include <env.h>
#include <printf.h>
#include <pmap.h>
#include <sched.h>
#include <types.h>
#include <sbilib_mos.h>

//extern u_int64_t sys_ecall();

void sbi_console_putchar(unsigned char ch) {
	sys_ecall(SBI_CONSOLE_PUTCHAR, ch, 0, 0);
}
