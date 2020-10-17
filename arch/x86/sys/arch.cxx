/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 25 of 2020, at 09:42 BRT
 * Last edited on October 17 of 2020, at 13:05 BRT */

#include <chicago/arch/arch.hxx>
#include <chicago/textout.hxx>

ArchImpl ArchImp;

Void ArchImpl::Halt(Void) {
	/* The halt instruction actually returns when we get some interrupt, so we need to keep on calling
	 * it in a loop, instead of just calling it once. */
	
	while (True) {
		asm volatile("hlt");
	}
}

Status ArchImpl::DoElfRelocation(UInt32 Type, const Elf::SectHeader *Section, const UInt8 *SymTab,
								 const Elf::RelHeader *Ent) {
	/* TODO: Change this to add the required relocations for DYN files (and to handle the diff between
	 * DYN and REL). */
	
	if ((Type != ELF_SECT_TYPE_REL && Type != ELF_SECT_TYPE_RELA) || Section == Null ||
		SymTab == Null) {
		/* We're not supposed to reach this place, if we did, or the elf loader is broken, or something
		 * else called us. */
		
		return Status::InvalidArg;
	}
	
	/* Calculate the base and value (without the addend), this is global for both REL and RELA sects */
	
#ifdef ARCH_64
	UInt32 sym = Ent->Info >> 32, type = (UInt32)Ent->Info;
#else
	UInt32 sym = Ent->Info >> 8, type = (UInt8)Ent->Info;
#endif
	
	UIntPtr val = sym ? ((Elf::SymHeader*)&SymTab[sym])->Value : 0;
	UIntPtr *base = (UIntPtr*)(Section->Address + Ent->Offset);
	UInt32 *base32 = (UInt32*)base;
#ifdef ARCH_64
	Int32 *base32s = (Int32*)base;
#endif
	
	/* Now, just for RELA sects, add the addend (it is just after the main rel header, and it is
	 * always a single IntPtr). */
	
	if (Type == ELF_SECT_TYPE_RELA) {
		val += *((IntPtr*)((UIntPtr)Ent + sizeof(Ent)));
		
		/* Different switch for RELA, as we need to just assign, instead of +=. */
		
		switch (type) {
		case ELF_REL_NONE: break;
		case ELF_REL_PTRWORD: *base = val; break;
		case ELF_REL_PC32: *base32 = (UInt32)(val - (UIntPtr)base); break;
#ifdef ARCH_64
		case ELF_REL_PTR32: *base32 = (UInt32)val; break;
		case ELF_REL_PTR32S: *base32s = (Int32)val; break;
		case ELF_REL_PC64: *base = val - (UIntPtr)base; break;
#endif
		default: {
			/* Found some invalid/still unsupported relocation, bail out, don't wanna have the chance
			* of this being a still unsupported one, and not doing it breaking the executable. */
		
#ifdef DBG
			Debug->Write("[Elf] Invalid/Unsupported relocation type 0x%x\n", type);
#endif
		
			return Status::Unsupported;
		}
		}
		
		return Status::Success;
	}
	
	/* REL section, we need to += instead of just assigning (we're probably only going to reach this
	 * part of the code on x86-32). */
	
	switch (type) {
	case ELF_REL_NONE: break;
	case ELF_REL_PTRWORD: *base += val; break;
	case ELF_REL_PC32: *base32 += (UInt32)(val - (UIntPtr)base); break;
#ifdef ARCH_64
	case ELF_REL_PTR32: *base32 += (UInt32)val; break;
	case ELF_REL_PTR32S: *base32s += (Int32)val; break;
	case ELF_REL_PC64: *base += val - (UIntPtr)base; break;
#endif
	default: {
		/* Found some invalid/still unsupported relocation, bail out, don't wanna have the chance
		* of this being a still unsupported one, and not doing it breaking the executable. */
	
#ifdef DBG
		Debug->Write("[Elf] Invalid/Unsupported relocation type 0x%x\n", type);
#endif
	
		return Status::Unsupported;
	}
	}
	
	return Status::Success;
}

Void InitArchInterface(Void) {
	Arch = &ArchImp;
}
