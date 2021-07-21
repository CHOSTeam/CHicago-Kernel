/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 21 of 2021, at 09:57 BRT
 * Last edited on July 21 of 2021, at 20:05 BRT */

#include <arch/acpi.hxx>
#include <arch/port.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

Boolean Apic::Initialized = False;
UIntPtr Apic::LApicAddress = 0;
List<IoApic> Apic::IoApics {};

Void IoApic::Free(UInt8 Num) {
    if (Num < Count) AtomicStore(Status[Num], False);
}

Int16 IoApic::Alloc(Void) {
    /* Find any free entry, make sure to use atomic operations for everything here (as we might be competing with other
     * cores). */

    for (UInt8 i = 0; i < Count; i++) if (!AtomicExchange(Status[i], True)) return i;
    return -1;
}

Boolean IoApic::Alloc(UInt8 Num) {
    return Num < Count && !AtomicExchange(Status[Num], True);
}

Void IoApic::Mask(Boolean Unmask) {
    for (UInt8 i = 0; i < Count; i++) {
        UInt32 cur = GetRegister(0x10 + i * 2);
        if (Unmask) GetRegister(0x10 + i * 2) = cur & ~0x10000;
        else GetRegister(0x10 + i * 2) = cur | 0x10000;
    }
}

Void IoApic::Mask(UInt8 Num, Boolean Unmask) {
    if (Num >= Count) return;
    UInt32 cur = GetRegister(0x10 + Num * 2);
    if (Unmask) GetRegister(0x10 + Num * 2) = cur & ~0x10000;
    else GetRegister(0x10 + Num * 2) = cur | 0x10000;
}

Void IoApic::Set(UInt8 Num, UInt8 Vector, UInt8 DeliveryMode) {
    /* Each redirection entry is 64-bits long, with the destination CPU being the last 8 bits of the high word. */

    if (Num >= Count) return;
    GetRegister(0x10 + Num * 2) = (GetRegister(0x10 + Num * 2) & ~0x7FF) | ((DeliveryMode & 0x07) << 8) | Vector;
    GetRegister(0x11 + Num * 2) = Apic::GetLApicId() << 24;
}

Void IoApic::Set(UInt8 Num, UInt8 Vector, Boolean Low, Boolean Level) {
    /* On the MADT we might find interrupt redirection entries, they mean that we should override the bits 13 and 15
     * (which are normally static for us), and the vector of course. */

    if (Num >= Count) return;
    GetRegister(0x10 + Num * 2) = (GetRegister(0x10 + Num * 2) & ~0xA000) | (Low << 13) | (Level << 15) | Vector;
}

Void IoApic::GetInfo(UInt8 Num, UInt8 &Vector, Boolean &Low, Boolean &Level) {
    if (Num >= Count) return;
    UInt32 val = GetRegister(0x10 + Num * 2);
    Vector = val;
    Low = (val >> 13) & 0x01;
    Level = (val >> 15) & 0x01;
}

