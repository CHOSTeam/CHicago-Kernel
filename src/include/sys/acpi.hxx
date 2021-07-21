/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 11 of 2021, at 17:50 BRT
 * Last edited on July 21 of 2021 at 12:23 BRT */

#pragma once

#include <base/status.hxx>

namespace CHicago {

struct packed BootInfo;
template<class T> class List;

class Acpi {
public:
    struct packed GenericAddress {
        UInt8 Type, Width, Offset, Size;
        UInt64 Address;
    };

    struct packed SdtHeader {
        Char Signature[4];
        UInt32 Length;
        UInt8 Revision, Checksum;
        Char OemID[6], OemTableID[8];
        UInt32 OemRevision, CreatorID, CreatorRevision;
    };

    struct packed Rsdt {
        SdtHeader Header;
        UInt32 Pointers[];
    };

    struct packed Xsdt {
        SdtHeader Header;
        UInt64 Pointers[];
    };

    struct packed Fadt {
        SdtHeader Header;
        UInt32 FirmwareCtl, Dsdt;
        UInt8 Res0, PrefPwmgrProf;
        UInt16 SciInt;
        UInt32 SmiCmdPort;
        UInt8 AcpiEnable, AcpiDisable, S4BiosReq, PStateCtrl;
        UInt32 Pm1aEventBlock, Pm1bEventBlock, Pm1aCtrlBlock, Pm1bCtrlBlock, Pm2CtrlBlock, PmTimerBlock, Gpe0Block,
               Gpe1Block;
        UInt8 Pm1EventLength, Pm1CtrlLength, Pm2CtrlLength, PmTimerLength, Gpe0Length, Gpe1Length, Gpe1Base, CStateCtrl;
        UInt16 WorstC2Lat, WorstC3Lat, FlushSize, FlushStride;
        UInt8 DutyOffset, DutyWidth, DayAlarm, MonthAlarm, Century;
        UInt16 BootArchFlags;
        UInt8 Res1;
        UInt32 Flags;
        GenericAddress ResetReg;
        UInt8 ResetValue, Res2[3];
        UInt64 XFirmwareCtrl, XDsdt;
        GenericAddress XPm1aEventBlock, XPm1bEventBlock, XPm1aCtrlBlock, XPm1CtrlBlock, XPm2CtrlBlock, XPmTimerBlock,
                       XGpe0Block, XGpe1Block;
    };

    static Void Initialize(const BootInfo&);
    static Void InitializeArch(const BootInfo&);
    static SdtHeader *GetHeader(const Char[4], UIntPtr&);
private:
    struct CacheEntry { UIntPtr Size; UInt64 Address; Char Signature[4]; };
    static List<CacheEntry> Cache;
};

}
