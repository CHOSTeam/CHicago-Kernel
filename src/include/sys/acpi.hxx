/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 11 of 2021, at 17:50 BRT
 * Last edited on March 11 of 2021 at 18:58 BRT */

#pragma once

#include <base/types.hxx>

namespace CHicago {

struct packed BootInfo;

class Acpi {
public:
    struct packed SdtHeader {
        Char Signature[4];
        UInt32 Length;
        UInt8 Revision, Checksum;
        Char OemID[6], OemTableID[8];
        UInt32 OemRevision, CreatorID, CreatorRevision;
    };

    static Void Initialize(BootInfo&);
};

}
