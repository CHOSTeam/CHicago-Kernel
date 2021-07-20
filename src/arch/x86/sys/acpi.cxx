/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 16 of 2021, at 16:06 BRT
 * Last edited on July 20 of 2021, at 15:46 BRT */

#include <arch/acpi.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

extern "C" Void SmpTrampoline(Void);

extern Gdt BspGdt;
extern "C" UInt32 SmpTrampolineCr3;
extern "C" CoreInfo *SmpTrampolineCoreInfo;

UIntPtr Hpet::Address = 0, Smp::LApicAddress = 0, Smp::IoApicAddress = 0, Smp::TlbShootdownAddress = 0,
        Smp::TlbShootdownSize = 0, Smp::TlbShootdownLeft = 0;
Boolean Hpet::Initialized = False, Smp::Initialized = False;
List<CoreInfo> Smp::CoreList {};
UInt64 Hpet::Frequency = 0;

Void Acpi::InitializeArch(const BootInfo &Info, const SdtHeader *Header) {
    if (Header == Null) {
        /* With everything properly initialized we can can startup the other cores (as now we have the timer and LAPIC
         * is setup). */

        ASSERT(Hpet::IsInitialized() && Smp::IsInitialized());
        Smp::StartupCores();
    } else if (CompareMemory(Header->Signature, "APIC", 4))
        Smp::Initialize(Info, reinterpret_cast<const Smp::Madt*>(Header));
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

    Frequency = GetRegister(0) >> 32;
    SetRegister(0x10, 0);
    SetRegister(0xF0, 0);
    SetRegister(0x10, 3);
    Initialized = True;

    Debug.Write("{}initialized the HPET (with the frequency of {}Hz){}\n", SetForeground { 0xFF00FF00 },
                1000000000000000 / Frequency, RestoreForeground{});
}

Void Smp::Initialize(const BootInfo &Info, const Madt *Header) {
    /* The MADT table contains info about SMP (multiples cores) and IO-APIC/Local APIC. The IO/LAPIC addresses are all
     * physical of course, so we need to grab the right one and map it into virtual memory (no need to save the size,
     * we're not going to unmap it while the kernel is running). */

    ASSERT(!Initialized);
    ASSERT(CoreList.Add({ Null, &BspGdt, 0, 0, True, Info.KernelStack }) == Status::Success);

    /* If possible we want to enable x2APIC, and in that case we can directly call GetLapicId(), but if we can only use
     * normal/xAPIC, we want to use CPUID to determine the BSP's LAPIC id. */

    Boolean x2apic = False;
    UInt64 lapic = Header->LApicAddress, ioapic = 0;
    UInt32 cx; asm volatile("cpuid" : "=c"(cx) : "a"(1) : "%ebx", "%edx");

    if (cx & 0x200000) {
        WriteMsr(0x1B, ReadMsr(0x1B) | 0xC00);
        CoreList[0].LApicId = GetLApicId();
        x2apic = True;
    } else {
        UInt32 bx; asm volatile("cpuid" : "=b"(bx) : "a"(1) : "%ecx", "%edx");
        CoreList[0].LApicId = bx >> 24;
    }

    UIntPtr id = CoreList[0].LApicId, size = PAGE_SIZE, size2 = PAGE_SIZE;

    for (auto cur = Header->Records;
         reinterpret_cast<UIntPtr>(cur) < reinterpret_cast<UIntPtr>(Header) + Header->Header.Length; cur += cur[1]) {

        switch (cur[0]) {
            case 0: {
                /* We have to be very careful with allocating the CPU/core structure, as SMP initialization will break
                 * if the address 0x8000 is not available. Fortunately, our allocator should be configured to skip
                 * pages in the first MiB. */

                if (!(cur[4] & 0x01) || cur[3] == id) break;

                auto stack = new UInt8[0x2000 + sizeof(Gdt)];
                if (stack == Null) break;
                else if (CoreList.Add({ Null, reinterpret_cast<Gdt*>(&stack[0x2000]), CoreList.GetLength(), cur[3],
                                        False, stack }) != Status::Success) delete[] stack;

                break;
            }
            case 1: ioapic = reinterpret_cast<const MadtIoApic*>(&cur[2])->Address; break;
            case 5: if (!x2apic) lapic = reinterpret_cast<const MadtLApicOverride*>(&cur[2])->Address; break;
        }
    }

    ASSERT(ioapic);
    ASSERT(VirtMem::MapIo(ioapic, size2, IoApicAddress) == Status::Success);
    if (!x2apic) ASSERT(VirtMem::MapIo(lapic, size, LApicAddress) == Status::Success);

    /* Now that the CPU/core list is initialized (and will not change anymore), we can initialize the .Self field of
     * all the info structs. */

    for (auto &info : CoreList) info.Self = &info;

    /* As we're not calling Arch::InitializeCore for the BSP, we need to manually init the FS/GS segment register. */

#ifdef __i386__
    BspGdt.LoadSpecialSegment(False, reinterpret_cast<UIntPtr>(&CoreList[0]));
#else
    WriteMsr(0xC0000101, reinterpret_cast<UIntPtr>(&CoreList[0]));
    WriteMsr(0xC0000102, reinterpret_cast<UIntPtr>(&CoreList[0]));
#endif

    /* Local APIC is necessary for initializing SMP, and it doesn't require any timer function (which we might not have
     * yet), so let's set it up: Interrupts are already disabled/masked, so we just need to setup the LAPIC itself and
     * sti. */

    IdtSetHandler(0xDD, TlbShootdownHandler);
    SetupLApic();

    Initialized = True;

    Debug.Write("detected {} core(s)\n", CoreList.GetLength());
}

