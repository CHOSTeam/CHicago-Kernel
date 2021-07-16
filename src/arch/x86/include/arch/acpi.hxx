/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 16 of 2021 at 09:52 BRT
 * Last edited on July 16 of 2021 at 16:27 BRT */

#pragma once

#include <sys/acpi.hxx>

namespace CHicago {

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

    [[nodiscard]] static auto &GetMainCounter() { return *reinterpret_cast<volatile UInt64*>(Address + 0xF0); }
    [[nodiscard]] static UIntPtr GetAddress(Void) { return Address; }
    [[nodiscard]] static UInt64 GetFrequency(Void) { return Frequency; }
    [[nodiscard]] static Boolean IsInitialized(Void) { return Initialized; }
private:
    [[nodiscard]] static auto &GetConfig(UInt8 Num) {
        return *reinterpret_cast<volatile UInt64*>(Address + 0x100 + 0x20 * Num);
    }

    [[nodiscard]] static auto &GetValue(UInt8 Num) {
        return *reinterpret_cast<volatile UInt64*>(Address + 0x108 + 0x20 * Num);
    }

    [[nodiscard]] static auto &GetFSBRoute(UInt8 Num) {
        return *reinterpret_cast<volatile UInt64*>(Address + 0x110 + 0x20 * Num);
    }

    [[nodiscard]] static auto &GetGeneralCaps(Void) { return *reinterpret_cast<volatile UInt64*>(Address); }
    [[nodiscard]] static auto &GetGeneralConfig() { return *reinterpret_cast<volatile UInt64*>(Address + 0x10); }
    [[nodiscard]] static auto &GetGeneralIntStatus() { return *reinterpret_cast<volatile UInt64*>(Address + 0x20); }

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

    static Void Initialize(const Madt*);

    [[nodiscard]] static auto GetLApicIds(Void) { return LApicIds; }
    [[nodiscard]] static UInt16 GetCoreCount(Void) { return CoreCount; }
    [[nodiscard]] static Boolean IsInitialized(Void) { return Initialized; }
    [[nodiscard]] static UIntPtr GetLApicAddress(Void) { return LApicAddress; }
    [[nodiscard]] static UIntPtr GetIoApicAddress(Void) { return IoApicAddress; };
private:
    static UInt16 CoreCount;
    static UInt8 LApicIds[256];
    static Boolean Initialized;
    static UIntPtr LApicAddress, IoApicAddress;
};

}
