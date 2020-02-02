// File author is Ítalo Lima Marconato Matias
//
// Created on October 25 of 2018, at 20:11 BRT
// Last edited on February 02 of 2020, at 16:07 BRT

#ifndef __CHICAGO_ELF_H__
#define __CHICAGO_ELF_H__

#include <chicago/types.h>

#ifndef ELF_MACHINE
#error This architecture does not support the driver loader
#endif

#define ELF_SH_GET(h, i) ((PELFSHdr)((UIntPtr)((h)) + (h)->sh_off + ((h)->sh_ent_size * (i))))
#define ELF_PH_GET(h, i) ((PELFPHdr)((UIntPtr)((h)) + (h)->ph_off + ((h)->ph_ent_size * (i))))
#define ELF_SHPH_CONTENT(h, i) ((PVoid)((UIntPtr)((h)) + (i)->offset))

#if __INTPTR_WIDTH__ == 64
typedef struct {
	UInt8 ident[16];
	UInt16 type;
	UInt16 machine;
	UInt32 version;
	UInt64 entry;
	UInt64 ph_off;
	UInt64 sh_off;
	UInt32 flags;
	UInt16 eh_size;
	UInt16 ph_ent_size;
	UInt16 ph_num;
	UInt16 sh_ent_size;
	UInt16 sh_num;
	UInt16 sh_str_ndx;
} ELFHdr, *PELFHdr;

typedef struct {
	UInt32 name;
	UInt32 type;
	UInt64 flags;
	UInt64 addr;
	UInt64 offset;
	UInt64 size;
	UInt32 link;
	UInt32 info;
	UInt64 align;
	UInt64 ent_size;
} ELFSHdr, *PELFSHdr;

typedef struct {
	UInt32 type;
	UInt32 flags;
	UInt64 offset;
	UInt64 vaddr;
	UInt64 paddr;
	UInt64 fsize;
	UInt64 msize;
	UInt64 align;
} ELFPHdr, *PELFPHdr;

typedef struct {
	UInt32 name;
	UInt8 info;
	UInt8 other;
	UInt16 shndx;
	UInt64 value;
	UInt64 size;
} ELFSym, *PELFSym;

typedef struct {
	UInt64 offset;
	UInt64 info;
} ELFRel, *PELFRel;

typedef struct {
	UInt64 offset;
	UInt64 info;
	Int64 addend;
} ELFRelA, *PELFRelA;

typedef struct {
	Int64 tag;
	UInt64 val_ptr;
} ELFDyn, *PELFDyn;
#else
typedef struct {
	UInt8 ident[16];
	UInt16 type;
	UInt16 machine;
	UInt32 version;
	UInt32 entry;
	UInt32 ph_off;
	UInt32 sh_off;
	UInt32 flags;
	UInt16 eh_size;
	UInt16 ph_ent_size;
	UInt16 ph_num;
	UInt16 sh_ent_size;
	UInt16 sh_num;
	UInt16 sh_str_ndx;
} ELFHdr, *PELFHdr;

typedef struct {
	UInt32 name;
	UInt32 type;
	UInt32 flags;
	UInt32 addr;
	UInt32 offset;
	UInt32 size;
	UInt32 link;
	UInt32 info;
	UInt32 align;
	UInt32 ent_size;
} ELFSHdr, *PELFSHdr;

typedef struct {
	UInt32 type;
	UInt32 offset;
	UInt32 vaddr;
	UInt32 paddr;
	UInt32 fsize;
	UInt32 msize;
	UInt32 flags;
	UInt32 align;
} ELFPHdr, *PELFPHdr;

typedef struct {
	UInt32 name;
	UInt32 value;
	UInt32 size;
	UInt8 info;
	UInt8 other;
	UInt16 shndx;
} ELFSym, *PELFSym;

typedef struct {
	UInt32 offset;
	UInt32 info;
} ELFRel, *PELFRel;

typedef struct {
	UInt32 offset;
	UInt32 info;
	Int32 addend;
} ELFRelA, *PELFRelA;

typedef struct {
	Int32 tag;
	UInt32 val_ptr;
} ELFDyn, *PELFDyn;
#endif

#endif		// __CHICAGO_ELF_H__
