#include <linux/module.h>
#include <linux/smp.h>
#include <linux/user.h>
#include <linux/elfcore.h>
#include <linux/sched.h>
#include <linux/in6.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/pci.h>
#include <linux/irq.h>
#include <asm/sections.h>
#include <asm/semaphore.h>
#include <asm/processor.h>
#include <asm/uaccess.h>
#include <asm/checksum.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>

extern int dump_fpu(struct pt_regs *, elf_fpregset_t *);
extern struct hw_interrupt_type no_irq_type;

EXPORT_SYMBOL(sh_mv);

/* platform dependent support */
EXPORT_SYMBOL(dump_fpu);
EXPORT_SYMBOL(kernel_thread);
EXPORT_SYMBOL(no_irq_type);

EXPORT_SYMBOL(strlen);

/* PCI exports */
#ifdef CONFIG_PCI
EXPORT_SYMBOL(pci_alloc_consistent);
EXPORT_SYMBOL(pci_free_consistent);
#endif

/* mem exports */
EXPORT_SYMBOL(memchr);
EXPORT_SYMBOL(memcpy);
EXPORT_SYMBOL(memset);
EXPORT_SYMBOL(memmove);
EXPORT_SYMBOL(__copy_user);

#ifdef CONFIG_FLATMEM
#include <linux/bootmem.h>
EXPORT_SYMBOL(min_low_pfn);     /* defined by bootmem.c, but not exported by gen
eric code */
EXPORT_SYMBOL(max_low_pfn);     /* defined by bootmem.c, but not exported by gen
eric code */
#endif

#ifdef CONFIG_MMU
EXPORT_SYMBOL(get_vm_area);
#endif

/* semaphore exports */
EXPORT_SYMBOL(__up);
EXPORT_SYMBOL(__down);
EXPORT_SYMBOL(__down_interruptible);
EXPORT_SYMBOL(__down_trylock);

EXPORT_SYMBOL(__udelay);
EXPORT_SYMBOL(__ndelay);
EXPORT_SYMBOL(__const_udelay);

#define DECLARE_EXPORT(name) extern void name(void);EXPORT_SYMBOL(name)

/* These symbols are generated by the compiler itself */
DECLARE_EXPORT(__udivsi3);
DECLARE_EXPORT(__sdivsi3);
DECLARE_EXPORT(__ashrsi3);
DECLARE_EXPORT(__ashlsi3);
DECLARE_EXPORT(__ashrdi3);
DECLARE_EXPORT(__ashldi3);
DECLARE_EXPORT(__ashiftrt_r4_6);
DECLARE_EXPORT(__ashiftrt_r4_7);
DECLARE_EXPORT(__ashiftrt_r4_8);
DECLARE_EXPORT(__ashiftrt_r4_9);
DECLARE_EXPORT(__ashiftrt_r4_10);
DECLARE_EXPORT(__ashiftrt_r4_11);
DECLARE_EXPORT(__ashiftrt_r4_12);
DECLARE_EXPORT(__ashiftrt_r4_13);
DECLARE_EXPORT(__ashiftrt_r4_14);
DECLARE_EXPORT(__ashiftrt_r4_15);
DECLARE_EXPORT(__ashiftrt_r4_20);
DECLARE_EXPORT(__ashiftrt_r4_21);
DECLARE_EXPORT(__ashiftrt_r4_22);
DECLARE_EXPORT(__ashiftrt_r4_23);
DECLARE_EXPORT(__ashiftrt_r4_24);
DECLARE_EXPORT(__ashiftrt_r4_27);
DECLARE_EXPORT(__ashiftrt_r4_30);
DECLARE_EXPORT(__lshrsi3);
DECLARE_EXPORT(__lshrdi3);
DECLARE_EXPORT(__movstrSI8);
DECLARE_EXPORT(__movstrSI12);
DECLARE_EXPORT(__movstrSI16);
DECLARE_EXPORT(__movstrSI20);
DECLARE_EXPORT(__movstrSI24);
DECLARE_EXPORT(__movstrSI28);
DECLARE_EXPORT(__movstrSI32);
DECLARE_EXPORT(__movstrSI36);
DECLARE_EXPORT(__movstrSI40);
DECLARE_EXPORT(__movstrSI44);
DECLARE_EXPORT(__movstrSI48);
DECLARE_EXPORT(__movstrSI52);
DECLARE_EXPORT(__movstrSI56);
DECLARE_EXPORT(__movstrSI60);
#if __GNUC__ == 4
DECLARE_EXPORT(__movmem);
#else
DECLARE_EXPORT(__movstr);
#endif

#ifdef CONFIG_CPU_SH4
#if __GNUC__ == 4
DECLARE_EXPORT(__movmem_i4_even);
DECLARE_EXPORT(__movmem_i4_odd);
DECLARE_EXPORT(__movmemSI12_i4);
/*
 * GCC 4.2 emits these for division, as do GCC 4.1.x versions of the ST
 * compiler which include backported patches.
 */
DECLARE_EXPORT(__sdivsi3_i4i);
DECLARE_EXPORT(__udiv_qrnnd_16);
DECLARE_EXPORT(__udivsi3_i4i);
#else /* GCC 3.x */
DECLARE_EXPORT(__movstr_i4_even);
DECLARE_EXPORT(__movstr_i4_odd);
DECLARE_EXPORT(__movmem_i4_even); /* movstr became movmem in gcc-4.1 */
DECLARE_EXPORT(__movmem_i4_odd);
DECLARE_EXPORT(__movstrSI12_i4);
#endif /* __GNUC__ == 4 */
#endif

#if !defined(CONFIG_CACHE_OFF) && (defined(CONFIG_CPU_SH4) || \
	defined(CONFIG_SH7705_CACHE_32KB))
/* needed by some modules */
EXPORT_SYMBOL(flush_cache_all);
EXPORT_SYMBOL(flush_cache_range);
EXPORT_SYMBOL(flush_cache_page);
EXPORT_SYMBOL(flush_dcache_page);
EXPORT_SYMBOL(__flush_wback_region);
EXPORT_SYMBOL(__flush_purge_region);
EXPORT_SYMBOL(__flush_invalidate_region);
#endif

#if !defined(CONFIG_CACHE_OFF) && defined(CONFIG_MMU) && \
	(defined(CONFIG_CPU_SH4) || defined(CONFIG_SH7705_CACHE_32KB))
EXPORT_SYMBOL(clear_user_page);
EXPORT_SYMBOL(copy_user_page);
#endif

EXPORT_SYMBOL(csum_partial);
EXPORT_SYMBOL(csum_partial_copy_generic);
#ifdef CONFIG_IPV6
EXPORT_SYMBOL(csum_ipv6_magic);
#endif
EXPORT_SYMBOL(clear_page);
EXPORT_SYMBOL(__clear_user);
EXPORT_SYMBOL(_ebss);

/* system call loopback symbols used by Havana */
asmlinkage long sys_getpid(void);
EXPORT_SYMBOL(sys_getpid);
asmlinkage long sys_sched_setscheduler(pid_t pid, int policy,
				       struct sched_param __user *param);
EXPORT_SYMBOL(sys_sched_setscheduler);
