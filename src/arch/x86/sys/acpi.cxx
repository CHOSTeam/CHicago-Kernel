/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 16 of 2021, at 16:06 BRT
 * Last edited on July 16 of 2021, at 16:37 BRT */

#include <arch/acpi.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

UIntPtr Hpet::Address = 0, Smp::LApicAddress = 0, Smp::IoApicAddress = 0;
Boolean Hpet::Initialized = False, Smp::Initialized = False;
UInt8 Smp::LApicIds[256] { 0 };
UInt64 Hpet::Frequency = 0;
UInt16 Smp::CoreCount = 0;

Void Acpi::InitializeArch(const SdtHeader *Header) {
    if (Header == Null) ASSERT(Hpet::IsInitialized() && Smp::IsInitialized());
    else if (CompareMemory(Header->Signature, "APIC", 4)) Smp::Initialize(reinterpret_cast<const Smp::Madt*>(Header));
    else if (CompareMemory(Header->Signature, "HPET", 4))
        Hpet::Initialize(reinterpret_cast<const Hpet::Header*>(Header));
}

Void Hpet::Initialize(const Header *Header) {
    /* HPET is our main/only timer in x86/amd64 (for now), it's all MMIO, so accessing it is fast (not as fast as
     * RDTSC), and we can set it up to trigger interrupts every 10ms or so (or less, for sleeping and task
     * switching). */

    ASSERT(!Initialized);

    UIntPtr size = PAGE_SIZE;
    ASSERT(VirtMem::MapIo(Header->Address.Address, size, Address) == Status::Success);

    Frequency = GetGeneralCaps() >> 32;
    GetGeneralConfig() = 0;
    GetMainCounter() = 0;
    GetGeneralConfig() = 0x03;
    Initialized = True;

    Debug.Write("initialized the HPET (with the frequency of {}Hz)\n", 1000000000000000 / Frequency);
}

Void Smp::Initialize(const Madt *Header) {
    /* The MADT table contains info about SMP (multiples cores) and IO-APIC/Local APIC. The IO/LAPIC addresses are all
     * physical of course, so we need to grab the right one and map it into virtual memory (no need to save the size,
     * we're not going to unmap it while the kernel is running). */

    ASSERT(!Initialized);

    UIntPtr size = PAGE_SIZE, size2 = PAGE_SIZE;
    UInt64 lapic = Header->LApicAddress, ioapic = 0;

    for (auto cur = Header->Records;
         reinterpret_cast<UIntPtr>(cur) < reinterpret_cast<UIntPtr>(Header) + Header->Header.Length; cur += cur[1]) {
        switch (cur[0]) {
            case 0: if ((cur[4] & 0x01) && CoreCount < 256) LApicIds[CoreCount++] = cur[3]; break;
            case 1: ioapic = reinterpret_cast<const MadtIoApic*>(&cur[2])->Address; break;
            case 5: lapic = reinterpret_cast<const MadtLApicOverride*>(&cur[2])->Address; break;
        }
    }

    ASSERT(ioapic);
    ASSERT(VirtMem::MapIo(lapic, size, LApicAddress) == Status::Success &&
           VirtMem::MapIo(ioapic, size2, IoApicAddress) == Status::Success);

    Initialized = True;

    Debug.Write("detected {} core(s)\n", CoreCount);
}
