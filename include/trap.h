#ifndef _TRAP_H_
#define _TRAP_H_

#define T_INSTADDRMISALN	0    /* instruction addr misaligned */
#define T_INSTACSFAULT		1    /* instruction access fault */
#define T_ILLINST		2    /* illegal instruction */
#define T_BRKPT			3    /* breakpoint */
#define T_LDADDRMISALN		4    /* load addr misaligned */
#define T_LDACSFAULT		5    /* load access fault */
#define T_STADDRMISALN		6    /* store addr misaligned */
#define T_STACSFAULT		7    /* store access fault */
#define T_ECALLFROMU		8    /* ECALL from U-mode */
#define T_ECALLFROMS		9    /* ECALL from S-mode */
/* 10 is reserved */
#define T_ECALLFROMM		11    /* ECALL from M-mode */
#define T_INSTPGFLT		12    /* instruction page fault */
#define T_LDPGFLT		13    /* load page fault */
/* 14 is reserved */
#define T_STPGFLT		15    /* store page fault */

/* These are arbitrarily chosen, but with care not to overlap
 * processor defined exceptions or interrupt vectors.
 */
//#define T_SYSCALL   0x30 /* system call */
//#define T_DEFAULT   500  /* catchall */

#define REGLEN_RISC_V	8 /* Register length in RISC-V, 8 Byte for RV64 */

#ifndef __ASSEMBLER__

#include <types.h>

struct Trapframe { //lr:need to be modified(reference to linux pt_regs) TODO
	/* Saved main processor registers. */
	u_int64_t regs[32];

	/* Saved special registers. */
	u_int64_t sstatus;
	u_int64_t cause;
	u_int64_t tval;
	u_int64_t epc;
	u_int64_t pc;
};
void *set_except_vector(int n, void *addr);
void trap_init();

#endif /* !__ASSEMBLER__ */
/*
 * Stack layout for all exceptions:
 *
 * ptrace needs to have all regs on the stack. If the order here is changed,
 * it needs to be updated in include/asm-mips/ptrace.h
 *
 * The first PTRSIZE*5 bytes are argument save space for C subroutines.
 */

#define TF_REG0		0
#define TF_REG1		((TF_REG0) + REGLEN_RISC_V)
#define TF_REG2		((TF_REG1) + REGLEN_RISC_V)
#define TF_REG3		((TF_REG2) + REGLEN_RISC_V)
#define TF_REG4		((TF_REG3) + REGLEN_RISC_V)
#define TF_REG5		((TF_REG4) + REGLEN_RISC_V)
#define TF_REG6		((TF_REG5) + REGLEN_RISC_V)
#define TF_REG7		((TF_REG6) + REGLEN_RISC_V)
#define TF_REG8		((TF_REG7) + REGLEN_RISC_V)
#define TF_REG9		((TF_REG8) + REGLEN_RISC_V)
#define TF_REG10	((TF_REG9) + REGLEN_RISC_V)
#define TF_REG11	((TF_REG10) + REGLEN_RISC_V)
#define TF_REG12	((TF_REG11) + REGLEN_RISC_V)
#define TF_REG13	((TF_REG12) + REGLEN_RISC_V)
#define TF_REG14	((TF_REG13) + REGLEN_RISC_V)
#define TF_REG15	((TF_REG14) + REGLEN_RISC_V)
#define TF_REG16	((TF_REG15) + REGLEN_RISC_V)
#define TF_REG17	((TF_REG16) + REGLEN_RISC_V)
#define TF_REG18	((TF_REG17) + REGLEN_RISC_V)
#define TF_REG19	((TF_REG18) + REGLEN_RISC_V)
#define TF_REG20	((TF_REG19) + REGLEN_RISC_V)
#define TF_REG21	((TF_REG20) + REGLEN_RISC_V)
#define TF_REG22	((TF_REG21) + REGLEN_RISC_V)
#define TF_REG23	((TF_REG22) + REGLEN_RISC_V)
#define TF_REG24	((TF_REG23) + REGLEN_RISC_V)
#define TF_REG25	((TF_REG24) + REGLEN_RISC_V)
#define TF_REG26	((TF_REG25) + REGLEN_RISC_V)
#define TF_REG27	((TF_REG26) + REGLEN_RISC_V)
#define TF_REG28	((TF_REG27) + REGLEN_RISC_V)
#define TF_REG29	((TF_REG28) + REGLEN_RISC_V)
#define TF_REG30	((TF_REG29) + REGLEN_RISC_V)
#define TF_REG31	((TF_REG30) + REGLEN_RISC_V)

#define TF_STATUS	((TF_REG31) + REGLEN_RISC_V)
#define TF_CAUSE	((TF_STATUS) + REGLEN_RISC_V)
#define TF_TVAL		((TF_CAUSE) + REGLEN_RISC_V)
#define TF_EPC		((TF_TVAL) + REGLEN_RISC_V)
#define TF_PC		((TF_EPC) + REGLEN_RISC_V)
/*
 * Size of stack frame, word/double word alignment
 */
#define TF_SIZE		((TF_PC) + REGLEN_RISC_V)
#endif /* _TRAP_H_ */
