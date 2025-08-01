/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017 Red Hat Inc
 *
 * Authors:
 *  David Hildenbrand <david@redhat.com>
 */
#ifndef _ASMS390X_ARCH_DEF_H_
#define _ASMS390X_ARCH_DEF_H_

#include <util.h>

struct stack_frame {
	struct stack_frame *back_chain;
	uint64_t reserved;
	/* GRs 2 - 5 */
	uint64_t argument_area[4];
	/* GRs 6 - 15 */
	uint64_t grs[10];
	/* FPRs 0, 2, 4, 6 */
	int64_t  fprs[4];
};

struct stack_frame_int {
	struct stack_frame *back_chain;
	uint64_t reserved;
	/*
	 * The GRs are offset compatible with struct stack_frame so we
	 * can easily fetch GR14 for backtraces.
	 */
	/* GRs 2 - 15 */
	uint64_t grs0[14];
	/* GRs 0 and 1 */
	uint64_t grs1[2];
	uint32_t reserved1;
	uint32_t fpc;
	uint64_t fprs[16];
	uint64_t crs[16];
};

struct psw {
	union {
		uint64_t	mask;
		struct {
			uint64_t reserved00:1;
			uint64_t per:1;
			uint64_t reserved02:3;
			uint64_t dat:1;
			uint64_t io:1;
			uint64_t ext:1;
			uint64_t key:4;
			uint64_t reserved12:1;
			uint64_t mchk:1;
			uint64_t wait:1;
			uint64_t pstate:1;
			uint64_t as:2;
			uint64_t cc:2;
			uint64_t prg_mask:4;
			uint64_t reserved24:7;
			uint64_t ea:1;
			uint64_t ba:1;
			uint64_t reserved33:31;
		};
	};
	uint64_t	addr;
};
static_assert(sizeof(struct psw) == 16);

#define PSW(m, a) ((struct psw){ .mask = (m), .addr = (uint64_t)(a) })

struct short_psw {
	uint32_t	mask;
	uint32_t	addr;
};

struct cpu {
	struct lowcore *lowcore;
	uint64_t *stack;
	void (*pgm_cleanup_func)(struct stack_frame_int *);
	void (*ext_cleanup_func)(struct stack_frame_int *);
	uint16_t addr;
	uint16_t idx;
	bool active;
	bool pgm_int_expected;
	bool ext_int_expected;
	bool in_interrupt_handler;
};

enum address_space {
	AS_PRIM = 0,
	AS_ACCR = 1,
	AS_SECN = 2,
	AS_HOME = 3
};

#define PSW_MASK_DAT			0x0400000000000000UL
#define PSW_MASK_HOME			0x0000C00000000000UL
#define PSW_MASK_IO			0x0200000000000000UL
#define PSW_MASK_EXT			0x0100000000000000UL
#define PSW_MASK_KEY			0x00F0000000000000UL
#define PSW_MASK_WAIT			0x0002000000000000UL
#define PSW_MASK_PSTATE			0x0001000000000000UL
#define PSW_MASK_EA			0x0000000100000000UL
#define PSW_MASK_BA			0x0000000080000000UL
#define PSW_MASK_64			(PSW_MASK_BA | PSW_MASK_EA)

#define CTL0_TRANSACT_EX_CTL			(63 -  8)
#define CTL0_LOW_ADDR_PROT			(63 - 35)
#define CTL0_EDAT				(63 - 40)
#define CTL0_FETCH_PROTECTION_OVERRIDE		(63 - 38)
#define CTL0_STORAGE_PROTECTION_OVERRIDE	(63 - 39)
#define CTL0_IEP				(63 - 43)
#define CTL0_AFP				(63 - 45)
#define CTL0_VECTOR				(63 - 46)
#define CTL0_EMERGENCY_SIGNAL			(63 - 49)
#define CTL0_EXTERNAL_CALL			(63 - 50)
#define CTL0_CLOCK_COMPARATOR			(63 - 52)
#define CTL0_CPU_TIMER				(63 - 53)
#define CTL0_SERVICE_SIGNAL			(63 - 54)
#define CR0_EXTM_MASK			0x0000000000006200UL /* Combined external masks */

