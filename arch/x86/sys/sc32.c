// File author is Ítalo Lima Marconato Matias
//
// Created on November 16 of 2018, at 00:48 BRT
// Last edited on January 05 of 2020, at 21:58 BRT

#include <chicago/arch/idt.h>
#include <chicago/sc.h>

static Void ArchScHandler(PRegisters regs) {
	switch (regs->eax) {
	case 0x00: {																								// Void SysGetVersion(PSystemRegisters regs)
		ScSysGetVersion((PSystemVersion)regs->ebx);
		break;
	}
	case 0x01: {																								// PInt SysGetErrno(Void)
		regs->eax = (UIntPtr)ScSysGetErrno();
		break;
	}
	case 0x02: {																								// Void SysCloseHandle(IntPtr handle)
		ScSysCloseHandle(regs->ebx);
		break;
	}
	case 0x03: {																								// UIntPtr VirtAllocAddress(UIntPtr addr, UIntPtr size, UInt32 flags)
		regs->eax = ScVirtAllocAddress(regs->ebx, regs->ecx, regs->edx);
		break;
	}
	case 0x04: {																								// Boolean VirtFreeAddress(UIntPtr addr, UIntPtr size)
		regs->eax = ScVirtFreeAddress(regs->ebx, regs->ecx);
		break;
	}
	case 0x05: {																								// UInt32 VirtQueryProtection(UIntPtr addr)
		regs->eax = ScVirtQueryProtection(regs->ebx);
		break;
	}
	case 0x06: {																								// Boolean VirtChangeProtection(UIntPtr addr, UIntPtr size, UInt32 flags)
		regs->eax = ScVirtChangeProtection(regs->ebx, regs->ecx, regs->edx);
		break;
	}
	case 0x07: {																								// UIntPtr VirtGetUsage(Void)
		regs->eax = ScVirtGetUsage();
		break;
	}
	case 0x08: {																								// IntPtr PsCreateThread(UIntPtr entry)
		regs->eax = ScPsCreateThread(regs->ebx);
		break;
	}
	case 0x09: {																								// IntPtr PsGetCurrentThread(Void)
		regs->eax = ScPsGetCurrentThread();
		break;
	}
	case 0x0A: {																								// IntPtr PsGetCurrentProcess(Void)
		regs->eax = ScPsGetCurrentProcess();
		break;
	}
	case 0x0B: {																								// Void PsSleep(UIntPtr ms)
		ScPsSleep(regs->ebx);
		break;
	}
	case 0x0C: {																								// UIntPtr PsWait(IntPtr handle)
		regs->eax = ScPsWait(regs->ebx);
		break;
	}
	case 0x0D: {																								// IntPtr PsCreateLock(Void)
		regs->eax = ScPsCreateLock();
		break;
	}
	case 0x0E: {																								// Void PsLock(IntPtr handle)
		ScPsLock(regs->ebx);
		break;
	}
	case 0x0F: {																								// Boolean PsTryLock(IntPtr handle)
		regs->eax = ScPsTryLock(regs->ebx);
		break;
	}
	case 0x10: {																								// Void PsUnlock(IntPtr handle)
		ScPsUnlock(regs->ebx);
		break;
	}
	case 0x11: {																								// Void PsExitThread(UIntPtr ret)
		ScPsExitThread(regs->ebx);
		break;
	}
	case 0x12: {																								// Void PsExitProcess(UIntPtr ret)
		ScPsExitProcess(regs->ebx);
		break;
	}
	case 0x13: {																								// IntPtr FsOpenFile(PWChar path)
		regs->eax = ScFsOpenFile((PWChar)regs->ebx);
		break;
	}
	case 0x14: {																								// UIntPtr FsReadFile(IntPtr handle, UIntPtr size, PUInt8 buf)
		regs->eax = ScFsReadFile(regs->ebx, regs->ecx, (PUInt8)regs->edx);
		break;
	}
	case 0x15: {																								// UIntPtr FsWriteFile(IntPtr handle, UIntPtr size, PUInt8 buf)
		regs->eax = ScFsWriteFile(regs->ebx, regs->ecx, (PUInt8)regs->edx);
		break;
	}
	case 0x16: {																								// Boolean FsMountFile(PWChar path, PWChar file, PWChar type)
		regs->eax = ScFsMountFile((PWChar)regs->ebx, (PWChar)regs->ecx, (PWChar)regs->edx);
		break;
	}
	case 0x17: {																								// Boolean FsUmountFile(PWChar path)
		regs->eax = ScFsUmountFile((PWChar)regs->ebx);
		break;
	}
	case 0x18: {																								// Boolean FsReadDirectoryEntry(IntPtr handle, UIntPtr entry, PWChar out)
		regs->eax = ScFsReadDirectoryEntry(regs->ebx, regs->ecx, (PWChar)regs->edx);
		break;
	}
	case 0x19: {																								// IntPtr FsFindInDirectory(IntPtr handle, PWChar name)
		regs->eax = ScFsFindInDirectory(regs->ebx, (PWChar)regs->ecx);
		break;
	}
	case 0x1A: {																								// Boolean FsCreateFile(IntPtr handle, PWChar name, UIntPtr type)
		regs->eax = ScFsCreateFile(regs->ebx, (PWChar)regs->ecx, regs->edx);
		break;
	}
	case 0x1B: {																								// Boolean FsControlFile(IntPtr handle, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf)
		regs->eax = ScFsControlFile(regs->ebx, regs->ecx, (PUInt8)regs->edx, (PUInt8)regs->esi);
		break;
	}
	case 0x1C: {																								// UIntPtr FsGetSize(IntPtr handle)
		regs->eax = ScFsGetFileSize(regs->ebx);
		break;
	}
	case 0x1D: {																								// UIntPtr FsGetPosition(IntPtr handle)
		regs->eax = ScFsGetPosition(regs->ebx);
		break;
	}
	case 0x1E: {																								// Boolean FsSetPosition(IntPtr handle, IntPtr base, UIntPtr off)
		ScFsSetPosition(regs->ebx, regs->ecx, regs->edx);
		break;
	}
	case 0x1F: {																								// IntPtr ExecCreateProcess(PWChar path, UIntPtr argc, PWChar *argv)
		regs->eax = ScExecCreateProcess((PWChar)regs->ebx, regs->ecx, (PWChar*)regs->edx);
		break;
	}
	case 0x20: {																								// IntPtr ExecLoadLibrary(PWChar path, Boolean global)
		regs->eax = ScExecLoadLibrary((PWChar)regs->ebx, regs->ecx);
		break;
	}
	case 0x21: {																								// UIntPtr ExecGetSymbol(IntPtr handle, PWChar name)
		regs->eax = ScExecGetSymbol(regs->ebx, (PWChar)regs->ecx);
		break;
	}
	case 0x22: {																								// Boolean IpcCreatePort(PWChar name)
		regs->eax = ScIpcCreatePort((PWChar)regs->ebx);
		break;
	}
	case 0x23: {																								// IntPtr IpcCreateResponsePort(Void)
		regs->eax = ScIpcCreateResponsePort();
		break;
	}
	case 0x24: {																								// Void IpcRemovePort(PWchar name)
		ScIpcRemovePort((PWChar)regs->ebx);
		break;
	}
	case 0x25: {																								// Boolean IpcCheckPort(PWChar name)
		regs->eax = ScIpcCheckPort((PWChar)regs->ebx);
		break;
	}
	case 0x26: {																								// PIpcMessage IpcSendMessage(PWChar port, UInt32 msg, UIntPtr size, PUInt8 buf, IntPtr rport)
		regs->eax = (UIntPtr)ScIpcSendMessage((PWChar)regs->ebx, regs->ecx, regs->edx, (PUInt8)regs->esi, regs->edi);
		break;
	}
	case 0x27: {																								// Void IpcSendResponse(IntPtr handle, UInt32 msg, UIntPtr size, PUInt8 buf)
		ScIpcSendResponse(regs->ebx, regs->ecx, regs->edx, (PUInt8)regs->esi);
		break;
	}
	case 0x28: {																								// PIpcMessage IpcReceiveMessage(PWChar name)
		regs->eax = (UIntPtr)ScIpcReceiveMessage((PWChar)regs->ebx);
		break;
	}
	case 0x29: {																								// PIpcMessage IpcReceiveResponse(IntPtr handle)
		regs->eax = (UIntPtr)ScIpcReceiveResponse(regs->ebx);
		break;
	}
	case 0x2A: {																								// UIntPtr ShmCreateSection(UIntPtr size, PUIntPtr key)
		regs->eax = ScShmCreateSection(regs->ebx, (PUIntPtr)regs->ecx);
		break;
	}
	case 0x2B: {																								// UIntPtr ShmMapSection(UIntPtr key)
		regs->eax = ScShmMapSection(regs->ebx);
		break;
	}
	case 0x2C: {																								// Void ShmUnmapSection(UIntPtr key)
		ScShmUnmapSection(regs->ebx);
		break;
	}
	default: {
		regs->eax = (UIntPtr)-1;
		break;
	}
	}
}

Void ArchInitSc(Void) {
	IDTRegisterInterruptHandler(0x3F, ArchScHandler);
}
