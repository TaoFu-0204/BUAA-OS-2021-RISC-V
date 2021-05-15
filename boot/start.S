/*
 * Copyright (C) 2001 MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <asm/regdef.h>
#include <asm/asm.h>

.data
            .global mCONTEXT
mCONTEXT:
            .quad 0

            .global delay
delay:
            .quad 0

            .global tlbra
tlbra:
            .quad 0


            .section .data.stk
KERNEL_STACK:
            .space 0x8000

.section .text.start_mos
LEAF(_start_mos)

    	/* Set up stack */
	la	sp, KERNEL_STACK
	li	t0, 0x08000
	add	sp, sp, t0
	la	t1, start_exc_vec
	//li	t1, 0x80204000
	csrrw	t1, stvec, t1
	//sfence.vma
	//li	t0, 0x80600000
	//la	t1, mCONTEXT
	//sd	t0, 0(t1)

    /* jump to main */
	tail	main

loop:
    j    loop
    nop
END(_start_mos)

	.section .text.exc_vec
NESTED(except_vec, 0, sp)
//        .set noreorder
//        1:
//DEBUG
//addi sp, sp, -4
//jal DEBUG_exc_mark
//nop
//addi sp, sp, 4
	call	exc_handler
/*        mfc0 k1,CP0_CAUSE
        la k0,exception_handlers
        andi k1,0x7c
        addu k0,k1
        lw k0,(k0)
        NOP
        jr k0
        nop*/
END(except_vec)
/*
.section .data
	.align 12
	.global boot_page_table_sv39
boot_page_table_sv39:
	.zero 8 * 511
	.quad (0x80000 << 10) | 0xcf*/