#define CTL2_GUARDED_STORAGE		(63 - 59)

#define LC_SIZE	(2 * PAGE_SIZE)
struct lowcore {
	uint8_t		pad_0x0000[0x0080 - 0x0000];	/* 0x0000 */
	uint32_t	ext_int_param;			/* 0x0080 */
	uint16_t	cpu_addr;			/* 0x0084 */
	uint16_t	ext_int_code;			/* 0x0086 */
	uint16_t	svc_int_id;			/* 0x0088 */
	uint16_t	svc_int_code;			/* 0x008a */
	uint16_t	pgm_int_id;			/* 0x008c */
	uint16_t	pgm_int_code;			/* 0x008e */
	uint32_t	dxc_vxc;			/* 0x0090 */
	uint16_t	mon_class_nb;			/* 0x0094 */
	uint8_t		per_code;			/* 0x0096 */
	uint8_t		per_atmid;			/* 0x0097 */
	uint64_t	per_addr;			/* 0x0098 */
	uint8_t		exc_acc_id;			/* 0x00a0 */
	uint8_t		per_acc_id;			/* 0x00a1 */
	uint8_t		op_acc_id;			/* 0x00a2 */
	uint8_t		arch_mode_id;			/* 0x00a3 */
	uint8_t		pad_0x00a4[0x00a8 - 0x00a4];	/* 0x00a4 */
	uint64_t	trans_exc_id;			/* 0x00a8 */
	uint64_t	mon_code;			/* 0x00b0 */
	uint32_t	subsys_id_word;			/* 0x00b8 */
	uint32_t	io_int_param;			/* 0x00bc */
	uint32_t	io_int_word;			/* 0x00c0 */
	uint8_t		pad_0x00c4[0x00c8 - 0x00c4];	/* 0x00c4 */
	uint32_t	stfl;				/* 0x00c8 */
	uint8_t		pad_0x00cc[0x00e8 - 0x00cc];	/* 0x00cc */
	uint64_t	mcck_int_code;			/* 0x00e8 */
	uint8_t		pad_0x00f0[0x00f4 - 0x00f0];	/* 0x00f0 */
	uint32_t	ext_damage_code;		/* 0x00f4 */
	uint64_t	failing_storage_addr;		/* 0x00f8 */
	uint64_t	emon_ca_origin;			/* 0x0100 */
	uint32_t	emon_ca_size;			/* 0x0108 */
	uint32_t	emon_exc_count;			/* 0x010c */
	uint64_t	breaking_event_addr;		/* 0x0110 */
	uint8_t		pad_0x0118[0x0120 - 0x0118];	/* 0x0118 */
	struct psw	restart_old_psw;		/* 0x0120 */
	struct psw	ext_old_psw;			/* 0x0130 */
	struct psw	svc_old_psw;			/* 0x0140 */
	struct psw	pgm_old_psw;			/* 0x0150 */
	struct psw	mcck_old_psw;			/* 0x0160 */
	struct psw	io_old_psw;			/* 0x0170 */
	uint8_t		pad_0x0180[0x01a0 - 0x0180];	/* 0x0180 */
	struct psw	restart_new_psw;		/* 0x01a0 */
	struct psw	ext_new_psw;			/* 0x01b0 */
	struct psw	svc_new_psw;			/* 0x01c0 */
	struct psw	pgm_new_psw;			/* 0x01d0 */
	struct psw	mcck_new_psw;			/* 0x01e0 */
	struct psw	io_new_psw;			/* 0x01f0 */
	/* sw definition: save area for registers in interrupt handlers */
	uint64_t	sw_int_grs[16];			/* 0x0200 */
	uint8_t		pad_0x0280[0x0308 - 0x0280];	/* 0x0280 */
	uint64_t	sw_int_crs[16];			/* 0x0308 */
	struct psw	sw_int_psw;			/* 0x0388 */
	struct cpu	*this_cpu;			/* 0x0398 */
	uint8_t		pad_0x03a0[0x11b0 - 0x03a0];	/* 0x03a0 */
	uint64_t	mcck_ext_sa_addr;		/* 0x11b0 */
	uint8_t		pad_0x11b8[0x1200 - 0x11b8];	/* 0x11b8 */
	uint64_t	fprs_sa[16];			/* 0x1200 */
	uint64_t	grs_sa[16];			/* 0x1280 */
	struct psw	psw_sa;				/* 0x1300 */
	uint8_t		pad_0x1310[0x1318 - 0x1310];	/* 0x1310 */
	uint32_t	prefix_sa;			/* 0x1318 */
	uint32_t	fpc_sa;				/* 0x131c */
	uint8_t		pad_0x1320[0x1324 - 0x1320];	/* 0x1320 */
	uint32_t	tod_pr_sa;			/* 0x1324 */
	uint64_t	cputm_sa;			/* 0x1328 */
	uint64_t	cc_sa;				/* 0x1330 */
	uint8_t		pad_0x1338[0x1340 - 0x1338];	/* 0x1338 */
	uint32_t	ars_sa[16];			/* 0x1340 */
	uint64_t	crs_sa[16];			/* 0x1380 */
	uint8_t		pad_0x1400[0x1800 - 0x1400];	/* 0x1400 */
	uint8_t		pgm_int_tdb[0x1900 - 0x1800];	/* 0x1800 */
} __attribute__ ((__packed__));
static_assert(sizeof(struct lowcore) == 0x1900);

