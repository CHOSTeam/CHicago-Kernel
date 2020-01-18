// File author is Ítalo Lima Marconato Matias
//
// Created on November 16 of 2018, at 01:04 BRT
// Last edited on January 18 of 2020, at 10:12 BRT

#ifndef __CHICAGO_SC_H__
#define __CHICAGO_SC_H__

#include <chicago/ipc.h>

#define HANDLE_TYPE_FILE 0x00
#define HANDLE_TYPE_THREAD 0x01
#define HANDLE_TYPE_PROCESS 0x02
#define HANDLE_TYPE_LOCK 0x03
#define HANDLE_TYPE_IPC_RESPONSE_PORT 0x04

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

IntPtr ScAppendHandle(UInt8 type, PVoid data);
Void ScFreeHandle(UInt8 type, PVoid data);
Void ScSysGetVersion(PSystemVersion ver);
PInt ScSysGetErrno(Void);
Void ScSysCloseHandle(IntPtr handle);
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
Boolean ScPsTryLock(IntPtr handle);
Void ScPsUnlock(IntPtr handle);
Void ScPsExitThread(UIntPtr ret);
Void ScPsExitProcess(UIntPtr ret);
IntPtr ScFsOpenFile(PWChar path);
UIntPtr ScFsReadFile(IntPtr handle, UIntPtr len, PUInt8 buf);
UIntPtr ScFsWriteFile(IntPtr handle, UIntPtr len, PUInt8 buf);
Boolean ScFsMountFile(PWChar path, PWChar file, PWChar type);
Boolean ScFsUmountFile(PWChar path);
Boolean ScFsReadDirectoryEntry(IntPtr dir, UIntPtr entry, PWChar out);
IntPtr ScFsFindInDirectory(IntPtr dir, PWChar name);
Boolean ScFsCreateFile(IntPtr handle, PWChar name, UIntPtr type);
Boolean ScFsControlFile(IntPtr handle, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf);
UIntPtr ScFsGetFileSize(IntPtr handle);
UIntPtr ScFsGetPosition(IntPtr handle);
Void ScFsSetPosition(IntPtr handle, UIntPtr base, UIntPtr off);
Boolean ScIpcCreatePort(PWChar name);
IntPtr ScIpcCreateResponsePort(Void);
Void ScIpcRemovePort(PWChar name);
Boolean ScIpcCheckPort(PWChar name);
Boolean ScIpcSendMessage(PWChar port, UInt32 msg, UIntPtr size, PUInt8 buf, IntPtr rport);
Boolean ScIpcSendResponse(IntPtr handle, UInt32 msg, UIntPtr size, PUInt8 buf);
Boolean ScIpcReceiveMessage(PWChar name, PUInt32 msg, UIntPtr size, PUInt8 buf);
Boolean ScIpcReceiveResponse(IntPtr handle, PUInt32 msg, UIntPtr size, PUInt8 buf);
UIntPtr ScShmCreateSection(UIntPtr size, PUIntPtr key);
UIntPtr ScShmMapSection(UIntPtr key);
Void ScShmUnmapSection(UIntPtr key);

#endif		// __CHICAGO_SC_H__
