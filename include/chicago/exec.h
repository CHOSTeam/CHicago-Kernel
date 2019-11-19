// File author is Ítalo Lima Marconato Matias
//
// Created on November 02 of 2018, at 12:09 BRT
// Last edited on November 16 of 2019, at 11:33 BRT

#ifndef __CHICAGO_EXEC_H__
#define __CHICAGO_EXEC_H__

#include <chicago/process.h>

typedef struct {
	PWChar name;
	UIntPtr loc;
} ExecSymbol, *PExecSymbol;

typedef struct {
	PList symbols;
	PList deps;
	PWChar name;
	UIntPtr refs;
	UIntPtr base;
	Boolean resolved;
} LibHandle, *PLibHandle;

typedef struct {
	PList symbols;
	PList deps;
	UIntPtr base;
} ExecHandle, *PExecHandle;

PProcess ExecCreateProcess(PWChar path);
PLibHandle ExecLoadLibrary(PWChar path, Boolean global);
Void ExecCloseLibrary(PLibHandle handle);
UIntPtr ExecGetSymbol(PLibHandle handle, PWChar name);

#endif		// __CHICAGO_EXEC_H__