extern struct lowcore lowcore;

#define THIS_CPU (lowcore.this_cpu)

#define PGM_INT_CODE_OPERATION			0x01
#define PGM_INT_CODE_PRIVILEGED_OPERATION	0x02
#define PGM_INT_CODE_EXECUTE			0x03
#define PGM_INT_CODE_PROTECTION			0x04
#define PGM_INT_CODE_ADDRESSING			0x05
#define PGM_INT_CODE_SPECIFICATION		0x06
#define PGM_INT_CODE_DATA			0x07
#define PGM_INT_CODE_FIXED_POINT_OVERFLOW	0x08
#define PGM_INT_CODE_FIXED_POINT_DIVIDE		0x09
#define PGM_INT_CODE_DECIMAL_OVERFLOW		0x0a
#define PGM_INT_CODE_DECIMAL_DIVIDE		0x0b
#define PGM_INT_CODE_HFP_EXPONENT_OVERFLOW	0x0c
#define PGM_INT_CODE_HFP_EXPONENT_UNDERFLOW	0x0d
#define PGM_INT_CODE_HFP_SIGNIFICANCE		0x0e
#define PGM_INT_CODE_HFP_DIVIDE			0x0f
#define PGM_INT_CODE_SEGMENT_TRANSLATION	0x10
#define PGM_INT_CODE_PAGE_TRANSLATION		0x11
#define PGM_INT_CODE_TRANSLATION_SPEC		0x12
#define PGM_INT_CODE_SPECIAL_OPERATION		0x13
#define PGM_INT_CODE_OPERAND			0x15
#define PGM_INT_CODE_TRACE_TABLE		0x16
#define PGM_INT_CODE_VECTOR_PROCESSING		0x1b
#define PGM_INT_CODE_SPACE_SWITCH_EVENT		0x1c
#define PGM_INT_CODE_HFP_SQUARE_ROOT		0x1d
#define PGM_INT_CODE_PC_TRANSLATION_SPEC	0x1f
#define PGM_INT_CODE_AFX_TRANSLATION		0x20
#define PGM_INT_CODE_ASX_TRANSLATION		0x21
#define PGM_INT_CODE_LX_TRANSLATION		0x22
#define PGM_INT_CODE_EX_TRANSLATION		0x23
#define PGM_INT_CODE_PRIMARY_AUTHORITY		0x24
#define PGM_INT_CODE_SECONDARY_AUTHORITY	0x25
#define PGM_INT_CODE_LFX_TRANSLATION		0x26
#define PGM_INT_CODE_LSX_TRANSLATION		0x27
#define PGM_INT_CODE_ALET_SPECIFICATION		0x28
#define PGM_INT_CODE_ALEN_TRANSLATION		0x29
#define PGM_INT_CODE_ALE_SEQUENCE		0x2a
#define PGM_INT_CODE_ASTE_VALIDITY		0x2b
#define PGM_INT_CODE_ASTE_SEQUENCE		0x2c
#define PGM_INT_CODE_EXTENDED_AUTHORITY		0x2d
#define PGM_INT_CODE_LSTE_SEQUENCE		0x2e
#define PGM_INT_CODE_ASTE_INSTANCE		0x2f
#define PGM_INT_CODE_STACK_FULL			0x30
#define PGM_INT_CODE_STACK_EMPTY		0x31
#define PGM_INT_CODE_STACK_SPECIFICATION	0x32
#define PGM_INT_CODE_STACK_TYPE			0x33
#define PGM_INT_CODE_STACK_OPERATION		0x34
#define PGM_INT_CODE_ASCE_TYPE			0x38
#define PGM_INT_CODE_REGION_FIRST_TRANS		0x39
#define PGM_INT_CODE_REGION_SECOND_TRANS	0x3a
#define PGM_INT_CODE_REGION_THIRD_TRANS		0x3b
#define PGM_INT_CODE_SECURE_STOR_ACCESS		0x3d
#define PGM_INT_CODE_NON_SECURE_STOR_ACCESS	0x3e
#define PGM_INT_CODE_SECURE_STOR_VIOLATION	0x3f
#define PGM_INT_CODE_MONITOR_EVENT		0x40
#define PGM_INT_CODE_PER			0x80
#define PGM_INT_CODE_CRYPTO_OPERATION		0x119
#define PGM_INT_CODE_TX_ABORTED_EVENT		0x200