Void Apic::Initialize(const Madt *Madt) {
    /* We're the ones that should initialize the LAPIC (find the right physical address and map it to memory), and also
     * the ones that should find and initialize all the IOAPICs (there might be more than one). Finding and
     * initializing all the cores is done by Smp::Initialize(). */

    ASSERT(!Initialized);

    /* If possible, enable the MSR-based x2APIC (else, just keep on using MMIO-based xAPIC). */

    UIntPtr size;
    Boolean x2apic = False;
    UInt64 lapic = Madt->LApicAddress;
    UInt32 cx; asm volatile("cpuid" : "=c"(cx) : "a"(1) : "%ebx", "%edx");

    if (cx & 0x200000) {
        WriteMsr(0x1B, ReadMsr(0x1B) | 0xC00);
        x2apic = True;
    }

    for (auto cur = Madt->Records;
         reinterpret_cast<UIntPtr>(cur) < reinterpret_cast<UIntPtr>(Madt) + Madt->Header.Length; cur += cur[1]) {
        switch (cur[0]) {
        case 1: {
            /* Found one IOAPIC, map it into virtual memory and initialize it (mask/disable everything, we're going to
             * enable stuff as we need/use it). */

            auto rec = reinterpret_cast<const MadtIoApic*>(&cur[2]);
            UIntPtr phys = rec->Address, virt;
            Status status;

            if (VirtMem::MapIo(phys, (size = PAGE_SIZE, size), virt) == Status::Success &&
                ((status = IoApics.Add(IoApic(virt, rec->GsiBase))) != Status::Success ||
                 !IoApics[IoApics.GetLength() - 1].IsValid())) {
                if (status == Status::Success) IoApics.Remove(IoApics.GetLength() - 1);
                VirtMem::Unmap(virt, size);
                VirtMem::Free(virt, size >> PAGE_SHIFT);
            } else Debug.Write("found IOAPIC handling IRQs {}-{}\n",
                               rec->GsiBase, rec->GsiBase + IoApics[IoApics.GetLength() - 1].GetCount());

            break;
        }
        case 5: if (!x2apic) lapic = reinterpret_cast<const MadtLApicOverride*>(&cur[2])->Address; break;
        }
    }

    /* Now that all IOAPICs should be setup, we can parse the redirection/override entries (to finish setting up
     * everything). */

    for (auto cur = Madt->Records;
         reinterpret_cast<UIntPtr>(cur) < reinterpret_cast<UIntPtr>(Madt) + Madt->Header.Length; cur += cur[1]) {
        if (cur[0] != 2) continue;
        auto ent = reinterpret_cast<const MadtIoApicSourceOverride*>(&cur[2]);
        Set(ent->Gsi, ent->Irq + 32, ent->Flags & 0x02, ent->Flags & 0x08);
        IdtSetHandler(ent->Irq, Null);
    }

    /* We want to make sure we have at least one IOAPIC (and that the LAPIC is properly mapped into virtual memory). */

    ASSERT(IoApics.GetLength() > 0);
    if (!x2apic) ASSERT(VirtMem::MapIo(lapic, (size = PAGE_SIZE, size), LApicAddress) == Status::Success);

    Port::OutByte(0x21, 0xFF);
    Port::OutByte(0xA1, 0xFF);
    SetupLApic();

    Initialized = True;
}

Void Apic::SetupLApic(Void) {
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

Void Apic::Free(UInt8 Num) {
    for (auto &ent : IoApics) {
        if (Num >= ent.GetBase() && Num < ent.GetCount() + ent.GetCount()) {
            ent.Free(Num - ent.GetBase());
            return;
        }
    }
}

Boolean Apic::Alloc(UInt8 Num) {
    for (auto &ent : IoApics) {
        if (Num >= ent.GetBase() && Num < ent.GetCount() + ent.GetCount())
            return ent.Alloc(Num - ent.GetBase());
    }

    return False;
}

Int16 Apic::Alloc(Void) {
    for (auto &ent : IoApics) {
        Int16 res = ent.Alloc();
        if (res != -1) return res + ent.GetBase();
    }

    return -1;
}

Void Apic::Mask(Boolean Unmask) {
    for (auto &ent : IoApics) ent.Mask(Unmask);
}

Void Apic::Mask(UInt8 Num, Boolean Unmask) {
    /* In both this and in the next function we need to check the GSI of each IOAPIC (to choose the one that redirects
     * the requested num). */

    for (auto &ent : IoApics) {
        if (Num >= ent.GetBase() && Num < ent.GetBase() + ent.GetCount()) {
            ent.Mask(Num - ent.GetBase(), Unmask);
            return;
        }
    }
}

Void Apic::Set(UInt8 Num, UInt8 Vector, UInt8 DeliveryMode) {
    for (auto &ent : IoApics) {
        if (Num >= ent.GetBase() && Num < ent.GetBase() + ent.GetCount()) {
            ent.Set(Num - ent.GetBase(), Vector, DeliveryMode);
            return;
        }
    }
}

Void Apic::Set(UInt8 Num, UInt8 Vector, Boolean Low, Boolean Level) {
    for (auto &ent : IoApics) {
        if (Num >= ent.GetBase() && Num < ent.GetBase() + ent.GetCount()) {
            ent.Set(Num - ent.GetBase(), Vector, Low, Level);
            return;
        }
    }
}

Void Apic::GetInfo(UInt8 Num, UInt8 &Vector, Boolean &Low, Boolean &Level) {
    for (auto &ent : IoApics) {
        if (Num >= ent.GetBase() && Num < ent.GetBase() + ent.GetCount()) {
            ent.GetInfo(Num - ent.GetBase(), Vector, Low, Level);
            return;
        }
    }
}
