/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 11 of 2021, at 18:08 BRT
 * Last edited on July 20 of 2021, at 09:37 BRT */

#include <sys/mm.hxx>
#include <sys/panic.hxx>

namespace CHicago {

List<Acpi::CacheEntry> Acpi::Cache {};

Void Acpi::Initialize(const BootInfo &Info) {
    /* This function should really only be called one time, but we can have an idea if it was called before using the
     * cache (if it isn't 0, we have been called before, that's for sure). Here we map the main SDT table, grab all
     * the subtables, and cache them so that we can search without having to map every single entry every tine. */

    UIntPtr size = Info.Acpi.Size + (-Info.Acpi.Size & PAGE_MASK), addr;

    ASSERT(!Cache.GetLength());
    ASSERT(VirtMem::MapIo(Info.Acpi.Sdt, size, addr) == Status::Success);

    for (UIntPtr i = 0; i < (Info.Acpi.Size - sizeof(SdtHeader)) >> (Info.Acpi.Extended ? 3 : 2); i++) {
        UInt64 phys = Info.Acpi.Extended ? reinterpret_cast<const Xsdt*>(addr)->Pointers[i] :
                                           reinterpret_cast<const Rsdt*>(addr)->Pointers[i];
        UIntPtr size2 = sizeof(SdtHeader) + (-sizeof(SdtHeader) & PAGE_MASK), addr2;

        if (VirtMem::MapIo(phys, size2, addr2) != Status::Success) continue;

        auto hdr = reinterpret_cast<const SdtHeader*>(addr2);

        if (Cache.Add({ hdr->Length + (-hdr->Length & PAGE_MASK), phys, {} }) == Status::Success)
            CopyMemory(Cache[Cache.GetLength() - 1].Signature, hdr->Signature, 4);

        VirtMem::Unmap(addr2 & ~PAGE_MASK, size2);
        VirtMem::Free(addr2 & ~PAGE_MASK, size2 >> PAGE_SHIFT);
    }

    /* We probably want to make sure we have at least one ACPI table? */

    VirtMem::Unmap(addr & ~PAGE_MASK, size);
    VirtMem::Free(addr & ~PAGE_MASK, size >> PAGE_SHIFT);
}

Acpi::SdtHeader *Acpi::GetHeader(const Char Sig[4], UIntPtr &Out) {
    /* With the cache is just a matter of finding the right signature and mapping the address (we already know the size,
     * so there is no need to "guess" it. */

    UIntPtr addr;

    for (auto &ent : Cache) {
        if (!CompareMemory(ent.Signature, Sig, 4)) continue;
        return VirtMem::MapIo(ent.Address, (Out = ent.Size, Out), addr) == Status::Success ?
               reinterpret_cast<SdtHeader*>(addr) : Null;
    }

    return Null;
}

}
