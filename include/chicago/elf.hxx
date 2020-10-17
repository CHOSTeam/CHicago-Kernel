/* File author is Ítalo Lima Marconato Matias
 *
 * Created on October 09 of 2020, at 22:19 BRT
 * Last edited on October 17 of 2020, at 11:35 BRT */

#ifndef __CHICAGO_ELF_HXX__
#define __CHICAGO_ELF_HXX__

#include <chicago/fs.hxx>

/* CHicago uses ELF files for executables, libraries, and drivers, while this may change in the future, at
 * least for now, that's how it is going to be. ELF files contain some arch-specific things, and for those
 * we just use our IArch interface. Also, the main header itself is different depending on if it's a 32-bits
 * architecture, or a 64-bits one. */

#define ELF_MAGIC_0 0x7F
#define ELF_MAGIC_1 'E'
#define ELF_MAGIC_2 'L'
#define ELF_MAGIC_3 'F'

#define ELF_TYPE_REL 0x01
#define ELF_TYPE_DYN 0x03

#define ELF_SECT_TYPE_PROGBITS 0x01
#define ELF_SECT_TYPE_SYMTAB 0x02
#define ELF_SECT_TYPE_STRTAB 0x03
#define ELF_SECT_TYPE_RELA 0x04
#define ELF_SECT_TYPE_NOBITS 0x08
#define ELF_SECT_TYPE_REL 0x09

#define ELF_SYM_UNDEF 0x00
#define ELF_SYM_ABSOLUTE 0xFFF1
#define ELF_SYM_COMMON 0xFFF2

#define ELF_SYM_TYPE_NOTYPE 0x00
#define ELF_SYM_TYPE_SECTION 0x03
#define ELF_SYM_TYPE_FILE 0x04

#define ELF_SYM_BIND_GLOBAL 0x01
#define ELF_SYM_BIND_WEAK 0x02

#define ELF_GET_SECT(h, s, i) ((i) >= (h).SectHeaderCount ? Null : \
							   (Elf::SectHeader*)(&((s)[(i) * (h).SectHeaderEntSize])))

class Elf {
public:
	struct packed Header {
		UInt8 Ident[16];
		UInt16 Type, Machine;
		UInt32 Version;
		UIntPtr Entry;
		UIntPtr ProgHeaderOffset, SectHeaderOffset;
		UInt32 Flags;
		UInt16 HeaderSize;
		UInt16 ProgHeaderEntSize, ProgHeaderCount;
		UInt16 SectHeaderEntSize, SectHeaderCount;
		UInt16 SectNameIndex;
	};
	
	struct packed ProgHeader {
		UInt32 Type;
#ifdef ARCH_64
		UInt32 Flags;
#endif
		UIntPtr Offset, VirtAddress, PhysAddress, FileSize, MemSize;
#ifndef ARCH_64
		UInt32 Flags;
#endif
		UIntPtr Alignment;
	};
	
	struct packed SectHeader {
		UInt32 Name, Type;
		UIntPtr Flags;
		UIntPtr Address, Offset, Size;
		UInt32 Link, Info;
		UIntPtr Alignment;
		UIntPtr EntSize;
	};
	
	struct packed SymHeader {
		UInt32 Name;
#ifdef ARCH_64
		UInt8 Info, Other;
		UInt16 SectIndex;
#endif
		UIntPtr Value, Size;
#ifndef ARCH_64
		UInt8 Info, Other;
		UInt16 SectIndex;
#endif
	};
	
	struct packed RelHeader {
		UIntPtr Offset;
		UIntPtr Info;
	};
	
	struct Symbol {
		const String Name;
		Void* Base;
	};
	
	/* The constructor is responsible for loading the file (if required), checking if it's actually an ELF
	 * file of the specified type, loading all the dependencies, symbols, and etc.
	 * The destructor closes and frees everything (as you may already imagine), and there is one "companion"
	 * function to check if the open actually succeded. */
	
	Elf(const String&, UInt8);
	Elf(const Void*, UInt64, UInt8);
	
	~Elf(Void);
	
	const Symbol *GetSection(const String&) const;
	const Symbol *GetSymbol(const String&) const;
	
	Status GetLoadStatus(Void) const { return LoadStatus; }
private:
	Void Load(const File&, UInt8);
	
	Void *Base;
	Status LoadStatus;
	List<const Symbol*> Sections, Symbols;
};

#endif
