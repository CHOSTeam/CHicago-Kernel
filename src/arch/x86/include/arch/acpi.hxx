/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 16 of 2021 at 09:52 BRT
 * Last edited on July 16 of 2021 at 13:22 BRT */

public:
    struct packed Madt {
        SdtHeader Header;
        UInt32 LApicAddress, Flags;
        UInt8 Records[];
    };

    struct packed Hpet {
        SdtHeader Header;
        UInt8 HwRevision, CompCount : 5, CountSize : 1, Res : 1, Legacy : 1;
        UInt16 PciVendor;
        GenericAddress Address;
        UInt8 Number;
        UInt16 MinTick;
        UInt8 Protection;
    };

    struct packed MadtIoApic {
        UInt8 Id, Res;
        UInt32 Address, GsiBase;
    };

    struct packed MadtLApicOverride {
        UInt16 Res;
        UInt64 Address;
    };

    [[nodiscard]] static UIntPtr GetHpetAddress() { return HpetAddress; }
    [[nodiscard]] static UInt64 GetHpetFrequency() { return HpetFrequency; }

    [[nodiscard]] static auto &GetHpetGeneralCaps() {
        return *reinterpret_cast<volatile UInt64*>(HpetAddress);
    }

    [[nodiscard]] static auto &GetHpetGeneralConfig() {
        return *reinterpret_cast<volatile UInt64*>(HpetAddress + 0x10);
    }

    [[nodiscard]] static auto &GetHpetGeneralIntStatus() {
        return *reinterpret_cast<volatile UInt64*>(HpetAddress + 0x20);
    }

    [[nodiscard]] static auto &GetHpetMainCounter() {
        return *reinterpret_cast<volatile UInt64*>(HpetAddress + 0xF0);
    }

    [[nodiscard]] static auto &GetHpetConfig(UInt8 Num) {
        return *reinterpret_cast<volatile UInt64*>(HpetAddress + 0x100 + 0x20 * Num);
    }

    [[nodiscard]] static auto &GetHpetValue(UInt8 Num) {
        return *reinterpret_cast<volatile UInt64*>(HpetAddress + 0x108 + 0x20 * Num);
    }

    [[nodiscard]] static auto &GetHpetFSBRoute(UInt8 Num) {
        return *reinterpret_cast<volatile UInt64*>(HpetAddress + 0x110 + 0x20 * Num);
    }

    [[nodiscard]] static auto GetLApicIds(Void) { return LApicIds; }
    [[nodiscard]] static UInt16 GetCoreCount(Void) { return CoreCount; }
    [[nodiscard]] static UIntPtr GetLApicAddress(Void) { return LApicAddress; }
    [[nodiscard]] static UIntPtr GetIoApicAddress(Void) { return IoApicAddress; };
private:
    static UInt16 CoreCount;
    static UInt64 HpetFrequency;
    static UInt8 LApicIds[2048];
    static UIntPtr LApicAddress, IoApicAddress, HpetAddress;
