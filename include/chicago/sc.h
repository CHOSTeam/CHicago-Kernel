// File author is Ítalo Lima Marconato Matias
//
// Created on November 16 of 2018, at 01:04 BRT
// Last edited on December 24 of 2019, at 12:59 BRT

#ifndef __CHICAGO_SC_H__
#define __CHICAGO_SC_H__

#include <chicago/exec.h>

#define HANDLE_TYPE_FILE 0x00
#define HANDLE_TYPE_THREAD 0x01
#define HANDLE_TYPE_PROCESS 0x02
#define HANDLE_TYPE_LOCK 0x03
#define HANDLE_TYPE_LIBRARY 0x04

typedef struct {
	PUInt32 major;
	PUInt32 minor;
	PUInt32 build;
	PWChar codename;
	PWChar arch;
} SystemVersion, *PSystemVersion;

typedef struct {
	IntPtr num;
	UInt8 type;
	PVoid data;
	Boolean used;
} Handle, *PHandle;

Void ScFreeHandle(UInt8 type, PVoid data);
Void ScSysGetVersion(PSystemVersion ver);
Void ScSysCloseHandle(IntPtr handle);
UIntPtr ScMmAllocMemory(UIntPtr size);
Void ScMmFreeMemory(UIntPtr block);
UIntPtr ScMmReallocMemory(UIntPtr block, UIntPtr size);
UIntPtr ScMmGetUsage(Void);
UIntPtr ScVirtAllocAddress(UIntPtr addr, UIntPtr size, UInt32 flags);
Boolean ScVirtFreeAddress(UIntPtr addr, UIntPtr size);
UInt32 ScVirtQueryProtection(UIntPtr addr);
Boolean ScVirtChangeProtection(UIntPtr addr, UIntPtr size, UInt32 flags);
UIntPtr ScVirtGetUsage(Void);
IntPtr ScPsCreateThread(UIntPtr entry);
IntPtr ScPsGetCurrentThread(Void);
IntPtr ScPsGetCurrentProcess(Void);
Void ScPsSleep(UIntPtr ms);
UIntPtr ScPsWait(IntPtr handle);
IntPtr ScPsCreateLock(Void);
Void ScPsLock(IntPtr handle);
Void ScPsUnlock(IntPtr handle);
Void ScPsExitThread(UIntPtr ret);
Void ScPsExitProcess(UIntPtr ret);
IntPtr ScFsOpenFile(PWChar path);
Boolean ScFsReadFile(IntPtr handle, UIntPtr len, PUInt8 buf);
Boolean ScFsWriteFile(IntPtr handle, UIntPtr len, PUInt8 buf);
Boolean ScFsMountFile(PWChar path, PWChar file, PWChar type);
Boolean ScFsUmountFile(PWChar path);
Boolean ScFsReadDirectoryEntry(IntPtr dir, UIntPtr entry, PWChar out);
IntPtr ScFsFindInDirectory(IntPtr dir, PWChar name);
Boolean ScFsCreateFile(IntPtr handle, PWChar name, UIntPtr type);
Boolean ScFsControlFile(IntPtr handle, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf);
UIntPtr ScFsGetFileSize(IntPtr handle);
UIntPtr ScFsGetPosition(IntPtr handle);
Void ScFsSetPosition(IntPtr handle, UIntPtr base, UIntPtr off);
IntPtr ScExecCreateProcess(PWChar path);
IntPtr ScExecLoadLibrary(PWChar path, Boolean global);
UIntPtr ScExecGetSymbol(IntPtr handle, PWChar name);

#endif		// __CHICAGO_SC_H__