struct cpuid {
	uint64_t version : 8;
	uint64_t id : 24;
	uint64_t type : 16;
	uint64_t format : 1;
	uint64_t reserved : 15;
};

#define SVC_LEAVE_PSTATE 1

static inline unsigned short stap(void)
{
	unsigned short cpu_address;

	asm volatile("stap %0" : "=Q" (cpu_address));
	return cpu_address;
}

static inline uint64_t stidp(void)
{
	uint64_t cpuid;

	asm volatile("stidp %0" : "=Q" (cpuid));

	return cpuid;
}

enum tprot_permission {
	TPROT_READ_WRITE = 0,
	TPROT_READ = 1,
	TPROT_RW_PROTECTED = 2,
	TPROT_TRANSL_UNAVAIL = 3,
};

static inline enum tprot_permission tprot(unsigned long addr, char access_key)
{
	int cc;

	asm volatile(
		"	tprot	0(%1),0(%2)\n"
		"	ipm	%0\n"
		"	srl	%0,28\n"
		: "=d" (cc) : "a" (addr), "a" (access_key << 4) : "cc");
	return (enum tprot_permission)cc;
}

static inline void lctlg(int cr, uint64_t value)
{
	asm volatile(
		"	lctlg	%1,%1,%0\n"
		: : "Q" (value), "i" (cr));
}

static inline uint64_t stctg(int cr)
{
	uint64_t value;

	asm volatile(
		"	stctg	%1,%1,%0\n"
		: "=Q" (value) : "i" (cr) : "memory");
	return value;
}

static inline void ctl_set_bit(int cr, unsigned int bit)
{
        uint64_t reg;

	reg = stctg(cr);
	reg |= 1UL << bit;
	lctlg(cr, reg);
}

static inline void ctl_clear_bit(int cr, unsigned int bit)
{
        uint64_t reg;

	reg = stctg(cr);
	reg &= ~(1UL << bit);
	lctlg(cr, reg);
}

static inline uint64_t extract_psw_mask(void)
{
	uint32_t mask_upper = 0, mask_lower = 0;

	asm volatile(
		"	epsw	%0,%1\n"
		: "=r" (mask_upper), "=a" (mask_lower));

	return (uint64_t) mask_upper << 32 | mask_lower;
}

#define PSW_WITH_CUR_MASK(addr) PSW(extract_psw_mask(), (addr))

static inline void load_psw_mask(uint64_t mask)
{
	struct psw psw = {
		.mask = mask,
		.addr = 0,
	};
	uint64_t tmp = 0;

	asm volatile(
		"	larl	%0,0f\n"
		"	stg	%0,8(%1)\n"
		"	lpswe	0(%1)\n"
		"0:\n"
		: "+r" (tmp) :  "a" (&psw) : "memory", "cc" );
}

