/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 16 of 2021 at 09:52 BRT
 * Last edited on July 21 of 2021 at 19:56 BRT */

#pragma once

#include <arch/desctables.hxx>
#include <ds/list.hxx>
#include <sys/acpi.hxx>

namespace CHicago {

/* Maybe we should move the classes around a bit (to other files)? */

struct packed CoreInfo {
    CoreInfo *Self;
    CHicago::Gdt *Gdt;
    UIntPtr Id, LApicId;
    volatile Boolean Status;
    const UInt8 *KernelStack;
};

class IoApic {
public:
    IoApic(UIntPtr Address, UInt8 Base) : Address(Address), Base(Base) {
        Status = new Boolean[Count = ((GetRegister(1) >> 16) & 0xFF) + 1];
        if (Status != Null) Mask(False);
    }

    Void Free(UInt8);
    Int16 Alloc(Void);
    Boolean Alloc(UInt8);

    Void Mask(Boolean);
    Void Mask(UInt8, Boolean);
    Void Set(UInt8, UInt8, UInt8);
    Void Set(UInt8, UInt8, Boolean, Boolean);
    Void GetInfo(UInt8, UInt8&, Boolean&, Boolean&);

    [[nodiscard]] volatile UInt32 &GetRegister(UIntPtr Off) {
        *reinterpret_cast<volatile UInt32*>(Address) = Off;
        return *reinterpret_cast<volatile UInt32*>(Address + 0x10);
    }

    [[nodiscard]] Boolean IsValid(Void) const { return Status != Null; }
    [[nodiscard]] UIntPtr GetAddress(Void) const { return Address; }
    [[nodiscard]] UInt8 GetCount(Void) const { return Count; }
    [[nodiscard]] UInt8 GetBase(Void) const { return Base; }
private:
    UIntPtr Address;
    UInt8 Count = 0, Base;
    Boolean *Status = Null;
};

class Apic {
public:
    struct packed Madt { Acpi::SdtHeader Header; UInt32 LApicAddress, Flags; UInt8 Records[]; };
    struct packed MadtIoApic { UInt8 Id, Res; UInt32 Address, GsiBase; };
    struct packed MadtIoApicSourceOverride { UInt8 Bus, Irq; UInt32 Gsi; UInt16 Flags; };
    struct packed MadtLApicOverride { UInt16 Res; UInt64 Address; };
    struct packed MadtX2Apic { UInt16 Res; UInt32 CoreId, Flags, AcpiId; };

    static Void Initialize(const Madt*);
    static Void SetupLApic(Void);

    static Void Free(UInt8);
    static Int16 Alloc(Void);
    static Boolean Alloc(UInt8);

    static Void Mask(Boolean);
    static Void Mask(UInt8, Boolean);
    static Void Set(UInt8, UInt8, UInt8);
    static Void Set(UInt8, UInt8, Boolean, Boolean);
    static Void GetInfo(UInt8, UInt8&, Boolean&, Boolean&);

    static UInt64 ReadLApicRegister(UIntPtr Off) {
        return !LApicAddress ? ReadMsr(0x800 + (Off >> 4)) : *reinterpret_cast<volatile UInt32*>(LApicAddress + Off);
    }

    static Void WriteLApicRegister(UIntPtr Off, UInt64 Value) {
        if (!LApicAddress) WriteMsr(0x800 + (Off >> 4), Value);
        else *reinterpret_cast<volatile UInt32*>(LApicAddress + Off) = Value;
    }

    static UInt8 GetLApicId(Void) {
        return !LApicAddress ? ReadLApicRegister(0x20) : ReadLApicRegister(0x20) >> 24;
    }

    [[nodiscard]] static Boolean IsInitialized(Void) { return Initialized; }
    [[nodiscard]] static UIntPtr GetLApicAddress(Void) { return LApicAddress; }
private:
    static Boolean Initialized;
    static List<IoApic> IoApics;
    static UIntPtr LApicAddress;
};

class Hpet {
public:
    struct packed Header {
        Acpi::SdtHeader Header;
        UInt8 HwRevision, CompCount : 5, CountSize : 1, Res : 1, Legacy : 1;
        UInt16 PciVendor;
        Acpi::GenericAddress Address;
        UInt8 Number;
        UInt16 MinTicks;
        UInt8 Protection;
    };

    struct Comparator {
        UIntPtr Id;
        Boolean Used;
        Void (*Handler)(Void*);
    };

    struct ComparatorGroup {
        UInt8 Irq;
        List<Comparator> Comparators;
    };

    static Void Initialize(const Header*);

    [[nodiscard]] static UInt64 ReadRegister(UIntPtr Off) {
#ifdef __i386__
        auto ptr = reinterpret_cast<volatile UInt32*>(Address + Off);
        return (UInt64)*ptr | ((UInt64)*(ptr + 1) << 32);
#else
        return *reinterpret_cast<volatile UInt64*>(Address + Off);
#endif
    }

    static Void WriteRegister(UIntPtr Off, UInt64 Value) {
#ifdef __i386__
        auto ptr = reinterpret_cast<volatile UInt32*>(Address + Off);
        *ptr = Value;
        *(ptr + 1) = Value >> 32;
#else
        *reinterpret_cast<volatile UInt64*>(Address + Off) = Value;
#endif
    }

    [[nodiscard]] static auto &GetGroups(Void) { return Groups; }
    [[nodiscard]] static UIntPtr GetAddress(Void) { return Address; }
    [[nodiscard]] static UIntPtr GetMinTicks(Void) { return MinTicks; }
    [[nodiscard]] static UInt64 GetFrequency(Void) { return Frequency; }
    [[nodiscard]] static Boolean IsInitialized(Void) { return Initialized; }
private:
    static UInt64 Frequency;
    static Boolean Initialized;
    static UIntPtr Address, MinTicks;
    static List<ComparatorGroup> Groups;
};

class Smp {
public:
    static Void Initialize(const BootInfo&, const Apic::Madt*);

    static Void SendIpi(UInt8, UInt32, UInt16);
    static Void SendTlbShootdown(UIntPtr, UIntPtr);

    [[nodiscard]] static CoreInfo &GetCurrentCore(Void) {
#ifdef __i386__
        UInt32 res; asm volatile("mov %%fs:0, %0" : "=r"(res));
#else
        UInt64 res; asm volatile("mov %%gs:0, %0" : "=r"(res));
#endif
        return *reinterpret_cast<CoreInfo*>(res);
    }

    [[nodiscard]] static auto &GetCoreList(Void) { return CoreList; }
    [[nodiscard]] static UIntPtr GetTlbShootdownSize(Void) { return TlbShootdownSize; }
    [[nodiscard]] static UIntPtr GetTlbShootdownAddress(Void) { return TlbShootdownAddress; }
private:
    static Void TlbShootdownHandler(Registers&);

    static List<CoreInfo> CoreList;
    static UIntPtr TlbShootdownAddress, TlbShootdownSize, TlbShootdownLeft;
};

}
