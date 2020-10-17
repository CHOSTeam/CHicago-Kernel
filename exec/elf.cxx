/* File author is Ítalo Lima Marconato Matias
 *
 * Created on October 09 of 2020, at 23:21 BRT
 * Last edited on October 17 of 2020, at 13:05 BRT */

#include <chicago/arch.hxx>
#include <chicago/siafs.hxx>
#include <chicago/textout.hxx>

Elf::Elf(const String &Path, UInt8 Type) {
	/* First, we need to check if the files exists. After that, we need to read/load it, we don't need to
	 * load all of it for now, as we're going to load header by header/section by section. */
	
	File file;
	
	if ((LoadStatus = FileSys::Open(Path, OPEN_READ, file)) == Status::Success) {
		/* But actually we're not going to do all the loading here, as the other constructor does the
		 * exactly same things that we would do here, so let's use one separate function for all the
		 * loading/checking. */
		
		Load(file, Type);
	}
}

Elf::Elf(const Void *Buffer, UInt64 Size, UInt8 Type) {
	/* For already loaded files, let's use the MemFs (the same that we used for SiaFs), and redirect into
	 * the Load function. */
	
	File file;
	
	if ((LoadStatus = MemFs::MakeFile((Void*)Buffer, Size, OPEN_READ, file)) == Status::Success) {
		Load(file, Type);
	}
}

Elf::~Elf(Void) {
	
}