Void Smp::StartupCores(Void) {
    if (CoreList.GetLength() == 1) return;

    auto addr = reinterpret_cast<UInt32*>(0x8000 + reinterpret_cast<UIntPtr>(&SmpTrampolineCr3) -
                                                   reinterpret_cast<UIntPtr>(SmpTrampoline));
    UIntPtr size = reinterpret_cast<UIntPtr>(&SmpTrampolineCoreInfo) - reinterpret_cast<UIntPtr>(SmpTrampoline) +
                   sizeof(Void*);

    size += -size & PAGE_MASK;

    /* Our bootstrap code should be in low physical memory (0x8000), said address (actually, anything up to 1MiB of
     * phys mem) should be free for use. */

    ASSERT(VirtMem::Map(0x8000, 0x8000, size, MAP_KERNEL | MAP_RW | MAP_EXEC) == Status::Success);
    CopyMemory(reinterpret_cast<Void*>(0x8000), reinterpret_cast<const Void*>(SmpTrampoline), size);
#ifdef __i386__
    asm volatile("mov %%cr3, %%eax; mov %%eax, %0" : "=r"(*addr));
#else
    asm volatile("mov %%cr3, %%rax; mov %%eax, %0" : "=r"(*addr));
#endif

    for (auto &info : CoreList) {
        if (info.Status) continue;

        *reinterpret_cast<CoreInfo**>(0x8000 + reinterpret_cast<UIntPtr>(&SmpTrampolineCoreInfo) -
                                               reinterpret_cast<UIntPtr>(SmpTrampoline)) = &info;

        /* Modern CPUs (pretty much everything since the original Pentiums) generally don't need more than a single
         * SIPI (and of course there is no need for the INIT deassert IPI). */

        SendIpi(0, info.LApicId, 0x4500);
        Arch::Sleep(TimeUnit::Milliseconds, 10);
        SendIpi(0, info.LApicId, 0x4608);

        while (!AtomicLoad(info.Status)) ARCH_PAUSE();
    }

    VirtMem::Unmap(0x8000, size);
}

Void Smp::SetupLApic(Void) {
    /* Hardware enable the APIC if required (if x2APIC on the non-BSP cores, and for everyone on non-x2APIC). */

    if (LApicAddress) WriteMsr(0x1B, ReadMsr(0x1B) | 0x800);
    else if (Arch::GetCoreId()) WriteMsr(0x1B, ReadMsr(0x1B) | 0xC00);

    /* 0x1FF = LAPIC enabled and spurious vector equal to 0xFF. */

    WriteLApicRegister(0xF0, 0x1FF);

    /* Unmask/setup external interrupts and NMIs (and enable interrupts). */

    WriteLApicRegister(0x350, 0x700);
    WriteLApicRegister(0x360, 0x400);
    WriteLApicRegister(0x80, 0);

    asm volatile("sti");
}

Void Smp::SendIpi(UInt8 Type, UInt32 Dest, UInt16 Vector) {
    UInt64 val = (Vector & ~0xFFFF3000) | (!Type ? 0 : (Type == 1 ? 0x80000 : 0xC0000));

    if (!Initialized) return;
    else if (!LApicAddress) WriteLApicRegister(0x300, ((UInt64)Dest << 32) | val);
    else {
        WriteLApicRegister(0x310, Dest << 24);
        WriteLApicRegister(0x300, val);
        do { ARCH_PAUSE(); } while (ReadLApicRegister(0x300) & 0x1000);
    }
}

Void Smp::SendTlbShootdown(UIntPtr Address, UIntPtr Size) {
    /* When unmapping something we need to warn the other cores that they need to update their TLB, for this, we use a
     * TLB shootdown IPI (vector 0xFD here in CHicago). The lock is so that if multiple cores are trying to update
     * each other, they sync correctly. */

    if (!Initialized) return;
    static SpinLock lock;
    lock.Acquire();

    TlbShootdownAddress = Address;
    TlbShootdownSize = Size;
    TlbShootdownLeft = 0;

    for (auto &info : CoreList) {
        if (!info.Status || info.LApicId == GetLApicId()) continue;
        SendIpi(0, info.LApicId, 0xFD);
        AtomicAddFetch(TlbShootdownLeft, 1);
    };

    while (AtomicLoad(TlbShootdownLeft)) ARCH_PAUSE();

    lock.Release();
}

Void Smp::TlbShootdownHandler(Registers&) {
    for (UIntPtr i = 0; i < TlbShootdownSize; i += PAGE_SIZE)
        asm volatile("invlpg (%0)" :: "r"(TlbShootdownAddress + i) : "memory");
    AtomicSubFetch(TlbShootdownLeft, 1);
}
