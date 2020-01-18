// File author is Ítalo Lima Marconato Matias
//
// Created on October 29 of 2018, at 18:10 BRT
// Last edited on January 18 of 2020, at 10:12 BRT

#include <chicago/arch/idt.h>
#include <chicago/sc.h>

static Void ArchScHandler(PRegisters regs) {
	switch (regs->rax) {
	case 0x00: {																								// Void SysGetVersion(PSystemRegisters regs)
		ScSysGetVersion((PSystemVersion)regs->rbx);
		break;
	}
	case 0x01: {																								// PInt SysGetErrno(Void)
		regs->rax = (UIntPtr)ScSysGetErrno();
		break;
	}
	case 0x02: {																								// Void SysCloseHandle(IntPtr handle)
		ScSysCloseHandle(regs->rbx);
		break;
	}
	case 0x03: {																								// UIntPtr VirtAllocAddress(UIntPtr addr, UIntPtr size, UInt32 flags)
		regs->rax = ScVirtAllocAddress(regs->rbx, regs->rcx, regs->rdx);
		break;
	}
	case 0x04: {																								// Boolean VirtFreeAddress(UIntPtr addr, UIntPtr size)
		regs->rax = ScVirtFreeAddress(regs->rbx, regs->rcx);
		break;
	}
	case 0x05: {																								// UInt32 VirtQueryProtection(UIntPtr addr)
		regs->rax = ScVirtQueryProtection(regs->rbx);
		break;
	}
	case 0x06: {																								// Boolean VirtChangeProtection(UIntPtr addr, UIntPtr size, UInt32 flags)
		regs->rax = ScVirtChangeProtection(regs->rbx, regs->rcx, regs->rdx);
		break;
	}
	case 0x07: {																								// UIntPtr VirtGetUsage(Void)
		regs->rax = ScVirtGetUsage();
		break;
	}
	case 0x08: {																								// IntPtr PsCreateThread(UIntPtr entry)
		regs->rax = ScPsCreateThread(regs->rbx);
		break;
	}
	case 0x09: {																								// IntPtr PsGetCurrentThread(Void)
		regs->rax = ScPsGetCurrentThread();
		break;
	}
	case 0x0A: {																								// IntPtr PsGetCurrentProcess(Void)
		regs->rax = ScPsGetCurrentProcess();
		break;
	}
	case 0x0B: {																								// Void PsSleep(UIntPtr ms)
		ScPsSleep(regs->rbx);
		break;
	}
	case 0x0C: {																								// UIntPtr PsWait(IntPtr handle)
		regs->rax = ScPsWait(regs->rbx);
		break;
	}
	case 0x0D: {																								// IntPtr PsCreateLock(Void)
		regs->rax = ScPsCreateLock();
		break;
	}
	case 0x0E: {																								// Void PsLock(IntPtr handle)
		ScPsLock(regs->rbx);
		break;
	}
	case 0x0F: {																								// Boolean PsTryLock(IntPtr handle)
		regs->rax = ScPsTryLock(regs->rbx);
		break;
	}
	case 0x10: {																								// Void PsUnlock(IntPtr handle)
		ScPsUnlock(regs->rbx);
		break;
	}
	case 0x11: {																								// Void PsExitThread(UIntPtr ret)
		ScPsExitThread(regs->rbx);
		break;
	}
	case 0x12: {																								// Void PsExitProcess(UIntPtr ret)
		ScPsExitProcess(regs->rbx);
		break;
	}
	case 0x13: {																								// IntPtr FsOpenFile(PWChar path)
		regs->rax = ScFsOpenFile((PWChar)regs->rbx);
		break;
	}
	case 0x14: {																								// UIntPtr FsReadFile(IntPtr handle, UIntPtr size, PUInt8 buf)
		regs->rax = ScFsReadFile(regs->rbx, regs->rcx, (PUInt8)regs->rdx);
		break;
	}
	case 0x15: {																								// UIntPtr FsWriteFile(IntPtr handle, UIntPtr size, PUInt8 buf)
		regs->rax = ScFsWriteFile(regs->rbx, regs->rcx, (PUInt8)regs->rdx);
		break;
	}
	case 0x16: {																								// Boolean FsMountFile(PWChar path, PWChar file, PWChar type)
		regs->rax = ScFsMountFile((PWChar)regs->rbx, (PWChar)regs->rcx, (PWChar)regs->rdx);
		break;
	}
	case 0x17: {																								// Boolean FsUmountFile(PWChar path)
		regs->rax = ScFsUmountFile((PWChar)regs->rbx);
		break;
	}
	case 0x18: {																								// Boolean FsReadDirectoryEntry(IntPtr handle, UIntPtr entry, PWChar out)
		regs->rax = ScFsReadDirectoryEntry(regs->rbx, regs->rcx, (PWChar)regs->rdx);
		break;
	}
	case 0x19: {																								// IntPtr FsFindInDirectory(IntPtr handle, PWChar name)
		regs->rax = ScFsFindInDirectory(regs->rbx, (PWChar)regs->rcx);
		break;
	}
	case 0x1A: {																								// Boolean FsCreateFile(IntPtr handle, PWChar name, UIntPtr type)
		regs->rax = ScFsCreateFile(regs->rbx, (PWChar)regs->rcx, regs->rdx);
		break;
	}
	case 0x1B: {																								// Boolean FsControlFile(IntPtr handle, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf)
		regs->rax = ScFsControlFile(regs->rbx, regs->rcx, (PUInt8)regs->rdx, (PUInt8)regs->rsi);
		break;
	}
	case 0x1C: {																								// UIntPtr FsGetSize(IntPtr handle)
		regs->rax = ScFsGetFileSize(regs->rbx);
		break;
	}
	case 0x1D: {																								// UIntPtr FsGetPosition(IntPtr handle)
		regs->rax = ScFsGetPosition(regs->rbx);
		break;
	}
	case 0x1E: {																								// Boolean FsSetPosition(IntPtr handle, IntPtr base, UIntPtr off)
		ScFsSetPosition(regs->rbx, regs->rcx, regs->rdx);
		break;
	}
	case 0x1F: {																								// Boolean IpcCreatePort(PWChar name)
		regs->rax = ScIpcCreatePort((PWChar)regs->rbx);
		break;
	}
	case 0x20: {																								// IntPtr IpcCreateResponsePort(Void)
		regs->rax = ScIpcCreateResponsePort();
		break;
	}
	case 0x21: {																								// Void IpcRemovePort(PWchar name)
		ScIpcRemovePort((PWChar)regs->rbx);
		break;
	}
	case 0x22: {																								// Boolean IpcCheckPort(PWChar name)
		regs->rax = ScIpcCheckPort((PWChar)regs->rbx);
		break;
	}
	case 0x23: {																								// Boolean IpcSendMessage(PWChar port, UInt32 msg, UIntPtr size, PUInt8 buf, IntPtr rport)
		regs->rax = ScIpcSendMessage((PWChar)regs->rbx, regs->rcx, regs->rdx, (PUInt8)regs->rsi, regs->rdi);
		break;
	}
	case 0x24: {																								// Boolean IpcSendResponse(IntPtr handle, UInt32 msg, UIntPtr size, PUInt8 buf)
		regs->rax = ScIpcSendResponse(regs->rbx, regs->rcx, regs->rdx, (PUInt8)regs->rsi);
		break;
	}
	case 0x25: {																								// Boolean IpcReceiveMessage(PWChar name, PUInt32 msg, UIntPtr size, PUInt8 buf)
		regs->rax = ScIpcReceiveMessage((PWChar)regs->rbx, (PUInt32)regs->rcx, regs->rdx, (PUInt8)regs->rsi);
		break;
	}
	case 0x26: {																								// PIpcMessage IpcReceiveResponse(IntPtr handle, PUInt32 msg, UIntPtr size, PUInt8 buf)
		regs->rax = ScIpcReceiveResponse(regs->rbx, (PUInt32)regs->rcx, regs->rdx, (PUInt8)regs->rsi);
		break;
	}
	case 0x27: {																								// UIntPtr ShmCreateSection(UIntPtr size, PUIntPtr key)
		regs->rax = ScShmCreateSection(regs->rbx, (PUIntPtr)regs->rcx);
		break;
	}
	case 0x28: {																								// UIntPtr ShmMapSection(UIntPtr key)
		regs->rax = ScShmMapSection(regs->rbx);
		break;
	}
	case 0x29: {																								// Void ShmUnmapSection(UIntPtr key)
		ScShmUnmapSection(regs->rbx);
		break;
	}
	default: {
		regs->rax = (UIntPtr)-1;
		break;
	}
	}
}

Void ArchInitSc(Void) {
	IDTRegisterInterruptHandler(0x3F, ArchScHandler);
}
