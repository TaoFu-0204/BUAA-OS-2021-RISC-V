#include <asm/regdef.h>
#include <asm/asm.h>

/*
 * This function is to set `satp` register to its proper value.
 * 
 * In RV64, `satp` structure is as follow:
 * 
 * |63  60|59          44|43                        0|
 * ---------------------------------------------------
 * | MODE |     ASID     |            PPN            |
 * ---------------------------------------------------
 * 
 * See RISC_V privileged manual for detailed info.
 *
 * u_int64_t set_vpt2(u_int64 a0, u_int64 a1, u_int64 a2);
 * a0 stands for MODE, a1 stands for ASID, a2 stands for PPN.
 * 
 * Return former value of `satp`.
 */
LEAF(set_vpt2)
	slli	a0, a0, 60

	li 	t0, 0x0FFFF00000000000
	slli	a1, a1, 44
	and	a1, a1, t0

	li	t0, 0x00000FFFFFFFFFFF
	and	a2, a2, t0
	
	or	a0, a0, a1
	or	a0, a0, a2

	//call	asmprint
	
//	li	t0, 0xFFFFFFFFFFFFFFFF
	csrrw	a0, satp, a0
	//call	asmprint
	sfence.vma
	//call	asmprint
	//mv	a0, ra	
	//call	asmprint
	jalr	zero, 0(ra)
	//nop
END(set_vpt2)

LEAF(set_exc_vec)
	li 	t0, 0xFFFFFFFFFFFFFFFC
	and	a0, a0, t0

	li	t0, 0x0000000000000003
	and	a1, a1, t0
	
	or	a0, a0, a1

	csrrw	a0, stvec, a0
//	sfence.vma

	jalr	zero, 0(ra)
	//nop
END(set_exc_vec)
