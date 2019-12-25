// File author is Ítalo Lima Marconato Matias
//
// Created on December 25 of 2019, at 10:56 BRT
// Last edited on December 25 of 2019, at 17:29 BRT

#ifndef __CHICAGO_SHM_H__
#define __CHICAGO_SHM_H__

#include <chicago/list.h>
#include <chicago/mm.h>
#include <chicago/virt.h>

#define SHM_NEW_FLAGS VIRT_PROT_READ | VIRT_PROT_WRITE | VIRT_FLAGS_HIGHEST
#define SHM_EXIST_FLAGS VIRT_FLAGS_AOR | SHM_NEW_FLAGS
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

UIntPtr ShmCreateSection(UIntPtr size, PUIntPtr key);
UIntPtr ShmMapSection(UIntPtr key);
Void ShmUnmapSection(UIntPtr key);
Void ShmInit(Void);

#endif		// __CHICAGO_SHM_H__
