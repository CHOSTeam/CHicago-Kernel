// File author is Ítalo Lima Marconato Matias
//
// Created on December 25 of 2019, at 10:56 BRT
// Last edited on January 26 of 2020, at 13:40 BRT

#ifndef __CHICAGO_SHM_H__
#define __CHICAGO_SHM_H__

#include <chicago/list.h>
#include <chicago/mm.h>

#define SHM_EXIST_FLAGS MM_FLAGS_READ | MM_FLAGS_WRITE | MM_FLAGS_HIGHEST
#define SHM_NEW_FLAGS SHM_EXIST_FLAGS | MM_FLAGS_ALLOC_NOW
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
