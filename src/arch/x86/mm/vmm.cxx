/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 12 of 2021, at 14:54 BRT
 * Last edited on July 15 of 2021, at 23:42 BRT */

#include <arch/mm.hxx>
#include <sys/mm.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

/* All the job is done by the generic vmm.cxx implementation, we just need our special arch macros. */

#define MMU_TYPE UIntPtr
#define MMU_UPDATE(Address) asm volatile("invlpg (%0)" :: "r"(Address) : "memory")

#define MMU_IS_HUGE(Entry) ((Entry) & PAGE_HUGE)
#define MMU_IS_USER(Entry) ((Entry) & PAGE_USER)
#define MMU_IS_WRITE(Entry) ((Entry) & PAGE_WRITE)
#define MMU_IS_PRESENT(Entry) ((Entry) & PAGE_PRESENT)

#define MMU_BASE_FLAGS PAGE_PRESENT
#define MMU_HUGE_FLAG(Flag) ((Flag) ? PAGE_HUGE : 0)
#define MMU_USER_FLAG(Flag) ((Flag) ? PAGE_USER : 0)
#define MMU_WRITE_FLAG(Flag) ((Flag) ? PAGE_WRITE : 0)

#define MMU_UNSET_PRESENT(Entry) ((Entry) &= ~PAGE_PRESENT)

#ifdef __i386__
#define HEAP_END 0xFFC00000
#define MMU_EXEC_FLAG(Flag) 0
#define MMU_IS_EXEC(Entry) True
#define MMU_DEST_LEVEL(Huge) (!(Huge))
#define MMU_ENTRY_MASK(Level) ((Level) ? PAGE_MASK : HUGE_PAGE_MASK)
#define MMU_ENTRY_SIZE(Level) ((Level) ? PAGE_SIZE : HUGE_PAGE_SIZE)
#define MMU_INDEX(Virtual, Level) (!(Level) ? 0xFFFFF000 + ((((Virtual) >> 22) & 0x3FF) << 2) : \
                                              0xFFC00000 + ((((Virtual) >> 12) & 0xFFFFF) << 2))
#define MMU_MAKE_TABLE(Virtual, Physical, Level) ((Physical) | PAGE_PRESENT | PAGE_WRITE | \
                                                  ((Virtual) >= 0x80000000 ? PAGE_USER : 0))
#else
#define HEAP_END 0xFFFFFF8000000000
#define MMU_DEST_LEVEL(Huge) ((Huge) ? 2 : 3)
#define MMU_IS_EXEC(Entry) (!((Entry) & PAGE_NO_EXEC))
#define MMU_EXEC_FLAG(Flag) ((Flag) ? 0 : PAGE_NO_EXEC)
#define MMU_ENTRY_MASK(Level) ((Level) == 2 ? HUGE_PAGE_MASK : PAGE_MASK)
#define MMU_ENTRY_SIZE(Level) (!(Level) ? 0x8000000000 : ((Level) == 1 ? 0x40000000 : \
                                                         ((Level) == 2 ? HUGE_PAGE_SIZE : PAGE_SIZE)))
#define MMU_INDEX(Virtual, Level) (!(Level) ? 0xFFFFFFFFFFFFF000 + ((((Virtual) >> 39) & 0x1FF) << 3) : \
                                   ((Level) == 1 ? 0xFFFFFFFFFFE00000 + ((((Virtual) >> 30) & 0x3FFFF) << 3) : \
                                   ((Level) == 2 ? 0xFFFFFFFFC0000000 + ((((Virtual) >> 21) & 0x7FFFFFF) << 3) : \
                                                   0xFFFFFF8000000000 + ((((Virtual) >> 12) & 0xFFFFFFFFF) << 3))))
#define MMU_MAKE_TABLE(Virtual, Physical, Level) ((Physical) | PAGE_PRESENT | PAGE_WRITE | \
                                                  ((Virtual) >= 0xFFFF800000000000 ? PAGE_USER : 0))
#endif

#include "../../vmm.cxx"

/*Void VirtMem::Initialize(BootInfo &Info) {
    UIntPtr start = (Info.KernelEnd + HUGE_PAGE_MASK) & ~HUGE_PAGE_MASK;

    Unmap(Info.EfiTempAddress & ~PAGE_MASK, PAGE_SIZE);

#ifdef __i386__
    for (UIntPtr i = start & ~HUGE_PAGE_MASK; i < HEAP_END; i += HUGE_PAGE_SIZE) {
#else
    for (UIntPtr i = start & ~0x7FFFFFFFFF; i < HEAP_END; i += 0x8000000000) {
#endif
        UInt64 phys;
        UIntPtr *ent;
        UInt8 lvl = 1;

        if (CheckDirectory(i, ent, lvl) != -1 || lvl != 1) continue;
        ASSERT(PhysMem::Reference(0, 1, phys) == Status::Success);

        lvl++;
        *ent = phys | PAGE_PRESENT | PAGE_WRITE;
        CheckDirectory(i, ent, lvl, True);
    }

    Start = Current = (start + VIRT_GROUP_RANGE - 1) & ~(VIRT_GROUP_RANGE - 1);
    End = HEAP_END & ~(VIRT_GROUP_RANGE - 1);

    Debug.Write("the kernel virtual address allocator starts at 0x{:0*:16} and ends at 0x{:0*:16}\n", Start, End);
}*/
