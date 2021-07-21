/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 20 of 2021, at 19:35 BRT
 * Last edited on July 21 of 2021 at 19:54 BRT */

#include <arch/acpi.hxx>
#include <sys/panic.hxx>
#include <util/bitop.hxx>

using namespace CHicago;


UIntPtr Hpet::Address = 0, Hpet::MinTicks = 0;
List<Hpet::ComparatorGroup> Hpet::Groups {};
Boolean Hpet::Initialized = False;
UInt64 Hpet::Frequency = 0;

static UInt64 GetTicks(TimeUnit Unit, UInt64 Count) {
    return Count * (Unit == TimeUnit::Nanoseconds ? 1000000 : (Unit == TimeUnit::Microseconds ? 1000000000 :
                   (Unit == TimeUnit::Milliseconds ? 1000000000000 : 1000000000000000))) / Hpet::GetFrequency();
}

static Void Handler(Registers &Regs) {
    UInt64 val = Hpet::ReadRegister(0x20);

    for (auto &group : Hpet::GetGroups()) {
        if (group.Irq != Regs.IntNum) continue;
        for (auto &comp : group.Comparators) if (val & (1 << comp.Id)) {
            Hpet::WriteRegister(0x20, 1 << comp.Id);
            if (AtomicLoad(comp.Used) && comp.Handler != Null) comp.Handler(&Regs);
            AtomicStore(comp.Used, False);
        }
    }
}

Void Hpet::Initialize(const Header *Header) {
    /* HPET is our main/only timer in x86/amd64 (for now), it's all MMIO, so accessing it is fast (not as fast as
     * RDTSC), and we can set it up to trigger interrupts every 10ms or so (or less, for sleeping and task
     * switching). */

    ASSERT(!Initialized);

    UInt64 caps;
    UIntPtr size = PAGE_SIZE, count = 0;
    ASSERT(VirtMem::MapIo(Header->Address.Address, size, Address) == Status::Success);

    Frequency = (caps = ReadRegister(0)) >> 32;
    MinTicks = Header->MinTicks;

    WriteRegister(0x10, 0x00);
    WriteRegister(0xF0, 0x00);
    WriteRegister(0x10, 0x01);

    for (UIntPtr i = 0; i < ((caps >> 8) & 0x1F) + 1; i++) {
        UInt8 irq = 0;
        List<UInt8> valid;
        Boolean fail = False;
        ComparatorGroup *found = Null;
        UInt64 off = 0x100 + 0x20 * i, val = ReadRegister(off);

        /* Let's try to divide the IRQs as evenly as possible (as we're probably going to have to share the same IRQ
         * with multiple comparators). */

        for (UInt32 cur = val >> 32; cur;) {
            Boolean any = False;
            UInt8 idx = BitOp::ScanForward(cur);

            irq += idx;

            for (auto &group : Groups) {
                if (group.Irq == irq) {
                    any = True;
                    break;
                }
            }

            if (!any && Apic::Alloc(irq)) {
                /* Best possible choice is that this is a new IRQ (no comparators in it yet), we don't even need to
                 * check which group is the best one. */

                if (Groups.Add({ irq, {} }) != Status::Success) Apic::Free(irq), fail = True;
                else found = &Groups[Groups.GetLength() - 1];
                break;
            } else valid.Add(irq);

            cur >>= idx + 1;
            irq++;
        }

        if (fail) continue;
        else if (found == Null) {
            found = &Groups[0];
            for (auto &cur : Groups) {
                if (found == &cur) continue;
                if (cur.Comparators.GetLength() < cur.Comparators.GetLength()) found = &cur;
            }
        }

        if (found->Comparators.Add({ i, False, Null }) == Status::Success) {
            WriteRegister(off, val | (found->Irq << 9) | 0x06);
            count++;
        }
    }

    /* Initialize the interrupt handler and alloc/reserve the IRQs. */

    Initialized = True;

    for (auto &group : Groups) {
        UInt8 irq;
        Boolean low, level;

        Apic::GetInfo(group.Irq, irq, low, level);
        if (!irq) {
            irq = IdtAllocIrq();
            if (irq == 0xFF) continue;
            irq += 32;
        }

        if (!level) Apic::Set(group.Irq, irq, low, True);

        Apic::Set(group.Irq, irq, 0);
        Apic::Mask(group.Irq, True);
        IdtSetHandler((group.Irq = irq) - 32, Handler);

        /* Setup dummy events on this group, required on QEMU at least (as it seems to send one interrupt to each
         * counter when you init them, and that could disrupt the SetEvent function). */

        for (auto &comp : group.Comparators) {
            AtomicStore(comp.Used, True);
            Hpet::WriteRegister(0x108 + 0x20 * comp.Id, Hpet::ReadRegister(0xF0) + MinTicks);
            while (AtomicLoad(comp.Used)) ;
        }
    }

    Debug.Write("{}initialized the HPET (frequency of {}Hz and {} comparators){}\n", SetForeground { 0xFF00FF00 },
                1000000000000000 / Frequency, count, RestoreForeground{});
}

Boolean Timer::SetEvent(TimeUnit Unit, UInt64 Count, Void (*Handler)(Void*)) {
    if (Handler == Null || !Hpet::IsInitialized()) return False;

    UInt64 dest = Hpet::ReadRegister(0xF0), ticks = GetTicks(Unit, Count);
    dest += ticks < Hpet::GetMinTicks() ? Hpet::GetMinTicks() : ticks;

    for (auto &group : Hpet::GetGroups()) for (auto &comp : group.Comparators) if (!AtomicExchange(comp.Used, True)) {
        comp.Handler = Handler;
        Hpet::WriteRegister(0x108 + 0x20 * comp.Id, dest - Hpet::ReadRegister(0xF0) < Hpet::GetMinTicks() ?
                                                           Hpet::ReadRegister(0xF0) + Hpet::GetMinTicks() : dest);
        return True;
    }

    return False;
}

Void Timer::Sleep(TimeUnit Unit, UInt64 Count) {
    if (!Hpet::IsInitialized()) return;
    for (UInt64 dest = Hpet::ReadRegister(0xF0) + GetTicks(Unit, Count); Hpet::ReadRegister(0xF0) < dest;) ARCH_PAUSE();
}

UInt64 Timer::GetUpTime(TimeUnit Unit) {
    if (!Hpet::IsInitialized()) return 0;
    return (Hpet::ReadRegister(0xF0) * Hpet::GetFrequency()) /
           (Unit == TimeUnit::Nanoseconds ? 1000000 : (Unit == TimeUnit::Microseconds ? 1000000000 :
           (Unit == TimeUnit::Milliseconds ? 1000000000000 : 1000000000000000)));
}
