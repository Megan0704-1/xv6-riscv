#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/swap.h>
#include <linux/pid.h>
#include <linux/pgtable.h>
#include <linux/swapops.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Megan Kuo");
MODULE_DESCRIPTION("Memory Management");

#define MEMFORMATSTR "[CSE330-Memory-Manager] PID [%d]: virtual address [%llx] physical address [%llx] swap identifier [NA]"
#define SWAPFORMATSTR "[CSE330-Memory-Manager] PID [%d]: virtual address [%llx] physical address [NA] swap identifier [%llx]"
#define INVALIDFORMATSTR "[CSE330-Memory-Manager] PID [%d]: virtual address [%llx] physical address [NA] swap identifier [NA]"

typedef unsigned long long ull;

static int pid=0;
module_param(pid, int, 0);
MODULE_PARM_DESC(pid, "PID of a process.");

static ull addr=0;
module_param(addr, ullong, 0);
MODULE_PARM_DESC(addr, "a virtual address of the process.");

static int __init logical_addr_translation(void) {
    struct task_struct *task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if(!task) {
        pr_err("See if its loading.\n");
        return 0;
    }
    get_task_struct(task);
    struct mm_struct *mm = task->mm; // memory map
    if(!mm) {
        pr_err("Invalid task struct memory map.\n");
        put_task_struct(task);
        return 0;
    }

    // address translation start
    {
        pgd_t *pgd;
        p4d_t *p4d;
        pmd_t *pmd;
        pud_t *pud;
        pte_t *pte;

        pgd = pgd_offset(mm, addr);
        if(pgd_none(*pgd) || unlikely(pgd_bad(*pgd))) {
            // pgd_bad checks certain bits in the PGD pointer to determine if the entry is corrupted or invalid.
            pr_info(INVALIDFORMATSTR, pid, addr);
            put_task_struct(task);
            return 0;
        }

        p4d = p4d_offset(pgd, addr);
        if(p4d_none(*p4d) || unlikely(p4d_bad(*p4d))) {
            // p4d_bad checks certain bits in the P4D pointer to determine if the entry is corrupted or invalid.
            pr_info(INVALIDFORMATSTR, pid, addr);
            put_task_struct(task);
            return 0;
        }

        pud = pud_offset(p4d, addr);
        if(pud_none(*pud) || unlikely(pud_bad(*pud))) {
            // pud_bad checks certain bits in the PUD pointer to determine if the entry is corrupted or invalid.
            pr_info(INVALIDFORMATSTR, pid, addr);
            put_task_struct(task);
            return 0;
        }

        pmd = pmd_offset(pud, addr);
        if(pmd_none(*pmd) || unlikely(pmd_bad(*pmd))) {
            // pmd_bad checks certain bits in the PMD pointer to determine if the entry is corrupted or invalid.
            pr_info(INVALIDFORMATSTR, pid, addr);
            put_task_struct(task);
            return 0;
        }

        pte = pte_offset_kernel(pmd, addr);
        if(!pte) {
            pr_info(INVALIDFORMATSTR, pid, addr);
            put_task_struct(task);
            return 0;
        }

        if(pte_present(*pte)) {
            // if address is in main memory
            ull pfn = pte_pfn(*pte);
            ull offset = addr & (PAGE_SIZE-1); // extracts the lower bits of the logical address.
            ull physicalAddr = (pfn << PAGE_SHIFT) | offset;
            pr_info(MEMFORMATSTR, pid, addr, physicalAddr);
            put_task_struct(task);
            return 0;
        }

        if(pte_none(*pte)) {
            // not present in memory and not swapped
            pr_info(INVALIDFORMATSTR, pid, addr);
            put_task_struct(task);
            return 0;
        }

        // swapped
        swp_entry_t entry = pte_to_swp_entry(*pte);
        ull swapID = entry.val;
        pr_info(SWAPFORMATSTR, pid, addr, swapID);
    }

    put_task_struct(task);
    return 0;
}

static void __exit exit_logical_addr_translation(void) {
    pr_info("Leaving address translation module.\n");
}

module_init(logical_addr_translation);
module_exit(exit_logical_addr_translation);
