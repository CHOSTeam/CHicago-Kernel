/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 16 of 2021 at 09:52 BRT
 * Last edited on July 19 of 2021 at 09:28 BRT */

#pragma once

#include <arch/desctables.hxx>
#include <ds/list.hxx>
#include <sys/acpi.hxx>

namespace CHicago {

struct packed CoreInfo {
    CoreInfo *Self;
    CHicago::Gdt *Gdt;
    UIntPtr Id, LApicId;
    volatile Boolean Status;
    const UInt8 *KernelStack;
};

class Hpet {
public:
    struct packed Header {
        Acpi::SdtHeader Header;
        UInt8 HwRevision, CompCount : 5, CountSize : 1, Res : 1, Legacy : 1;
        UInt16 PciVendor;
        Acpi::GenericAddress Address;
        UInt8 Number;
        UInt16 MinTick;
        UInt8 Protection;
    };

    static Void Initialize(const Header*);

    [[nodiscard]] static UInt64 GetRegister(UIntPtr Off) {
#ifdef __i386__
        auto ptr = reinterpret_cast<volatile UInt32*>(Address + Off);
        return (UInt64)*ptr | ((UInt64)*(ptr + 1) << 32);
#else
        return *reinterpret_cast<volatile UInt64*>(Address + Off);
#endif
    }

    static Void SetRegister(UIntPtr Off, UInt64 Value) {
#ifdef __i386__
        auto ptr = reinterpret_cast<volatile UInt32*>(Address + Off);
        *ptr = Value;
        *(ptr + 1) = Value >> 32;
#else
        *reinterpret_cast<volatile UInt64*>(Address + Off) = Value;
#endif
    }

    [[nodiscard]] static UIntPtr GetAddress(Void) { return Address; }
    [[nodiscard]] static UInt64 GetFrequency(Void) { return Frequency; }
    [[nodiscard]] static Boolean IsInitialized(Void) { return Initialized; }
private:
    static UIntPtr Address;
    static UInt64 Frequency;
    static Boolean Initialized;
};

class Smp {
public:
    struct packed Madt {
            Acpi::SdtHeader Header;
            UInt32 LApicAddress, Flags;
            UInt8 Records[];
    };

    struct packed MadtIoApic {
            UInt8 Id, Res;
            UInt32 Address, GsiBase;
    };

    struct packed MadtLApicOverride {
            UInt16 Res;
            UInt64 Address;
    };

    static Void Initialize(const BootInfo&, const Madt*);
    static Void StartupCores(Void);
    static Void SetupLApic(Void);

    static Void SendIpi(UInt8, UInt8, UInt16);
    static Void SendTlbShootdown(UIntPtr, UIntPtr);

    static auto &GetLApicRegister(UIntPtr Off) { return *reinterpret_cast<volatile UInt32*>(LApicAddress + Off); }

    static UInt8 GetLApicId(Void) {
        if (LApicAddress) return GetLApicRegister(0x20) >> 24;
        UInt32 bx; asm volatile("cpuid" : "=b"(bx) : "a"(1) : "%ecx", "%edx");
        return bx >> 24;
    };

    [[nodiscard]] static CoreInfo &GetCurrentCore(Void) {
#ifdef __i386__
        UInt32 res; asm volatile("mov %%fs:0, %0" : "=r"(res));
#else
        UInt64 res; asm volatile("mov %%gs:0, %0" : "=r"(res));
#endif
        return *reinterpret_cast<CoreInfo*>(res);
    }

    [[nodiscard]] static auto &GetCoreList(Void) { return CoreList; }
    [[nodiscard]] static Boolean IsInitialized(Void) { return Initialized; }
    [[nodiscard]] static UIntPtr GetLApicAddress(Void) { return LApicAddress; }
    [[nodiscard]] static UIntPtr GetIoApicAddress(Void) { return IoApicAddress; };
    [[nodiscard]] static UIntPtr GetTlbShootdownSize(Void) { return TlbShootdownSize; }
    [[nodiscard]] static UIntPtr GetTlbShootdownAddress(Void) { return TlbShootdownAddress; }
private:
    static Void TlbShootdownHandler(Registers&);

    static Boolean Initialized;
    static List<CoreInfo> CoreList;
    static UIntPtr LApicAddress, IoApicAddress, TlbShootdownAddress, TlbShootdownSize;
};

}
