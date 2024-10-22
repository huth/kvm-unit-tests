// SPDX-License-Identifier: GPL-2.0-only
#include <libcflat.h>
#include <cpumask.h>
#include <limits.h>
#include <asm/sbi.h>
#include <asm/setup.h>

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
			unsigned long arg1, unsigned long arg2,
			unsigned long arg3, unsigned long arg4,
			unsigned long arg5)
{
	register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0);
	register uintptr_t a1 asm ("a1") = (uintptr_t)(arg1);
	register uintptr_t a2 asm ("a2") = (uintptr_t)(arg2);
	register uintptr_t a3 asm ("a3") = (uintptr_t)(arg3);
	register uintptr_t a4 asm ("a4") = (uintptr_t)(arg4);
	register uintptr_t a5 asm ("a5") = (uintptr_t)(arg5);
	register uintptr_t a6 asm ("a6") = (uintptr_t)(fid);
	register uintptr_t a7 asm ("a7") = (uintptr_t)(ext);
	struct sbiret ret;

	asm volatile (
		"ecall"
		: "+r" (a0), "+r" (a1)
		: "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
		: "memory");
	ret.error = a0;
	ret.value = a1;

	return ret;
}

void sbi_shutdown(void)
{
	sbi_ecall(SBI_EXT_SRST, 0, 0, 0, 0, 0, 0, 0);
	puts("SBI shutdown failed!\n");
}

struct sbiret sbi_hart_start(unsigned long hartid, unsigned long entry, unsigned long sp)
{
	return sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_START, hartid, entry, sp, 0, 0, 0);
}

struct sbiret sbi_hart_stop(void)
{
	return sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_STOP, 0, 0, 0, 0, 0, 0);
}

struct sbiret sbi_hart_get_status(unsigned long hartid)
{
	return sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_STATUS, hartid, 0, 0, 0, 0, 0);
}

struct sbiret sbi_send_ipi(unsigned long hart_mask, unsigned long hart_mask_base)
{
	return sbi_ecall(SBI_EXT_IPI, SBI_EXT_IPI_SEND_IPI, hart_mask, hart_mask_base, 0, 0, 0, 0);
}

struct sbiret sbi_send_ipi_cpu(int cpu)
{
	return sbi_send_ipi(1UL, cpus[cpu].hartid);
}

struct sbiret sbi_send_ipi_cpumask(const cpumask_t *mask)
{
	struct sbiret ret;
	cpumask_t tmp;

	if (cpumask_full(mask))
		return sbi_send_ipi(0, -1UL);

	cpumask_copy(&tmp, mask);

	while (!cpumask_empty(&tmp)) {
		unsigned long base = ULONG_MAX;
		unsigned long mask = 0;
		int cpu;

		for_each_cpu(cpu, &tmp) {
			if (base > cpus[cpu].hartid)
				base = cpus[cpu].hartid;
		}

		for_each_cpu(cpu, &tmp) {
			if (cpus[cpu].hartid < base + BITS_PER_LONG) {
				mask |= 1UL << (cpus[cpu].hartid - base);
				cpumask_clear_cpu(cpu, &tmp);
			}
		}

		ret = sbi_send_ipi(mask, base);
		if (ret.error)
			break;
	}

	return ret;
}

struct sbiret sbi_set_timer(unsigned long stime_value)
{
	return sbi_ecall(SBI_EXT_TIME, SBI_EXT_TIME_SET_TIMER, stime_value, 0, 0, 0, 0, 0);
}

long sbi_probe(int ext)
{
	struct sbiret ret;

	ret = sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_GET_SPEC_VERSION, 0, 0, 0, 0, 0, 0);
	assert(!ret.error && ret.value >= 2);

	ret = sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_PROBE_EXT, ext, 0, 0, 0, 0, 0);
	assert(!ret.error);

	return ret.value;
}