Void Elf::Load(const File &Source, UInt8 Type) {
	if (LoadStatus != Status::Success) {
		return;
	} else if (Type != ELF_TYPE_REL && Type != ELF_TYPE_DYN) {
		LoadStatus = Status::WrongElf;
		return;
	}
	
	/* Load the header, and check if the magic number is the right one, if the architecture is the
	 * right one, if the type is the one that the user expected, etc. */
	
	UInt64 read;
	Elf::Header hdr;
	
	if ((LoadStatus = Source.Read(0, sizeof(hdr), &hdr, read)) != Status::Success) {
		return;
	} else if (read != sizeof(hdr)) {
		LoadStatus = Status::ReadFailed;
		return;
	} else if (hdr.Ident[0] != ELF_MAGIC_0 || hdr.Ident[1] != ELF_MAGIC_1 ||
			   hdr.Ident[2] != ELF_MAGIC_2 || hdr.Ident[3] != ELF_MAGIC_3) {
		LoadStatus = Status::NotElf;
		return;
	} else if (hdr.Ident[4] != ELF_CLASS || hdr.Ident[5] != ELF_DATA ||
			   hdr.Machine != ELF_MACHINE) {
		LoadStatus = Status::InvalidArch;
		return;
	} else if (hdr.Type != Type) {
		LoadStatus = Status::WrongElf;
		return;
	}
	
	/* Now it's time to load all the sections, but there is a catch (of course), on DYN files, we can
	 * use the program headers, but on REL files, we need to use the section headers. We still want to
	 * iterate over the section headers (but later on DYN files) to save info about where each section
	 * starts.
	 * But as we need to use the section headers anyway, let's load all of them. */
	
	UInt8 *shdrs = new UInt8[hdr.SectHeaderCount * hdr.SectHeaderEntSize], *symtab = Null;
	Char *strtab = Null, *snames = Null;
	Elf::SectHeader *shdr = Null;
	UIntPtr symc = 0, syment = 0;
	
	if (shdrs == Null) {
		LoadStatus = Status::OutOfMemory;
		return;
	} else if ((LoadStatus = Source.Read(hdr.SectHeaderOffset,
										 hdr.SectHeaderCount * hdr.SectHeaderEntSize,
										 shdrs, read)) != Status::Success) {
		goto c;
	} else if (read != hdr.SectHeaderCount * hdr.SectHeaderEntSize) {
		LoadStatus = Status::ReadFailed;
		goto c;
	}
	
	/* Also, we need the shstrtab section, as it contains the name of each section (and try to guess?
	 * We need the name of the sections, both for possible reallocations, and for searching specific
	 * sections). */
	
	shdr = (Elf::SectHeader*)(&shdrs[hdr.SectNameIndex * hdr.SectHeaderEntSize]);
	
	if ((snames = new Char[shdr->Size]) == Null) {
		LoadStatus = Status::OutOfMemory;
		goto c;
	} else if ((LoadStatus = Source.Read(shdr->Offset, shdr->Size, snames,
										 read)) != Status::Success) {
		goto c;
	} else if (read != shdr->Size) {
		LoadStatus = Status::ReadFailed;
		goto c;
	} else if (Type == ELF_TYPE_REL) {
		/* REL: Calculate how much memory we need to allocate (using the size of each section that we
		 * need to load), alloc the memory, and load everything up while adding the list entries. */
		
		UIntPtr size = 0;
		
		for (UInt16 i = 0; i < hdr.SectHeaderCount; i++) {
			shdr = ELF_GET_SECT(hdr, shdrs, i);
			
			if (shdr->Type == ELF_SECT_TYPE_PROGBITS || shdr->Type == ELF_SECT_TYPE_NOBITS) {
				size += shdr->Size;
			}
		}
		
		Void *sbase = Base = new UInt8[size];
		
		if (Base == Null) {
			LoadStatus = Status::OutOfMemory;
			goto c;
		}
		
		for (UInt16 i = 0; i < hdr.SectHeaderCount; i++) {
			shdr = ELF_GET_SECT(hdr, shdrs, i);
			
			/* Progbits is proper loadable data (.code, .data etc), Nobits is bss/data that can be
			 * initialized as all zeroes. Size == 0 means that we can skip this entry (else we gonna
			 * get InvalidArg). */
			
			if (shdr->Size == 0) {
				continue;
			} else if (shdr->Type == ELF_SECT_TYPE_PROGBITS) {
				if ((LoadStatus = Source.Read(shdr->Offset, shdr->Size, sbase,
											  read)) != Status::Success) {
					goto c;
				} else if (read != shdr->Size) {
					LoadStatus = Status::ReadFailed;
					goto c;
				}
			} else if (shdr->Type == ELF_SECT_TYPE_NOBITS) {
				StrSetMemory(sbase, 0, shdr->Size);
			} else {
				continue;
			}
			
			/* Initialize the section structure, and save it into the list. */
			
			Elf::Symbol *sect = new Elf::Symbol(String(&snames[shdr->Name], True), sbase);
			
			if (sect == Null) {
				LoadStatus = Status::OutOfMemory;
				goto c;
			} else if ((LoadStatus = Sections.Add(sect)) != Status::Success) {
				delete sect;
				goto c;
			}
			
			/* Save the address of this section into the section header itself, so that the
			 * relocations can be done a bit more easily. */
			
			shdr->Address = (UIntPtr)sbase;
			sbase = &((UInt8*)sbase)[shdr->Size];
		}
	} else {
		/* DYN: Still unsupported, add it later (when we start adding userspace support). */
		
#ifdef DBG
		Debug->Write("[Elf]: Unsupported DYN format\n");
#endif
		
		LoadStatus = Status::WrongElf;
		goto c;
	}
	
	/* Load up the symbol table, and add all the symbols, this is required before moving forward. Also
	 * load the string table, as the name of the symbols are on it. */
	
	for (UInt16 i = 0; i < hdr.SectHeaderCount; i++) {
		shdr = ELF_GET_SECT(hdr, shdrs, i);
		
		/* There should be only one symtab, so we can use the first one we find, but there are (or at
		 * least there should be) two strtabs, one for section names, and other for everything else, we
		 * already loaded the first one, so we want the second one. */
		
		if (shdr->Type == ELF_SECT_TYPE_SYMTAB && symtab == Null) {
			syment = shdr->EntSize;
			symc = shdr->Size / syment;
			
			if ((symtab = new UInt8[shdr->Size]) == Null) {
				LoadStatus = Status::OutOfMemory;
				goto c;
			} else if ((LoadStatus = Source.Read(shdr->Offset, shdr->Size, symtab,
												 read)) != Status::Success) {
				goto c;
			} else if (read != shdr->Size) {
				LoadStatus = Status::ReadFailed;
				goto c;
			}
		} else if (shdr->Type == ELF_SECT_TYPE_STRTAB && i != hdr.SectNameIndex && strtab == Null) {
			if ((strtab = new Char[shdr->Size]) == Null) {
				LoadStatus = Status::OutOfMemory;
				goto c;
			} else if ((LoadStatus = Source.Read(shdr->Offset, shdr->Size, strtab,
												 read)) != Status::Success) {
				goto c;
			} else if (read != shdr->Size) {
				LoadStatus = Status::ReadFailed;
				goto c;
			}
		}
	}
	
	/* Now, go ahead and add all the symbols to the symbol table, on REL files, we need to use the
	 * SectIndex field, as the offset of the variable is relative to the section, but on DYN files, it is
	 * relative to the base/absolute. */
	
	for (UInt16 i = 0; i < symc; i++) {
		Elf::SymHeader *sym = (Elf::SymHeader*)((UIntPtr)symtab + i * syment);
		UInt8 type = sym->Info & 0x0F, bind = sym->Info >> 4;
		Void *sbase = Null;
		
		if (sym->SectIndex == ELF_SYM_UNDEF || strtab[sym->Name] == 0 ||
			type == ELF_SYM_TYPE_NOTYPE || type == ELF_SYM_TYPE_SECTION ||
			type == ELF_SYM_TYPE_FILE) {
			/* Empty symbols (or symbols with empty name) should be skipped. NOTYPE, SECTION and FILE
			 * symbols should also be skipped (as they are useless for us). */
			
			continue;
		} else if (sym->SectIndex == ELF_SYM_COMMON) {
			/* There should be NO symbols defined as COMMON, even on REL files, if there is, this is not a
			 * valid ELF file (for us). */
			
#ifdef DBG
			Debug->Write("[Elf] REL file contains symbol defined as COMMON\n");
#endif
			
			LoadStatus = Status::CorruptedElf;
			goto c;
		} else if (sym->SectIndex == ELF_SYM_ABSOLUTE) {
			/* Absolute address, this one is the easiest one to handle. */
			
			sbase = (Void*)sym->Value;
		} else if (sym->SectIndex == ELF_SYM_UNDEF) {
			/* External symbol, unless this is a weak symbol, fail out, as the driver can't (for now)
			 * import kernel symbols/symbols from other drivers (unless it uses the Interface funcs).
			 * CHANGE THIS AFTER DYN SUPPORT IS ADDED, THIS PART WILL NEED TO BE COMPLETLY REDONE,
			 * probably. */
			
			if (!(bind & ELF_SYM_BIND_WEAK)) {
#ifdef DBG
				Debug->Write("[Elf]: Tried to import external symbol.");
#endif
				
				LoadStatus = Status::CorruptedElf;
				goto c;
			}
		} else {
			/* Ok, one symbol that at least we have to "fix"/add the section start to the value, and, of
			 * course, if it is an global symbol we need to add it to the list.
			 * THIS PART ALSO NEEDS TO BE REDONE AFTER DYN SUPPORT. */
			
			sbase = (Void*)(sym->Value +=
							((Elf::SectHeader*)&shdrs[sym->SectIndex *
													  hdr.SectHeaderEntSize])->Address);
			
			if (!(bind & ELF_SYM_BIND_GLOBAL)) {
				continue;
			}
		}
		
		/* If we reached this place, this is a global symbol that we need to add, the address is already
		 * saved in sbase, so we just need to do the same that we did for the sections. */
		
		Elf::Symbol *rsym = new Elf::Symbol(String(&strtab[sym->Name], True), sbase);
		
		if (rsym == Null) {
			LoadStatus = Status::OutOfMemory;
			goto c;
		} else if ((LoadStatus = Symbols.Add(rsym)) != Status::Success) {
			delete rsym;
			goto c;
		}
	}
	
	/* Last thing that we need to do: Do all the relocations, this is arch-specific, so while we're going
	 * to iterate and get the .rel/.rela sections, the arch-specific function is going to do the actual
	 * relocation.
	 * TODO: When adding DYN support, we also need to get the entry point, load all the required
	 * libraries, and correctly handle the symbols/sections. */
	
	for (UInt16 i = 0; i < hdr.SectHeaderCount; i++) {
		shdr = ELF_GET_SECT(hdr, shdrs, i);
		
		if (shdr->Type != ELF_SECT_TYPE_RELA && shdr->Type != ELF_SECT_TYPE_REL) {
			continue;
		}
		
		UInt8 *rel = new UInt8[shdr->Size], *cur = rel, *end = rel + shdr->Size;
		
		if (rel == Null) {
			LoadStatus = Status::OutOfMemory;
			goto c;
		} else if ((LoadStatus = Source.Read(shdr->Offset, shdr->Size, rel,
											 read)) != Status::Success) {
			delete rel;
			goto c;
		}
		
		/* Now that we loaded the section, let's call Arch->DoElfRelocation for each of the entries
		 * in it. */
		
		Elf::SectHeader *rsect = ELF_GET_SECT(hdr, shdrs, shdr->Info);
		
		for (; cur < end; cur += shdr->EntSize) {
			if ((LoadStatus = Arch->DoElfRelocation(shdr->Type, rsect, symtab,
													(Elf::RelHeader*)cur)) != Status::Success) {
				delete rel;
				goto c;
			}
		}
		
		delete rel;
	}
	
	/* Set the load status to AlreadyLoaded, indicating to the user/ourselves that the everything went
	 * well, and to ourselves that we should block any other call to Load. */
	
	LoadStatus = Status::AlreadyLoaded;
	
	return;

c:	/* Clean up section, only for errors, deallocate everything that we allocated (except things that
	 * the deconstructor will deallocate for us), and return. */
	
	if (symtab != Null) {
		delete symtab;
	}
	
	if (strtab != Null) {
		delete strtab;
	}
	
	if (snames != Null) {
		delete snames;
	}
	
	if (shdrs != Null) {
		delete shdrs;
	}
}
