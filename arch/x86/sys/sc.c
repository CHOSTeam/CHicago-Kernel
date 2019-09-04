// File author is Ítalo Lima Marconato Matias
//
// Created on November 16 of 2018, at 00:48 BRT
// Last edited on February 23 of 2019, at 17:20 BRT

#include <chicago/arch/idt.h>
#include <chicago/sc.h>

Void ArchScHandler(PRegisters regs) {
	switch (regs->eax) {
	case 0x00: {																								// Void SysGetVersion(PSystemRegisters regs)
		ScSysGetVersion((PSystemVersion)regs->ebx);
		break;
	}
	case 0x01: {																								// UIntPtr MmAllocMemory(UIntPtr size)
		regs->eax = ScMmAllocMemory(regs->ebx);
		break;
	}
	case 0x02: {																								// Void MmFreeMemory(UIntPtr block)
		ScMmFreeMemory(regs->ebx);
		break;
	}
	case 0x03: {																								// UIntPtr MmReallocMemory(UIntPtr block, UIntPtr size)
		regs->eax = ScMmReallocMemory(regs->ebx, regs->ecx);
		break;
	}
	case 0x04: {																								// UIntPtr MmGetUsage(Void)
		regs->eax = ScMmGetUsage();
		break;
	}
	case 0x05: {																								// UIntPtr VirtAllocAddress(UIntPtr addr, UIntPtr size, UInt32 flags)
		regs->eax = ScVirtAllocAddress(regs->ebx, regs->ecx, regs->edx);
		break;
	}
	case 0x06: {																								// Boolean VirtFreeAddress(UIntPtr addr, UIntPtr size)
		regs->eax = ScVirtFreeAddress(regs->ebx, regs->ecx);
		break;
	}
	case 0x07: {																								// UInt32 VirtQueryProtection(UIntPtr addr)
		regs->eax = ScVirtQueryProtection(regs->ebx);
		break;
	}
	case 0x08: {																								// Boolean VirtChangeProtection(UIntPtr addr, UIntPtr size, UInt32 flags)
		regs->eax = ScVirtChangeProtection(regs->ebx, regs->ecx, regs->edx);
		break;
	}
	case 0x09: {																								// UIntPtr VirtGetUsage(Void)
		regs->eax = ScVirtGetUsage();
		break;
	}
	case 0x0A: {																								// UIntPtr PsCreateThread(UIntPtr entry)
		regs->eax = ScPsCreateThread(regs->ebx);
		break;
	}
	case 0x0B: {																								// UIntPtr PsGetTID(Void)
		regs->eax = ScPsGetTID();
		break;
	}
	case 0x0C: {																								// UIntPtr PsGetPID(Void)
		regs->eax = ScPsGetPID();
		break;
	}
	case 0x0D: {																								// Void PsSleep(UIntPtr ms)
		ScPsSleep(regs->ebx);
		break;
	}
	case 0x0E: {																								// UIntPtr PsWaitThread(UIntPtr id)
		regs->eax = ScPsWaitThread(regs->ebx);
		break;
	}
	case 0x0F: {																								// UIntPtr PsWaitProcess(UIntPtr id)
		regs->eax = ScPsWaitProcess(regs->ebx);
		break;
	}
	case 0x10: {																								// Void PsLock(PLock lock)
		ScPsLock((PLock)regs->ebx);
		break;
	}
	case 0x11: {																								// Void PsUnlock(PLock lock)
		ScPsUnlock((PLock)regs->ebx);
		break;
	}
	case 0x12: {																								// PProcessPty PsGetPty(Void)
		regs->eax = (UIntPtr)ScPsGetPty();
		break;
	}
	case 0x13: {																								// Void PsAllocPty(Void (*kbdclear)(PProcessPty), Boolean (*kbdback)(PProcessPty), Boolean (*kbdwrite)(PProcessPty, WChar), Boolean (*read)(PProcessPty, UIntPtr, PWChar), Boolean (*write)(PProcessPty, UIntPtr, PWChar))
		ScPsAllocPty((PVoid)regs->ebx, (PVoid)regs->ecx, (PVoid)regs->edx, (PVoid)regs->esi, (PVoid)regs->edi);
		break;
	}
	case 0x14: {																								// Void PsFreePty(Void)
		ScPsFreePty();
		break;
	}
	case 0x15: {																								// Void PsExitThread(UIntPtr ret)
		ScPsExitThread(regs->ebx);
		break;
	}
	case 0x16: {																								// Void PsExitProcess(UIntPtr ret)
		ScPsExitProcess(regs->ebx);
		break;
	}
	case 0x17: {																								// Void PsForceSwitch(Void)
		ScPsForceSwitch();
		break;
	}
	case 0x18: {																								// IntPtr FsOpenFile(PWChar path)
		regs->eax = ScFsOpenFile((PWChar)regs->ebx);
		break;
	}
	case 0x19: {																								// Void FsCloseFile(IntPtr file)
		ScFsCloseFile(regs->ebx);
		break;
	}
	case 0x1A: {																								// Boolean FsReadFile(IntPtr file, UIntPtr size, PUInt8 buf)
		regs->eax = ScFsReadFile(regs->ebx, regs->ecx, (PUInt8)regs->edx);
		break;
	}
	case 0x1B: {																								// Boolean FsWriteFile(IntPtr file, UIntPtr size, PUInt8 buf)
		regs->eax = ScFsWriteFile(regs->ebx, regs->ecx, (PUInt8)regs->edx);
		break;
	}
	case 0x1C: {																								// Boolean FsMountFile(PWChar path, PWChar file, PWChar type)
		regs->eax = ScFsMountFile((PWChar)regs->ebx, (PWChar)regs->ecx, (PWChar)regs->edx);
		break;
	}
	case 0x1D: {																								// Boolean FsUmountFile(PWChar path)
		regs->eax = ScFsUmountFile((PWChar)regs->ebx);
		break;
	}
	case 0x1E: {																								// Boolean FsReadDirectoryEntry(IntPtr dir, UIntPtr entry, PWChar out)
		regs->eax = ScFsReadDirectoryEntry(regs->ebx, regs->ecx, (PWChar)regs->edx);
		break;
	}
	case 0x1F: {																								// IntPtr FsFindInDirectory(IntPtr dir, PWChar name)
		regs->eax = ScFsFindInDirectory(regs->ebx, (PWChar)regs->ecx);
		break;
	}
	case 0x20: {																								// Boolean FsCreateFile(IntPtr dir, PWChar name, UIntPtr type)
		regs->eax = ScFsCreateFile(regs->ebx, (PWChar)regs->ecx, regs->edx);
		break;
	}
	case 0x21: {																								// Boolean FsControlFile(IntPtr file, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf)
		regs->eax = ScFsControlFile(regs->ebx, regs->ecx, (PUInt8)regs->edx, (PUInt8)regs->esi);
		break;
	}
	case 0x22: {																								// UIntPtr FsGetSize(IntPtr file)
		regs->eax = ScFsGetFileSize(regs->ebx);
		break;
	}
	case 0x23: {																								// UIntPtr FsGetPosition(IntPtr file)
		regs->eax = ScFsGetPosition(regs->ebx);
		break;
	}
	case 0x24: {																								// Boolean FsSetPosition(IntPtr file, IntPtr base, UIntPtr off)
		ScFsSetPosition(regs->ebx, regs->ecx, regs->edx);
		break;
	}
	case 0x25: {																								// Boolean IpcCreatePort(PWChar name)
		regs->eax = ScIpcCreatePort((PWChar)regs->ebx);
		break;
	}
	case 0x26: {																								// Void IpcRemovePort(PWChar name)
		ScIpcRemovePort((PWChar)regs->ebx);
		break;
	}
	case 0x27: {																								// Void IpcSendMessage(PWChar port, UInt32 msg, UIntPtr size, PUInt8 buf)
		ScIpcSendMessage((PWChar)regs->ebx, regs->ecx, regs->edx, (PUInt8)regs->esi);
		break;
	}
	case 0x28: {																								// PIpcMessage IpcReceiveMessage(PWChar name)
		regs->eax = (UIntPtr)ScIpcReceiveMessage((PWChar)regs->ebx);
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
