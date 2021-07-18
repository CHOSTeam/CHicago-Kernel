/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:47 BRT
 * Last edited on July 17 of 2021, at 23:12 BRT */

#include <arch/acpi.hxx>
#include <arch/desctables.hxx>
#include <sys/arch.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

extern "C" Void SmpTrampoline(Void);
extern "C" CoreInfo *SmpTrampolineCoreInfo;

Gdt BspGdt {};

Void Arch::InitializeCore(Void) {
    /* Information about the current core is stored (at the moment) in the address 0x8000 + the offset into the saved
     * core info struct, we need to set the status flag in it to True (to indicate that this core has been initialized
     * successfully, and that we can go into initializing the next core). */

    auto &info = **reinterpret_cast<CoreInfo**>(0x8000 + reinterpret_cast<UIntPtr>(&SmpTrampolineCoreInfo) -
                                                         reinterpret_cast<UIntPtr>(SmpTrampoline));
    info.Status = True;

    /* Initialize the FPU (SSE+AVX2) support. */

    UInt16 cw0 = 0x37E, cw1 = 0x37A;
    asm volatile("fninit; fldcw %0; fldcw %1\n"
#ifdef __i386__
                 "xor %%ecx, %%ecx; xgetbv; or $0x07, %%eax; xsetbv\n"
#else
                 "xor %%rcx, %%rcx; xgetbv; or $0x07, %%rax; xsetbv\n"
#endif
                 :: "m"(cw0), "m"(cw1)
#ifdef __i386__
                  : "%eax", "%ecx", "%edx");
#else
                  : "%rax", "%rcx", "%rdx");
#endif

    /* The IDT struct can be used as is, but the GDT needs to be per core (because of the TSS). */

    info.Gdt->Initialize();

#ifdef __i386__
    info.Gdt->LoadSpecialSegment(True, reinterpret_cast<UIntPtr>(&info));
#else
    WriteMsr(0xC0000100, reinterpret_cast<UIntPtr>(&info));
#endif

    IdtReload();
    Smp::SetupLApic();
    asm volatile("sti");
}

Void Arch::Initialize(const BootInfo&) {
    /* Initialize the descriptor tables (GDT and IDT, as they are required for pretty much everything, including
     * gracefully handling exceptions/errors). */

    BspGdt.Initialize();
    Debug.Write("{}initialized the global descriptor table{}\n", SetForeground { 0xFF00FF00 }, RestoreForeground{});

    IdtInit();
    Debug.Write("{}initialized the interrupt descriptor table{}\n", SetForeground { 0xFF00FF00 }, RestoreForeground{});
}

UIntPtr Arch::GetCoreId(Void) {
    return Smp::GetCoreList().GetLength() <= 1 ? 0 : Smp::GetCurrentCore().Id;
}

Void Arch::EnterPanicState(Void) {
    /* Halt all the other processors but ourselves (as we got a kernel panic/fatal error). */
}

no_return Void Arch::Halt(Boolean Full) {
    if (Full) while (True) asm volatile("cli; hlt");
    else while (True) asm volatile("hlt");
}

Void Arch::Sleep(TimeUnit Unit, UInt64 Count) {
    /* HPET has a precision on the femtoseconds range, so we don't really need to worry about it sleeping too little
     * or too much. */

    for (UInt64 dest = Hpet::GetRegister(0xF0) + Count *
                       (Unit == TimeUnit::Nanoseconds ? 1000000 : (Unit == TimeUnit::Microseconds ? 1000000000 :
                       (Unit == TimeUnit::Milliseconds ? 1000000000000 : 1000000000000000))) / Hpet::GetFrequency();
         Hpet::GetRegister(0xF0) < dest;) asm volatile("pause" ::: "memory");
}

UInt64 Arch::GetUpTime(TimeUnit Unit) {
    return (Hpet::GetRegister(0xF0) * Hpet::GetFrequency()) /
           (Unit == TimeUnit::Nanoseconds ? 1000000 : (Unit == TimeUnit::Microseconds ? 1000000000 :
           (Unit == TimeUnit::Milliseconds ? 1000000000000 : 1000000000000000)));
}
