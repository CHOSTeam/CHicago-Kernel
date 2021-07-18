/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 11 of 2021, at 18:08 BRT
 * Last edited on July 17 of 2021, at 15:21 BRT */

#include <sys/mm.hxx>
#include <sys/panic.hxx>

namespace CHicago {

Void Acpi::Initialize(const BootInfo &Info) {
    /* For now, everything that we might use the ACPI tables for is arch-specific, so we just need to map the RSDT/XSDT
     * and travel through it (redirecting each table to the arch-specific ACPI init function). */

    UIntPtr size = Info.Acpi.Size + (-Info.Acpi.Size & PAGE_MASK), out;
    ASSERT(VirtMem::MapIo(Info.Acpi.Sdt, size, out) == Status::Success);

    for (UIntPtr i = 0; i < (Info.Acpi.Size - sizeof(SdtHeader)) >> (Info.Acpi.Extended ? 3 : 2); i++) {
        UInt64 phys = Info.Acpi.Extended ? reinterpret_cast<const Xsdt*>(out)->Pointers[i] :
                                           reinterpret_cast<const Rsdt*>(out)->Pointers[i];
        UIntPtr size2 = PAGE_SIZE, out2;

        if (VirtMem::MapIo(phys, size2, out2) != Status::Success) continue;

        /* If we "guessed" a size that is too small for the table, unmap it, free the virtual address and remap it
         * using the right size. */

        UIntPtr nsize = reinterpret_cast<const SdtHeader*>(out2)->Length;

        if (size2 - (out2 & PAGE_MASK) < nsize) {
            VirtMem::Unmap(out2 & ~PAGE_MASK, size2);
            VirtMem::Free(out2& ~PAGE_MASK, size2 >> PAGE_SHIFT);
            size2 = nsize;
            if (VirtMem::MapIo(phys, size2, out2) != Status::Success) continue;
        }

        InitializeArch(Info, reinterpret_cast<const SdtHeader*>(out2));
        VirtMem::Unmap(out2 & ~PAGE_MASK, size2);
        VirtMem::Free(out2 & ~PAGE_MASK, size2 >> PAGE_SHIFT);
    }

    InitializeArch(Info, Null);
}

}
