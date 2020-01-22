// File author is Ítalo Lima Marconato Matias
//
// Created on December 25 of 2019, at 10:56 BRT
// Last edited on January 21 of 2020, at 23:14 BRT

#ifndef __CHICAGO_SHM_H__
#define __CHICAGO_SHM_H__

#include <chicago/list.h>
#include <chicago/mm.h>
#include <chicago/virt.h>

#define SHM_EXIST_FLAGS VIRT_PROT_READ | VIRT_PROT_WRITE | VIRT_FLAGS_HIGHEST
#define SHM_NEW_FLAGS SHM_EXIST_FLAGS | VIRT_FLAGS_ALLOCNOW
#define SHM_MM_FLAGS MM_MAP_USER | MM_MAP_READ | MM_MAP_WRITE

typedef struct {
	UIntPtr key;
	UIntPtr refs;
	List pregions;
} ShmSection, *PShmSection;

typedef struct {
	PShmSection shm;
	UIntPtr virt;
} ShmMappedSection, *PShmMappedSection;

Status ShmCreateSection(UIntPtr size, PUIntPtr key, PUIntPtr ret);
Status ShmMapSection(UIntPtr key, PUIntPtr ret);
Status ShmUnmapSection(UIntPtr key);

#endif		// __CHICAGO_SHM_H__
