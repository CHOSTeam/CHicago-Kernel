// File author is Ítalo Lima Marconato Matias
//
// Created on November 10 of 2018, at 21:11 BRT
// Last edited on August 27 of 2019, at 14:46 BRT

#include <chicago/chexec.h>
#include <chicago/mm.h>
#include <chicago/string.h>
#include <chicago/virt.h>

Boolean CHExecValidateHeader(PUInt8 buf, UInt8 type) {
	if (buf == Null) {																							// Valid buffer?
		return False;																							// No...
	}
	
	PCHExecHeader hdr = (PCHExecHeader)buf;

	if (hdr->magic != CHEXEC_HEADER_MAGIC) {																	// Valid magic?
		return False;																							// No...
	}
	
	if ((hdr->flags & CHEXEC_ARCH) != CHEXEC_ARCH) {															// Valid arch?
		return False;																							// No...
	}
	
	return (hdr->flags & type) == type;																			// Check the type
}

UIntPtr CHExecLoadSections(PUInt8 buf) {
	if (buf == Null) {																							// Valid buffer?
		return 0;																								// No...
	}
	
	PCHExecHeader hdr = (PCHExecHeader)buf;
	PCHExecSection sh = (PCHExecSection)(((UIntPtr)buf) + hdr->sh_start);
	UIntPtr size = 0;
	
	for (UIntPtr i = 0; i < hdr->sh_count; i++) {																// First, we need to know how much memory we need to alloc!
		if ((sh->virt + sh->size) > size) {
			size = sh->virt + sh->size;	
		}
		
		sh = (PCHExecSection)(((UIntPtr)sh) + sizeof(CHExecSection) + (sh->name_len * sizeof(WChar)));
	}
	
	UIntPtr base = MmAllocUserMemory(size);																		// Now, let's alloc it
	
	if (base == 0) {
		return 0;																								// Failed...
	} else if (!VirtChangeProtection(base, size, VIRT_PROT_READ | VIRT_PROT_WRITE | VIRT_PROT_EXEC)) {			// Add the exec permission/flag
		MmFreeUserMemory(base);
		return 0;
	}
	
	sh = (PCHExecSection)(((UIntPtr)buf) + hdr->sh_start);
	
	for (UIntPtr i = 0; i < hdr->sh_count; i++) {																// And let's load!
		if ((sh->flags & CHEXEC_SECTION_FLAGS_ZEROINIT) == CHEXEC_SECTION_FLAGS_ZEROINIT) {						// BSS?
			StrSetMemory((PUInt8)(base + sh->virt), 0, sh->size);												// Yes, just zero it
		} else {
			StrCopyMemory((PUInt8)(base + sh->virt), (PUInt8)(((UIntPtr)buf) + sh->offset), sh->size);			// No, copy it!
		}
		
		sh = (PCHExecSection)(((UIntPtr)sh) + sizeof(CHExecSection) + (sh->name_len * sizeof(WChar)));
	}
	
	return base;
}
