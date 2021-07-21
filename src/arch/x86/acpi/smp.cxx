/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 20 of 2021, at 19:42 BRT
 * Last edited on July 21 of 2021, at 20:03 BRT */

#include <arch/acpi.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

extern "C" Void SmpTrampoline(Void);

extern Gdt BspGdt;
extern "C" UInt32 SmpTrampolineCr3;
extern "C" CoreInfo *SmpTrampolineCoreInfo;

UIntPtr Smp::TlbShootdownAddress = 0, Smp::TlbShootdownSize = 0, Smp::TlbShootdownLeft = 0;
List<CoreInfo> Smp::CoreList {};

Void Arch::InitializeCore(Void) {
    auto &info = **reinterpret_cast<CoreInfo**>(0x8000 + reinterpret_cast<UIntPtr>(&SmpTrampolineCoreInfo) -
                                                reinterpret_cast<UIntPtr>(SmpTrampoline));

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
    info.Gdt->LoadSpecialSegment(False, reinterpret_cast<UIntPtr>(&info));
#else
    WriteMsr(0xC0000101, reinterpret_cast<UIntPtr>(&info));
    WriteMsr(0xC0000102, reinterpret_cast<UIntPtr>(&info));
#endif

    IdtReload();
    Apic::SetupLApic();
    asm volatile("sti");
}

UIntPtr Arch::GetCoreId(Void) {
    return Smp::GetCoreList().GetLength() <= 1 ? 0 : Smp::GetCurrentCore().Id;
}

Void Arch::FinishCore(Void) {
    AtomicStore(Smp::GetCurrentCore().Status, True);
}

Void Smp::Initialize(const BootInfo &Info, const Apic::Madt *Header) {
    ASSERT(CoreList.Add({ Null, &BspGdt, 0, Apic::GetLApicId(), True, Info.KernelStack }) == Status::Success);

    UIntPtr id = CoreList[0].LApicId;

    for (auto cur = Header->Records;
         reinterpret_cast<UIntPtr>(cur) < reinterpret_cast<UIntPtr>(Header) + Header->Header.Length; cur += cur[1]) {
        if (!cur[0]) {
            /* We have to be very careful with allocating the CPU/core structure, as SMP initialization will break
             * if the address 0x8000 is not available. Fortunately, our allocator should be configured to skip
             * pages in the first MiB. */

            if (!(cur[4] & 0x01) || cur[3] == id) continue;

            auto stack = new UInt8[0x2000 + sizeof(Gdt)];
            if (stack != Null &&
                CoreList.Add({ Null, reinterpret_cast<Gdt*>(&stack[0x2000]), CoreList.GetLength(), cur[3], False,
                               stack }) != Status::Success) delete[] stack;
        } else if (cur[0] == 9) {
            /* This is the same as above, but using a 32-bit x2APIC id instead of a 8-bit xAPIC id. */

            auto core = reinterpret_cast<const Apic::MadtX2Apic*>(&cur[2]);

            if (!(core->Flags & 0x01) || core->CoreId == id) continue;

            auto stack = new UInt8[0x2000 + sizeof(Gdt)];
            if (stack != Null &&
                CoreList.Add({ Null, reinterpret_cast<Gdt*>(&stack[0x2000]), CoreList.GetLength(), core->CoreId, False,
                               stack }) != Status::Success) delete[] stack;
        }
    }

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

    Debug.Write("detected {} core(s)\n", CoreList.GetLength());
    if (CoreList.GetLength() <= 1) return;

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
        Timer::Sleep(TimeUnit::Milliseconds, 10);
        SendIpi(0, info.LApicId, 0x4608);

        while (!AtomicLoad(info.Status)) ARCH_PAUSE();
    }

    VirtMem::Unmap(0x8000, size);
}


Void Smp::SendIpi(UInt8 Type, UInt32 Dest, UInt16 Vector) {
    UInt64 val = (Vector & ~0xFFFF3000) | (!Type ? 0 : (Type == 1 ? 0x80000 : 0xC0000));

    if (!Apic::IsInitialized()) return;
    else if (!Apic::GetLApicAddress()) Apic::WriteLApicRegister(0x300, ((UInt64)Dest << 32) | val);
    else {
        Apic::WriteLApicRegister(0x310, Dest << 24);
        Apic::WriteLApicRegister(0x300, val);
        do { ARCH_PAUSE(); } while (Apic::ReadLApicRegister(0x300) & 0x1000);
    }
}

Void Smp::SendTlbShootdown(UIntPtr Address, UIntPtr Size) {
    /* When unmapping something we need to warn the other cores that they need to update their TLB, for this, we use a
     * TLB shootdown IPI (vector 0xFD here in CHicago). The lock is so that if multiple cores are trying to update
     * each other, they sync correctly. */

    if (!Apic::IsInitialized() || CoreList.GetLength() <= 1) return;
    static SpinLock lock;
    lock.Acquire();

    TlbShootdownAddress = Address;
    TlbShootdownSize = Size;
    TlbShootdownLeft = 0;

    for (auto &info : CoreList) {
        if (!info.Status || info.LApicId == Apic::GetLApicId()) continue;
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
