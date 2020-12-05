/* File author is Ítalo Lima Marconato Matias
 *
 * Created on November 30 of 2020, at 16:49 BRT
 * Last edited on December 02 of 2020, at 20:10 BRT */

#include <arch/arch.hxx>
#include <arch/desctables.hxx>
#include <arch/port.hxx>
#include <process.hxx>

Void *ArchImpl::CreateContext(UIntPtr Entry) {
	Context *ctx = new Context();
	
	if (ctx == Null) {
		return Null;
	}
	
	/* Setup the kernel stack: This is going to say the entry point, and the entry stack, for now, we
	 * only need to handle the kernel case (that is, when the process/thread is a kernel one). */
	
	UIntPtr *kstack = (UIntPtr*)&ctx->KernelStack[0x10000 - (sizeof(UIntPtr) * 2)];
	
	*kstack-- = 0x10;
	*kstack-- = (UIntPtr)&ctx->KernelStack[0x10000 - (sizeof(UIntPtr) * 2)];
	*kstack-- = 0x202;
	*kstack-- = 0x08;
	*kstack-- = Entry;
	
	for (Int i = 0; i < 6; i++) {
		*kstack-- = 0;
	}
	
#ifdef ARCH_64
	for (Int i = 0; i < 8; i++) {
		*kstack-- = 0;
	}
#else
	*kstack-- = (UIntPtr)&ctx->KernelStack[0x10000 - (sizeof(UIntPtr) * 2)];
#endif
	
	for (Int i = 0; i < 3; i++) {
		*kstack-- = 0;
	}
	
	for (Int i = 0; i < 2; i++) {
		*kstack-- = 0x10;
	}
	
	ctx->StackPointer = (UIntPtr)(kstack + 1);
	
	return ctx;
}

Void ArchImpl::FreeContext(Void *Data) {
	if (Data != Null) {
		delete (Context*)Data;
	}
}

Void ProcManager::Switch(Boolean Requeue) {
	/* Using a variable in the process/thread itself should be better/safer than a global one... */
	
	if (!ProcManager::GetStatus()) {
		return;
	} else if (!Requeue) {
		CurrentThread->Requeue = False;
	}
	
	asm volatile("sti; int $0x3E");
}

static Boolean CompareAndExchange(UIntPtr *Pointer, UIntPtr Expected, UIntPtr Value) {
	return __atomic_compare_exchange_n(Pointer, &Expected, Value, 0,
									   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static Void DoSwitch(UIntPtr Stack) {
#ifdef ARCH_64
	asm volatile("mov %%rax, %%rsp" :: "a"(Stack));
	asm volatile("pop %rax");
	asm volatile("mov %rax, %es");
	asm volatile("pop %rax");
	asm volatile("mov %rax, %ds");
	asm volatile("pop %r15");
	asm volatile("pop %r14");
	asm volatile("pop %r13");
	asm volatile("pop %r12");
	asm volatile("pop %r11");
	asm volatile("pop %r10");
	asm volatile("pop %r9");
	asm volatile("pop %r8");
	asm volatile("pop %rbp");
	asm volatile("pop %rdi");
	asm volatile("pop %rsi");
	asm volatile("pop %rdx");
	asm volatile("pop %rcx");
	asm volatile("pop %rbx");
	asm volatile("pop %rax");
	asm volatile("add $16, %rsp");
	asm volatile("iretq");
#else
	asm volatile("mov %%eax, %%esp" :: "a"(Stack));
	asm volatile("pop %es");
	asm volatile("pop %ds");
	asm volatile("popa");
	asm volatile("add $8, %esp");
	asm volatile("iret");
#endif
}

Void ProcManager::SwitchManual(Void *Data) {
	/* We can handle the WaitAddress "locks" even here (it's not something timer-specific/related), so,
	 * let's do that. */
	
w:	for (Thread *th : WaitA) {
		if (CompareAndExchange(th->WaitA.Address, th->WaitA.Expected, th->WaitA.Value)) {
			WakeupThread(WaitA, th);
			goto w;
		}
	}
	
	if (!Enabled) {
		return;
	}
	
	/* No need to return anything, just modifying the Registers struct (which is the Data variable)
	 * is already enough. */
	
	Registers *regs = (Registers*)Data;
	Thread *old = CurrentThread;
	
	/* If the old process isn't Null (that is, this is not the first process being executed), we need
	 * to update its fields, and requeue (well, if ->Requeue is set). */
	
	if (CurrentThread != Null) {
		CurrentThread->Time = 19;
		((Context*)CurrentThread->Context)->StackPointer = (UIntPtr)regs;
	}
	
	/* No need to go forward if the CurrentThread hasn't changed. Also, now it's just a matter of
	 * switching the page directory, the stack pointer, and popping everything that we need. */
	
	if ((CurrentThread = GetNext()) == old) {
		return;
	}
	
	if (old != Null) {
		if (old->Requeue) {
				Queue.Add(old);
			} else {
				old->Requeue = True;
			}
		
		if (GetCurrentProcess()->Directory != old->Parent->Directory) {
			Arch->SwitchDirectory(GetCurrentProcess()->Directory);
		}
	}
	
	DoSwitch(((Context*)CurrentThread->Context)->StackPointer);
}

Void ProcManager::SwitchTimer(Void *Data) {
	/* Simillar to the function above, but we have extra things to do (well, and also some things
	 * that we don't need to do): We don't need to switch if the ->Time field of the process/thread
	 * is not zero; We don't need to check if we need to requeue the old process, nor if it was
	 * Null. */
	
wa:	for (Thread *th : WaitA) {
		if (CompareAndExchange(th->WaitA.Address, th->WaitA.Expected, th->WaitA.Value)) {
			WakeupThread(WaitA, th);
			goto wa;
		}
	}
	
ws:	for (Thread *th : WaitS) {
		if (th->WaitS == 0) {
			WakeupThread(WaitS, th);
			goto ws;
		}
		
		th->WaitS--;
	}
	
	if (!Enabled || CurrentThread == Null) {
		return;
	} else if (CurrentThread->Time != 0) {
		CurrentThread->Time--;
		return;
	}
	
	Registers *regs = (Registers*)Data;
	Thread *old = CurrentThread;
	
	CurrentThread->Time = 19;
	((Context*)CurrentThread->Context)->StackPointer = (UIntPtr)regs;
	
	if ((CurrentThread = GetNext()) == old) {
		return;
	}
	
	Queue.Add(old);
	
	if (GetCurrentProcess()->Directory != old->Parent->Directory) {
		Arch->SwitchDirectory(GetCurrentProcess()->Directory);
	}
	
	/* Also, we need to outb(0x20, 0x20), as this is being run in the timer interrupt (not in the
	 * 0x3E syscall). */
	
	Port::OutByte(0x20, 0x20);
	DoSwitch(((Context*)CurrentThread->Context)->StackPointer);
}

static Void Handler(Registers *Regs) {
	ProcManager::SwitchManual(Regs);
}

Void InitProcessInterface(Void) {
	/* The int num that we pass to IdtSetHandler is actually the int num - 32 (so the actual switch
	 * syscall is 0x3E, not 0x1E). */
	
	IdtSetHandler(0x1E, Handler);
}