static inline void disabled_wait(uint64_t message)
{
	struct psw psw = {
		.mask = PSW_MASK_WAIT,  /* Disabled wait */
		.addr = message,
	};

	asm volatile("  lpswe 0(%0)\n" : : "a" (&psw) : "memory", "cc");
}

/**
 * psw_mask_clear_bits - clears bits from the current PSW mask
 * @clear: bitmask of bits that will be cleared
 */
static inline void psw_mask_clear_bits(uint64_t clear)
{
	load_psw_mask(extract_psw_mask() & ~clear);
}

/**
 * psw_mask_set_bits - sets bits on the current PSW mask
 * @set: bitmask of bits that will be set
 */
static inline void psw_mask_set_bits(uint64_t set)
{
	load_psw_mask(extract_psw_mask() | set);
}

/**
 * psw_mask_clear_and_set_bits - clears and sets bits on the current PSW mask
 * @clear: bitmask of bits that will be cleared
 * @set: bitmask of bits that will be set
 *
 * The bits in the @clear mask will be cleared, then the bits in the @set mask
 * will be set.
 */
static inline void psw_mask_clear_and_set_bits(uint64_t clear, uint64_t set)
{
	load_psw_mask((extract_psw_mask() & ~clear) | set);
}

/**
 * enable_dat - enable the DAT bit in the current PSW
 */
static inline void enable_dat(void)
{
	psw_mask_set_bits(PSW_MASK_DAT);
}

/**
 * disable_dat - disable the DAT bit in the current PSW
 */
static inline void disable_dat(void)
{
	psw_mask_clear_bits(PSW_MASK_DAT);
}

static inline void wait_for_interrupt(uint64_t irq_mask)
{
	uint64_t psw_mask = extract_psw_mask();

	load_psw_mask(psw_mask | irq_mask | PSW_MASK_WAIT);
	/*
	 * After being woken and having processed the interrupt, let's restore
	 * the PSW mask.
	 */
	load_psw_mask(psw_mask);
}

static inline void enter_pstate(void)
{
	psw_mask_set_bits(PSW_MASK_PSTATE);
}

static inline void leave_pstate(void)
{
	asm volatile("	svc %0\n" : : "i" (SVC_LEAVE_PSTATE));
}

static inline int stsi(void *addr, int fc, int sel1, int sel2)
{
	register int r0 asm("0") = (fc << 28) | sel1;
	register int r1 asm("1") = sel2;
	int cc;

	asm volatile(
		"stsi	0(%3)\n"
		"ipm	%[cc]\n"
		"srl	%[cc],28\n"
		: "+d" (r0), [cc] "=d" (cc)
		: "d" (r1), "a" (addr)
		: "cc", "memory");
	return cc;
}

static inline unsigned long stsi_get_fc(void)
{
	register unsigned long r0 asm("0") = 0;
	register unsigned long r1 asm("1") = 0;
	int cc;

	asm volatile("stsi	0\n"
		     "ipm	%[cc]\n"
		     "srl	%[cc],28\n"
		     : "+d" (r0), [cc] "=d" (cc)
		     : "d" (r1)
		     : "cc", "memory");
	assert(!cc);
	return r0 >> 28;
}

static inline int servc(uint32_t command, unsigned long sccb)
{
	int cc;

	asm volatile(
		"       .insn   rre,0xb2200000,%1,%2\n"  /* servc %1,%2 */
		"       ipm     %0\n"
		"       srl     %0,28"
		: "=&d" (cc) : "d" (command), "a" (sccb)
		: "cc", "memory");
	return cc;
}

static inline void set_prefix(uint32_t new_prefix)
{
	asm volatile("	spx %0" : : "Q" (new_prefix) : "memory");
}

static inline uint32_t get_prefix(void)
{
	uint32_t current_prefix;

	asm volatile("	stpx %0" : "=Q" (current_prefix));
	return current_prefix;
}

static inline void diag44(void)
{
	asm volatile("diag	0,0,0x44\n");
}

static inline void diag500(uint64_t val)
{
	asm volatile(
		"lgr	2,%[val]\n"
		"diag	0,0,0x500\n"
		:
		: [val] "d"(val)
		: "r2"
	);
}

#endif
