/*
 * arch/openrisc/kernel/vdso.c
 *
 * derived from the sh vsyscall.c file.
 *
 *  Copyright (C) 2006 Paul Mundt
 *
 * vDSO randomization
 * Copyright(C) 2005-2006, Red Hat, Inc., Ingo Molnar
 *
 * Copyright(C) 2015 Ben Payne
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <linux/elf.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/binfmts.h>

//#define DUMMY_VDSO

/*
 * Should the kernel map a VDSO page into processes and pass its
 * address down to glibc upon exec()?
 */
unsigned int __read_mostly vdso_enabled = 1;
EXPORT_SYMBOL_GPL(vdso_enabled);

static int __init vdso_setup(char *s)
{
	vdso_enabled = simple_strtoul(s, NULL, 0);
	return 1;
}
__setup("vdso=", vdso_setup);

/*
 * These symbols are defined by vsyscall.o to mark the bounds
 * of the ELF DSO images included therein.
 */
#ifndef DUMMY_VDSO
extern const char vdso_start, vdso_end;
extern const char __vvar_page;
#endif
static struct page *vdso_pages[1];
static struct page *vvar_pages[1];
unsigned long vdso_size = 0;

int __init vdso_init(void)
{
	printk( "vdso_start 0x%lx-0x%lx, size %d\n", (unsigned long)&vdso_start, (unsigned long)&vdso_end, &vdso_end - &vdso_start );
	vdso_pages[0] = virt_to_page(&vdso_start);
	vvar_pages[0] = virt_to_page(&__vvar_page);
	get_page(vdso_pages[0]);
	get_page(vvar_pages[0]);
	vdso_size = &vdso_end - &vdso_start;
	printk( "vvar start 0x%lx\n", (unsigned long)&__vvar_page );
	return 0;
}

/* Setup a VMA at program startup for the vsyscall page */
int arch_setup_additional_pages(struct linux_binprm *bprm, int uses_interp)
{
	struct mm_struct *mm = current->mm;
	unsigned long addr;
	int ret = 0;

	current->mm->context.vdso = 0;

	printk( "Arch Setup\n" );
	down_write(&mm->mmap_sem);
	addr = get_unmapped_area(NULL, 0, PAGE_SIZE+vdso_size, 0, 0);
	if (IS_ERR_VALUE(addr)) {
		ret = addr;
		printk("get_unmapped_area failed\n");
		goto up_fail;
	}

	ret = install_special_mapping(mm, addr, PAGE_SIZE,
				      VM_READ | VM_MAYREAD,
				      vvar_pages);
	if (unlikely(ret)) {
		printk("install_special_mapping vvar failed\n");
		goto up_fail;
	}

	ret = install_special_mapping(mm, addr+PAGE_SIZE, vdso_size,
				      VM_READ | VM_EXEC |
				      VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC,
				      vdso_pages);
	if (unlikely(ret)) {
		printk("install_special_mapping vdso failed\n");
		goto up_fail;
	}
	
	current->mm->context.vdso = (void *)addr+PAGE_SIZE;
	printk("setup vDSO page at %p size %ld\n", (void*)addr, vdso_size);
up_fail:
	up_write(&mm->mmap_sem);
	return ret;
}

const char *arch_vma_name(struct vm_area_struct *vma)
{
	if (vma->vm_mm && vma->vm_start == (long)vma->vm_mm->context.vdso)
		return "[vdso]";

	return NULL;
}
