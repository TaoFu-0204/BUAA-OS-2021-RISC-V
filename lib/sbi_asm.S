#include <asm/regdef.h>
#include <asm/asm.h>

LEAF(sys_ecall)
	mv 	a7, a0
	mv	a0, a1
	mv	a1, a2
	mv	a2, a3
	ecall
	jr ra
END(sys_ecall)
