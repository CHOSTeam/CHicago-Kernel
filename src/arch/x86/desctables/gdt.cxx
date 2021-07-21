/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 29 of 2020, at 10:17 BRT
 * Last edited on July 17 of 2021, at 20:43 BRT */

#include <arch/desctables.hxx>
#include <base/string.hxx>

namespace CHicago {

Void Gdt::Initialize(Void) {
    /* Setup all the GDT entries: 0 is the null entry, 1 is the kernel code entry, 2 is kernel date entry, 3 is the
	 * user code entry, 4 is the user date entry and, finally, 5 (and 6 on x86-64) is the TSS entry. */

    SetGate(0, 0, 0, 0, 0);

    /* Btw, all the entries except for the null entry are subarch-specific). */

#ifdef __i386__
    SetGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
	SetGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
	SetGate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
	SetGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
	SetGate(6, 0, 0xFFFFFFFF, 0x92, 0xCF);
	SetGate(7, 0, 0xFFFFFFFF, 0x92, 0xCF);
#else
    SetGate(1, 0, 0xFFFFFFFF, 0x98, 0x20);
    SetGate(2, 0, 0xFFFFFFFF, 0x92, 0x00);
    SetGate(3, 0, 0xFFFFFFFF, 0xF8, 0x20);
    SetGate(4, 0, 0xFFFFFFFF, 0xF2, 0x00);
#endif

    SetTss(5, 0);

    /* Now, we just need to fill the GDT pointer, load the new GDT, reload all the segment registers, and flush the
     * TSS. */

    Pointer.Limit = sizeof(Entries) - 1;
    Pointer.Base = reinterpret_cast<UIntPtr>(Entries);

    Reload();
}

Void Gdt::Reload(Void) const {
    asm volatile("lgdt %0; mov $0x10, %%ax; mov %%ax, %%ds; mov %%ax, %%es; mov %%ax, %%ss" :: "m"(Pointer));

    /* On x86, FS and GS are controlled by the GDT (so we need to update them here), but on amd64 they are controlled
     * by MSRs (so we don't need to do anything here). */

#ifdef __i386__
    asm volatile("mov $0x30, %ax; mov %ax, %fs; mov $0x38, %ax; mov %ax, %gs");
#else
    asm volatile("mov $0x10, %ax; mov %ax, %fs; mov %ax, %gs");
#endif

    /* Reloading the CS is a bit different on x86-64... */

#ifdef __i386__
    asm volatile("ljmp $0x08, $1f; 1:");
#else
    asm volatile("movabs $1f, %r8; push $0x08; push %r8; lretq; 1:");
#endif

    asm volatile("mov $0x28, %ax; ltr %ax");
}

#ifdef __i386__
Void Gdt::LoadSpecialSegment(Boolean Gs, UIntPtr Base) {
    /* We don't really need to reload the entire entry, we could just set the base here... */

    if (Gs) {
        SetGate(7, Base, 0xFFFFFFFF, 0x92, 0xCF);
        asm volatile("mov $0x38, %ax; mov %ax, %gs");
    } else {
        SetGate(6, Base, 0xFFFFFFFF, 0x92, 0xCF);
        asm volatile("mov $0x30, %ax; mov %ax, %fs");
    }
}
#endif

Void Gdt::SetGate(UInt8 Num, UIntPtr Base, UIntPtr Limit, UInt8 Type, UInt8 Granularity) {
    /* We could probably make a packed struct for this (like we're using for the TSS), but for now, let's encode
	 * everything manually, starting with the base and the limit. */

    Entries[Num][0] = Limit & 0xFF;
    Entries[Num][1] = (Limit >> 8) & 0xFF;
    Entries[Num][6] = (Granularity & 0xF0) | ((Limit >> 16) & 0x0F);

    Entries[Num][2] = Base & 0xFF;
    Entries[Num][3] = (Base >> 8) & 0xFF;
    Entries[Num][4] = (Base >> 16) & 0xFF;
    Entries[Num][7] = (Base >> 24) & 0xFF;

    /* And... encode the type? It is just a normal byte that we're going to set, we don't exactly have anything to
     * encode here... */

    Entries[Num][5] = Type;
}

Void Gdt::SetTss(UInt8 Num, UIntPtr Stack) {
    /* We're supposed to only call this one time, if that is not the case, well, F***. First, (subarch-specific)
	 * set the right GDT entry(ies). */

    auto base = reinterpret_cast<UIntPtr>(&Tss);

#ifdef __i386__
    SetGate(Num, base, base + sizeof(TssEntry), 0xE9, 0x00);
#else
    UIntPtr base3 = (base >> 32) & 0xFFFF, base4 = (base >> 48) & 0xFFFF;
    SetGate(Num, base, sizeof(TssEntry), 0xE9, 0x00);
    SetGate(Num + 1, base4, base3, 0x00, 0x00);
#endif

    /* Now we can clean the TSS entry struct, and fill the fields we need (which again, those fields are
     * subarch-specific, except for the IoMapBase). */

    SetMemory(&Tss, 0, sizeof(TssEntry));

#ifdef __i386__
    Tss.Ss0 = 0x10;
	Tss.Esp0 = Stack;
#else
    Tss.Rsp0 = Stack;
#endif

    Tss.IoMapBase = sizeof(TssEntry);
}

}
