/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 29 of 2020, at 10:17 BRT
 * Last edited on October 24 of 2020, at 14:34 BRT */

#include <arch/desctables.hxx>
#include <string.hxx>

#ifdef ARCH_64
static UInt8 GdtEntries[7][8];
#else
static UInt8 GdtEntries[6][8];
#endif

static TSSEntry GdtTSSEntry;
static DescTablePointer GdtPointer;

no_inline static Void GdtSetGate(UInt8 Num, UIntPtr Base, UIntPtr Limit, UInt8 Type, UInt8 Granurality) {	
	/* We could probably make a packed struct for this (like we're using for the TSS), but for now, let's
	 * encode everything manually, starting with the base and the limit. */
	
	GdtEntries[Num][0] = Limit & 0xFF;
	GdtEntries[Num][1] = (Limit >> 8) & 0xFF;
	GdtEntries[Num][6] = (Granurality & 0xF0) | ((Limit >> 16) & 0x0F);
	
	GdtEntries[Num][2] = Base & 0xFF;
	GdtEntries[Num][3] = (Base >> 8) & 0xFF;
	GdtEntries[Num][4] = (Base >> 16) & 0xFF;
	GdtEntries[Num][7] = (Base >> 24) & 0xFF;
	
	/* And... encode the type? It is just a normal byte that we're going to set, we don't exactly have
	 * anything to encode here... */
	
	GdtEntries[Num][5] = Type;
}

no_inline static Void GdtSetTSS(UInt8 Num, UIntPtr Stack) {
	/* We're supposed to only call this one time, if that is not the case, well, F***. First, (subarch-specific)
	 * set the right GDT entry(ies). */
	
	UIntPtr base = (UIntPtr)&GdtTSSEntry;
	
#ifdef ARCH_64
	UIntPtr base3 = (base >> 32) & 0xFFFF;
	UIntPtr base4 = (base >> 48) & 0xFFFF;
	
	GdtSetGate(Num, base, sizeof(TSSEntry), 0xE9, 0x00);
	GdtSetGate(Num + 1, base4, base3, 0x00, 0x00);
#else
	GdtSetGate(Num, base, base + sizeof(TSSEntry), 0xE9, 0x00);
#endif
	
	/* Now we can clean the TSS entry struct, and fill the fields we need (which again, those fields are
	 * subarch-specific, except for the iomap_base). */
	
	StrSetMemory(&GdtTSSEntry, 0, sizeof(TSSEntry));
	
#ifdef ARCH_64
	GdtTSSEntry.Rsp0 = Stack;
#else
	GdtTSSEntry.Ss0 = 0x10;
	GdtTSSEntry.Esp0 = Stack;
#endif
	
	GdtTSSEntry.IoMapBase = sizeof(TSSEntry);
}

Void GdtInit(Void) {
	/* Setup all the GDT entries: 0 is the null entry, 1 is the kernel code entry, 2 is kernel date entry, 3 is the
	 * user code entry, 4 is the user date entry and, finally, 5 (and 6 on x86-64) is the TSS entry. */
	
	GdtSetGate(0, 0, 0, 0, 0);
	
	/* Btw, all the entries except for the null entry are subarch-specific). */
	
#ifdef ARCH_64
	GdtSetGate(1, 0, 0xFFFFFFFF, 0x98, 0x20);
	GdtSetGate(2, 0, 0xFFFFFFFF, 0x92, 0x00);
	GdtSetGate(3, 0, 0xFFFFFFFF, 0xF8, 0x20);
	GdtSetGate(4, 0, 0xFFFFFFFF, 0xF2, 0x00);
#else
	GdtSetGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
	GdtSetGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
	GdtSetGate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
	GdtSetGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
#endif
	
	GdtSetTSS(5, 0);
	
	/* Now, we just need to fill the GDT pointer, load the new GDT, reload all the segment registers, and flush the
	 * TSS. */
	
	GdtPointer.Limit = sizeof(GdtEntries) - 1;
	GdtPointer.Base = (UIntPtr)GdtEntries;
	
	asm volatile("lgdt %0" :: "m"(GdtPointer));
	asm volatile("mov $0x10, %ax; mov %ax, %ds; mov %ax, %es; mov %ax, %ss; \
				  mov %ax, %fs; mov %ax, %gs");
	
	/* Reloading the CS is a bit different on x86-64... */
	
#ifdef ARCH_64
	asm volatile("movabs $1f, %r8; push $0x08; push %r8; lretq; 1:");
#else
	asm volatile("ljmp $0x08, $1f; 1:");
#endif
	
	asm volatile("ltr %0" :: "r"((UInt16)0x2B));
}
