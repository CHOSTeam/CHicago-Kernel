/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 29 of 2020, at 10:17 BRT
 * Last edited on March 05 of 2021, at 13:22 BRT */

#include <arch/desctables.hxx>
#include <base/string.hxx>

namespace CHicago {

#ifdef __i386__
static UInt8 GdtEntries[6][8];
#else
static UInt8 GdtEntries[7][8];
#endif

static TssEntry GdtTssEntry;
static DescTablePointer GdtPointer;

static Void GdtSetGate(UInt8 Num, UIntPtr Base, UIntPtr Limit, UInt8 Type, UInt8 Granularity) {
	/* We could probably make a packed struct for this (like we're using for the TSS), but for now, let's encode
	 * everything manually, starting with the base and the limit. */

	GdtEntries[Num][0] = Limit & 0xFF;
	GdtEntries[Num][1] = (Limit >> 8) & 0xFF;
	GdtEntries[Num][6] = (Granularity & 0xF0) | ((Limit >> 16) & 0x0F);

	GdtEntries[Num][2] = Base & 0xFF;
	GdtEntries[Num][3] = (Base >> 8) & 0xFF;
	GdtEntries[Num][4] = (Base >> 16) & 0xFF;
	GdtEntries[Num][7] = (Base >> 24) & 0xFF;

	/* And... encode the type? It is just a normal byte that we're going to set, we don't exactly have anything to
	 * encode here... */

	GdtEntries[Num][5] = Type;
}

static Void GdtSetTss(UInt8 Num, UIntPtr Stack) {
	/* We're supposed to only call this one time, if that is not the case, well, F***. First, (subarch-specific)
	 * set the right GDT entry(ies). */

	auto base = reinterpret_cast<UIntPtr>(&GdtTssEntry);

#ifdef __i386__
    GdtSetGate(Num, base, base + sizeof(TssEntry), 0xE9, 0x00);
#else
    UIntPtr base3 = (base >> 32) & 0xFFFF, base4 = (base >> 48) & 0xFFFF;
    GdtSetGate(Num, base, sizeof(TssEntry), 0xE9, 0x00);
    GdtSetGate(Num + 1, base4, base3, 0x00, 0x00);
#endif

	/* Now we can clean the TSS entry struct, and fill the fields we need (which again, those fields are
	 * subarch-specific, except for the IoMapBase). */

	SetMemory(&GdtTssEntry, 0, sizeof(TssEntry));

#ifdef __i386__
    GdtTssEntry.Ss0 = 0x10;
	GdtTssEntry.Esp0 = Stack;
#else
    GdtTssEntry.Rsp0 = Stack;
#endif

	GdtTssEntry.IoMapBase = sizeof(TssEntry);
}

Void GdtInit() {
	/* Setup all the GDT entries: 0 is the null entry, 1 is the kernel code entry, 2 is kernel date entry, 3 is the
	 * user code entry, 4 is the user date entry and, finally, 5 (and 6 on x86-64) is the TSS entry. */

	GdtSetGate(0, 0, 0, 0, 0);

	/* Btw, all the entries except for the null entry are subarch-specific). */

#ifdef __i386__
    GdtSetGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
	GdtSetGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
	GdtSetGate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
	GdtSetGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
#else
    GdtSetGate(1, 0, 0xFFFFFFFF, 0x98, 0x20);
    GdtSetGate(2, 0, 0xFFFFFFFF, 0x92, 0x00);
    GdtSetGate(3, 0, 0xFFFFFFFF, 0xF8, 0x20);
    GdtSetGate(4, 0, 0xFFFFFFFF, 0xF2, 0x00);
#endif

	GdtSetTss(5, 0);

	/* Now, we just need to fill the GDT pointer, load the new GDT, reload all the segment registers, and flush the
	 * TSS. */

	GdtPointer.Limit = sizeof(GdtEntries) - 1;
	GdtPointer.Base = reinterpret_cast<UIntPtr>(GdtEntries);

	asm volatile("lgdt %0; mov $0x10, %%ax; mov %%ax, %%ds; mov %%ax, %%es; mov %%ax, %%ss; mov %%ax, %%fs\n"
                 "mov %%ax, %%gs" :: "m"(GdtPointer));

	/* Reloading the CS is a bit different on x86-64... */

#ifdef __i386__
    asm volatile("ljmp $0x08, $1f; 1:");
#else
    asm volatile("movabs $1f, %r8; push $0x08; push %r8; lretq; 1:");
#endif

	asm volatile("ltr %0" :: "r"(static_cast<UInt16>(0x2B)));
}

}
