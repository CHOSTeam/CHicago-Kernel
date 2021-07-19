/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 12 of 2021, at 14:54 BRT
 * Last edited on July 19 of 2021, at 09:25 BRT */

#include <arch/acpi.hxx>
#include <arch/mm.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

/* All the job is done by the generic vmm.cxx implementation, we just need our special arch macros. */

#define MMU_TYPE UInt64
#define MMU_SHOOTDOWN(Address, Size) Smp::SendTlbShootdown(Address, Size)
#define MMU_UPDATE(Address) asm volatile("invlpg (%0)" :: "r"(Address) : "memory")

#define MMU_IS_HUGE(Entry) ((Entry) & PAGE_HUGE)
#define MMU_IS_USER(Entry) ((Entry) & PAGE_USER)
#define MMU_IS_WRITE(Entry) ((Entry) & PAGE_WRITE)
#define MMU_IS_EXEC(Entry) (!((Entry) & PAGE_NO_EXEC))
#define MMU_IS_PRESENT(Entry) ((Entry) & PAGE_PRESENT)

#define MMU_BASE_FLAGS PAGE_PRESENT
#define MMU_HUGE_FLAG(Flag) ((Flag) ? PAGE_HUGE : 0)
#define MMU_USER_FLAG(Flag) ((Flag) ? PAGE_USER : 0)
#define MMU_WRITE_FLAG(Flag) ((Flag) ? PAGE_WRITE : 0)
#define MMU_EXEC_FLAG(Flag) ((Flag) ? 0 : PAGE_NO_EXEC)

#define MMU_UNSET_PRESENT(Entry) ((Entry) &= ~PAGE_PRESENT)

#ifdef __i386__
#define HEAP_END 0xFF800000
#define MMU_SKIP_LEVEL(Level) (!(Level))
#define MMU_DEST_LEVEL(Huge) ((Huge) ? 1 : 2)
#define MMU_ENTRY_MASK(Level) ((Level) == 2 ? PAGE_MASK : HUGE_PAGE_MASK)
#define MMU_ENTRY_SIZE(Level) ((Level) == 1 ? HUGE_PAGE_SIZE : PAGE_SIZE)
#define MMU_INDEX(Virtual, Level) ((Level) == 1 ? 0xFFFFC000 + (((Virtual) >> 21) << 3) : \
                                                  0xFF800000 + (((Virtual) >> 12) << 3))
#define MMU_MAKE_TABLE(Virtual, Physical, Level) ((Physical) | PAGE_PRESENT | PAGE_WRITE | \
                                                  ((Virtual) >= 0x80000000 ? PAGE_USER : 0))
#else
#define HEAP_END 0xFFFFFF8000000000
#define MMU_SKIP_LEVEL(Level) False
#define MMU_DEST_LEVEL(Huge) ((Huge) ? 2 : 3)
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
