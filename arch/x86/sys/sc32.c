// File author is Ítalo Lima Marconato Matias
//
// Created on November 16 of 2018, at 00:48 BRT
// Last edited on November 16 of 2019, at 11:25 BRT

#include <chicago/arch/idt.h>
#include <chicago/sc.h>

static Void ArchScHandler(PRegisters regs) {
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
	case 0x12: {																								// Void PsExitThread(UIntPtr ret)
		ScPsExitThread(regs->ebx);
		break;
	}
	case 0x13: {																								// Void PsExitProcess(UIntPtr ret)
		ScPsExitProcess(regs->ebx);
		break;
	}
	case 0x14: {																								// IntPtr FsOpenFile(PWChar path)
		regs->eax = ScFsOpenFile((PWChar)regs->ebx);
		break;
	}
	case 0x15: {																								// Void FsCloseFile(IntPtr file)
		ScFsCloseFile(regs->ebx);
		break;
	}
	case 0x16: {																								// Boolean FsReadFile(IntPtr file, UIntPtr size, PUInt8 buf)
		regs->eax = ScFsReadFile(regs->ebx, regs->ecx, (PUInt8)regs->edx);
		break;
	}
	case 0x17: {																								// Boolean FsWriteFile(IntPtr file, UIntPtr size, PUInt8 buf)
		regs->eax = ScFsWriteFile(regs->ebx, regs->ecx, (PUInt8)regs->edx);
		break;
	}
	case 0x18: {																								// Boolean FsMountFile(PWChar path, PWChar file, PWChar type)
		regs->eax = ScFsMountFile((PWChar)regs->ebx, (PWChar)regs->ecx, (PWChar)regs->edx);
		break;
	}
	case 0x19: {																								// Boolean FsUmountFile(PWChar path)
		regs->eax = ScFsUmountFile((PWChar)regs->ebx);
		break;
	}
	case 0x1A: {																								// Boolean FsReadDirectoryEntry(IntPtr dir, UIntPtr entry, PWChar out)
		regs->eax = ScFsReadDirectoryEntry(regs->ebx, regs->ecx, (PWChar)regs->edx);
		break;
	}
	case 0x1B: {																								// IntPtr FsFindInDirectory(IntPtr dir, PWChar name)
		regs->eax = ScFsFindInDirectory(regs->ebx, (PWChar)regs->ecx);
		break;
	}
	case 0x1C: {																								// Boolean FsCreateFile(IntPtr dir, PWChar name, UIntPtr type)
		regs->eax = ScFsCreateFile(regs->ebx, (PWChar)regs->ecx, regs->edx);
		break;
	}
	case 0x1D: {																								// Boolean FsControlFile(IntPtr file, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf)
		regs->eax = ScFsControlFile(regs->ebx, regs->ecx, (PUInt8)regs->edx, (PUInt8)regs->esi);
		break;
	}
	case 0x1E: {																								// UIntPtr FsGetSize(IntPtr file)
		regs->eax = ScFsGetFileSize(regs->ebx);
		break;
	}
	case 0x1F: {																								// UIntPtr FsGetPosition(IntPtr file)
		regs->eax = ScFsGetPosition(regs->ebx);
		break;
	}
	case 0x20: {																								// Boolean FsSetPosition(IntPtr file, IntPtr base, UIntPtr off)
		ScFsSetPosition(regs->ebx, regs->ecx, regs->edx);
		break;
	}
	case 0x21: {																								// UIntPtr ExecCreateProcess(PWChar path)
		regs->eax = ScExecCreateProcess((PWChar)regs->ebx);
		break;
	}
	case 0x22: {																								// PLibHandle ExecLoadLibrary(PWChar path, Boolean global)
		regs->eax = (UIntPtr)ScExecLoadLibrary((PWChar)regs->ebx, regs->ecx);
		break;
	}
	case 0x23: {																								// Void ScExecCloseLibrary(PLibHandle handle)
		ScExecCloseLibrary((PLibHandle)regs->ebx);
		break;
	}
	case 0x24: {																								// UIntPtr ExecGetSymbol(PLibHandle handle, PWChar name)
		regs->eax = ScExecGetSymbol((PLibHandle)regs->ebx, (PWChar)regs->ecx);
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
