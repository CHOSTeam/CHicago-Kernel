// File author is Ítalo Lima Marconato Matias
//
// Created on October 29 of 2018, at 18:10 BRT
// Last edited on November 16 of 2019, at 11:25 BRT

#include <chicago/arch/idt.h>
#include <chicago/sc.h>

static Void ArchScHandler(PRegisters regs) {
	switch (regs->rax) {
	case 0x00: {																								// Void SysGetVersion(PSystemRegisters regs)
		ScSysGetVersion((PSystemVersion)regs->rbx);
		break;
	}
	case 0x01: {																								// UIntPtr MmAllocMemory(UIntPtr size)
		regs->rax = ScMmAllocMemory(regs->rbx);
		break;
	}
	case 0x02: {																								// Void MmFreeMemory(UIntPtr block)
		ScMmFreeMemory(regs->rbx);
		break;
	}
	case 0x03: {																								// UIntPtr MmReallocMemory(UIntPtr block, UIntPtr size)
		regs->rax = ScMmReallocMemory(regs->rbx, regs->rcx);
		break;
	}
	case 0x04: {																								// UIntPtr MmGetUsage(Void)
		regs->rax = ScMmGetUsage();
		break;
	}
	case 0x05: {																								// UIntPtr VirtAllocAddress(UIntPtr addr, UIntPtr size, UInt32 flags)
		regs->rax = ScVirtAllocAddress(regs->rbx, regs->rcx, regs->rdx);
		break;
	}
	case 0x06: {																								// Boolean VirtFreeAddress(UIntPtr addr, UIntPtr size)
		regs->rax = ScVirtFreeAddress(regs->rbx, regs->rcx);
		break;
	}
	case 0x07: {																								// UInt32 VirtQueryProtection(UIntPtr addr)
		regs->rax = ScVirtQueryProtection(regs->rbx);
		break;
	}
	case 0x08: {																								// Boolean VirtChangeProtection(UIntPtr addr, UIntPtr size, UInt32 flags)
		regs->rax = ScVirtChangeProtection(regs->rbx, regs->rcx, regs->rdx);
		break;
	}
	case 0x09: {																								// UIntPtr VirtGetUsage(Void)
		regs->rax = ScVirtGetUsage();
		break;
	}
	case 0x0A: {																								// UIntPtr PsCreateThread(UIntPtr entry)
		regs->rax = ScPsCreateThread(regs->rbx);
		break;
	}
	case 0x0B: {																								// UIntPtr PsGetTID(Void)
		regs->rax = ScPsGetTID();
		break;
	}
	case 0x0C: {																								// UIntPtr PsGetPID(Void)
		regs->rax = ScPsGetPID();
		break;
	}
	case 0x0D: {																								// Void PsSleep(UIntPtr ms)
		ScPsSleep(regs->rbx);
		break;
	}
	case 0x0E: {																								// UIntPtr PsWaitThread(UIntPtr id)
		regs->rax = ScPsWaitThread(regs->rbx);
		break;
	}
	case 0x0F: {																								// UIntPtr PsWaitProcess(UIntPtr id)
		regs->rax = ScPsWaitProcess(regs->rbx);
		break;
	}
	case 0x10: {																								// Void PsLock(PLock lock)
		ScPsLock((PLock)regs->rbx);
		break;
	}
	case 0x11: {																								// Void PsUnlock(PLock lock)
		ScPsUnlock((PLock)regs->rbx);
		break;
	}
	case 0x12: {																								// Void PsExitThread(UIntPtr ret)
		ScPsExitThread(regs->rbx);
		break;
	}
	case 0x13: {																								// Void PsExitProcess(UIntPtr ret)
		ScPsExitProcess(regs->rbx);
		break;
	}
	case 0x14: {																								// IntPtr FsOpenFile(PWChar path)
		regs->rax = ScFsOpenFile((PWChar)regs->rbx);
		break;
	}
	case 0x15: {																								// Void FsCloseFile(IntPtr file)
		ScFsCloseFile(regs->rbx);
		break;
	}
	case 0x16: {																								// Boolean FsReadFile(IntPtr file, UIntPtr size, PUInt8 buf)
		regs->rax = ScFsReadFile(regs->rbx, regs->rcx, (PUInt8)regs->rdx);
		break;
	}
	case 0x17: {																								// Boolean FsWriteFile(IntPtr file, UIntPtr size, PUInt8 buf)
		regs->rax = ScFsWriteFile(regs->rbx, regs->rcx, (PUInt8)regs->rdx);
		break;
	}
	case 0x18: {																								// Boolean FsMountFile(PWChar path, PWChar file, PWChar type)
		regs->rax = ScFsMountFile((PWChar)regs->rbx, (PWChar)regs->rcx, (PWChar)regs->rdx);
		break;
	}
	case 0x19: {																								// Boolean FsUmountFile(PWChar path)
		regs->rax = ScFsUmountFile((PWChar)regs->rbx);
		break;
	}
	case 0x1A: {																								// Boolean FsReadDirectoryEntry(IntPtr dir, UIntPtr entry, PWChar out)
		regs->rax = ScFsReadDirectoryEntry(regs->rbx, regs->rcx, (PWChar)regs->rdx);
		break;
	}
	case 0x1B: {																								// IntPtr FsFindInDirectory(IntPtr dir, PWChar name)
		regs->rax = ScFsFindInDirectory(regs->rbx, (PWChar)regs->rcx);
		break;
	}
	case 0x1C: {																								// Boolean FsCreateFile(IntPtr dir, PWChar name, UIntPtr type)
		regs->rax = ScFsCreateFile(regs->rbx, (PWChar)regs->rcx, regs->rdx);
		break;
	}
	case 0x1D: {																								// Boolean FsControlFile(IntPtr file, UIntPtr cmd, PUInt8 ibuf, PUInt8 obuf)
		regs->rax = ScFsControlFile(regs->rbx, regs->rcx, (PUInt8)regs->rdx, (PUInt8)regs->rsi);
		break;
	}
	case 0x1E: {																								// UIntPtr FsGetSize(IntPtr file)
		regs->rax = ScFsGetFileSize(regs->rbx);
		break;
	}
	case 0x1F: {																								// UIntPtr FsGetPosition(IntPtr file)
		regs->rax = ScFsGetPosition(regs->rbx);
		break;
	}
	case 0x20: {																								// Boolean FsSetPosition(IntPtr file, IntPtr base, UIntPtr off)
		ScFsSetPosition(regs->rbx, regs->rcx, regs->rdx);
		break;
	}
	case 0x21: {																								// UIntPtr ExecCreateProcess(PWChar path)
		regs->rax = ScExecCreateProcess((PWChar)regs->rbx);
		break;
	}
	case 0x22: {																								// PLibHandle ExecLoadLibrary(PWChar path, Boolean global)
		regs->rax = (UIntPtr)ScExecLoadLibrary((PWChar)regs->rbx, regs->rcx);
		break;
	}
	case 0x23: {																								// Void ScExecCloseLibrary(PLibHandle handle)
		ScExecCloseLibrary((PLibHandle)regs->rbx);
		break;
	}
	case 0x24: {																								// UIntPtr ExecGetSymbol(PLibHandle handle, PWChar name)
		regs->rax = ScExecGetSymbol((PLibHandle)regs->rbx, (PWChar)regs->rcx);
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
