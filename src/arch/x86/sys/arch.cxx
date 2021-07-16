/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:47 BRT
 * Last edited on July 16 of 2021, at 13:25 BRT */

#include <arch/desctables.hxx>
#include <sys/arch.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

UInt16 Acpi::CoreCount = 0;
UInt64 Acpi::HpetFrequency = 0;
UInt8 Acpi::LApicIds[2048] { 0 };
UIntPtr Acpi::HpetAddress = 0, Acpi::LApicAddress = 0, Acpi::IoApicAddress = 0;

Void Acpi::InitializeArch(const SdtHeader *Header) {
    if (Header == Null) {
        /* Null header is so that we can check if everything is need is here (if all the tables existed). */
        ASSERT(HpetAddress && LApicAddress && IoApicAddress);
    } else if (CompareMemory(Header->Signature, "APIC", 4)) {
        auto madt = reinterpret_cast<const Madt*>(Header);

        /* The MADT table contains info about SMP (multiples cores) and IO-APIC/Local APIC. The IO/LAPIC addresses are
         * all physical of course, so we need to grab the right one and map it into virtual memory (no need to save the
         * size btw, we're not going to unmap it while the kernel is running). */

        UIntPtr size = PAGE_SIZE, size2 = PAGE_SIZE;
        UInt64 lapic = madt->LApicAddress, ioapic = 0;

        for (auto cur = madt->Records;
             reinterpret_cast<UIntPtr>(cur) < reinterpret_cast<UIntPtr>(Header) + Header->Length; cur += cur[1]) {
            switch (cur[0]) {
            case 0: if ((cur[4] & 0x01) && CoreCount < 2048) LApicIds[CoreCount++] = cur[3]; break;
            case 1: ioapic = reinterpret_cast<const MadtIoApic*>(&cur[2])->Address; break;
            case 5: lapic = reinterpret_cast<const MadtLApicOverride*>(&cur[2])->Address; break;
            }
        }

        ASSERT(ioapic);
        ASSERT(VirtMem::MapIo(lapic, size, LApicAddress) == Status::Success &&
               VirtMem::MapIo(ioapic, size2, IoApicAddress) == Status::Success);
    } else if (CompareMemory(Header->Signature, "HPET", 4)) {
        /* HPET is our main/only timer in x86/amd64 (for now), it's all MMIO, so accessing it is fast (not as fast as
         * RDTSC tho), and we can set it up to trigger interrupts every 10ms or so (or less, for sleeping and task
         * switching). */

        UIntPtr size = PAGE_SIZE;
        auto hpet = reinterpret_cast<const Hpet*>(Header);
        ASSERT(VirtMem::MapIo(hpet->Address.Address, size, HpetAddress) == Status::Success);

        HpetFrequency = GetHpetGeneralCaps() >> 32;
        GetHpetGeneralConfig() = 0;
        GetHpetMainCounter() = 0;
        GetHpetGeneralConfig() = 0x03;
    }
}

Void Arch::Initialize(const BootInfo&) {
    /* Initialize the descriptor tables (GDT and IDT, as they are required for pretty much everything, including
     * gracefully handling exceptions/errors).*/

    GdtInit();
    Debug.Write("initialized the global descriptor table\n");

    IdtInit();
    Debug.Write("initialized the interrupt descriptor table\n");
}

no_return Void Arch::Halt(Boolean Full) {
    if (Full) while (True) asm volatile("cli; hlt");
    else while (True) asm volatile("hlt");
}

Void Arch::Sleep(UInt8 Unit, UInt64 Count) {
    /* HPET has a precision on the femtoseconds range, so we don't really need to worry about it sleeping too little
     * or too much. */

    for (UInt64 dest = Acpi::GetHpetMainCounter() + Count *
                       (!Unit ? 1000000 : (Unit == 1 ? 1000000000 : (Unit == 2 ? 1000000000000 : 1000000000000000))) /
                       Acpi::GetHpetFrequency(); Acpi::GetHpetMainCounter() < dest;) ;
}

UInt64 Arch::GetUpTime(UInt8 Unit) {
    return (Acpi::GetHpetMainCounter() * Acpi::GetHpetFrequency()) / (!Unit ? 1000000 : (Unit == 1 ? 1000000000 :
                                                                     (Unit == 2 ? 1000000000000 : 1000000000000000)));
}
