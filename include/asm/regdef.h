/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995 by Ralf Baechle
 */

#ifndef __ASM_RISCV_REGDEF_H
#define __ASM_RISCV_REGDEF_H

/*
 * Symbolic register names for 32 bit ABI
 */
#define zero    x0      /* wired zero */
#define ra      x1      /* return address */
#define sp      x2      /* stack pointer */
#define gp      x3      /* global pointer */
#define tp      x4      /* thread pointer */
#define t0      x5
#define t1      x6
#define t2      x7
#define s0      x8      /* saved register */
#define fp	x8      /* frame pointer */
#define s1      x9
#define a0      x10     /* function argument, return value */
#define a1      x11     /* function argument, return value */
#define a2      x12     /* function argument */
#define a3      x13
#define a4      x14
#define a5      x15
#define a6      x16
#define a7      x17
#define s2      x18     /* saved register */
#define s3      x19
#define s4      x20
#define s5      x21
#define s6      x22
#define s7      x23
#define s8      x24
#define s9      x25
/*#define jp      x25      PIC jump register  */
#define s10      x26     /* kernel scratch */
#define s11      x27
#define t3      x28
#define t4      x29
#define t5      x30
#define t6      x31

#endif /* __ASM_RISCV_REGDEF_H */
