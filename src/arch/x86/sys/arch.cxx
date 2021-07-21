/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:47 BRT
 * Last edited on July 21 of 2021, at 19:55 BRT */

#include <arch/acpi.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

Gdt BspGdt {};

static Void Handler(Registers&) { Arch::Halt(True); }

Void Acpi::InitializeArch(const BootInfo &Info) {
    /* APIC (LAPIC and IOAPICs) -> HPET -> SMP (HPET depends on the IOAPIC, SMP depends on both APIC and HPET). */

    UIntPtr size1, size2;
    auto madt = reinterpret_cast<const Apic::Madt*>(GetHeader("APIC", size1));
    auto hpet = reinterpret_cast<const Hpet::Header*>(GetHeader("HPET", size2));

    ASSERT(madt != Null && hpet != Null);
    Apic::Initialize(madt);
    Hpet::Initialize(hpet);
    Smp::Initialize(Info, madt);

    VirtMem::Unmap(reinterpret_cast<UIntPtr>(madt) & ~PAGE_MASK, size1);
    VirtMem::Unmap(reinterpret_cast<UIntPtr>(hpet) & ~PAGE_MASK, size2);
    VirtMem::Free(reinterpret_cast<UIntPtr>(madt) & ~PAGE_MASK, size1 >> PAGE_SHIFT);
    VirtMem::Free(reinterpret_cast<UIntPtr>(hpet) & ~PAGE_MASK, size2 >> PAGE_SHIFT);
}

Void Arch::Initialize(const BootInfo&) {
    /* Initialize the descriptor tables (GDT and IDT, as they are required for pretty much everything, including
     * gracefully handling exceptions/errors). */

    BspGdt.Initialize();
    Debug.Write("{}initialized the global descriptor table{}\n", SetForeground { 0xFF00FF00 }, RestoreForeground{});

    IdtInit();
    IdtSetHandler(0xDE, Handler);
    Debug.Write("{}initialized the interrupt descriptor table{}\n", SetForeground { 0xFF00FF00 }, RestoreForeground{});
}

Void Arch::EnterPanicState(Void) {
    /* Halt all the other processors but ourselves (as we got a kernel panic/fatal error). */

    static SpinLock lock;
    lock.Acquire();
    Smp::SendIpi(2, 0, 0xFE);
}

no_return Void Arch::Halt(Boolean Full) {
    if (Full) while (True) asm volatile("cli; hlt");
    else while (True) asm volatile("hlt");
}
