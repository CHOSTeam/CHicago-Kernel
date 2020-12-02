/* File author is Ítalo Lima Marconato Matias
 *
 * Created on November 30 of 2020, at 13:20 BRT
 * Last edited on December 01 of 2020, at 16:42 BRT */

#include <arch.hxx>
#include <process.hxx>
#include <textout.hxx>

SpinLock ProcManager::Lock;
Boolean ProcManager::Enabled = False;
UIntPtr ProcManager::LastPID = 0;
List<Process*> ProcManager::Processes;
Thread *ProcManager::CurrentThread = Null;
List<Thread*> ProcManager::Queue, ProcManager::WaitA, ProcManager::WaitT, ProcManager::WaitP,
			  ProcManager::WaitS, ProcManager::KillList;

Thread::Thread(UIntPtr ID, Process *Parent, Void *Context) : Context(Context), Parent(Parent),
															 ID(ID) { }

Process::Process(const String &Name, UIntPtr ID, UIntPtr Directory) : Name(Name), ID(ID),
																	  Directory(Directory) { }

Thread::~Thread(Void) {
	/* Don't even try deleting anything if the thread is the current running one. */
	
	if (this == ProcManager::CurrentThread) {
		Debug->Write("PANIC! Tried deleting the currently running thread\n");
		Arch->Halt();
	}
	
	if (Context != Null) {
		Arch->FreeContext(Context);
	}
	
	UIntPtr i = 0;
	
	for (Thread *th : Parent->Threads) {
		if (th == this) {
			Parent->Threads.Remove(i);
			break;
		}
		
		i++;
	}
	
	/* We can directly access ProcManager::Queue (and we need to, so that we can remove this thread from
	 * it). */
	
	ProcManager::Lock.Lock();
	
	i = 0;
	
	for (Thread *th : ProcManager::Queue) {
		if (th == this) {
			ProcManager::Queue.Remove(i);
			break;
		}
		
		i++;
	}
	
	/* Ah, we also have to wakeup any thread on the WaitT list (and remove ourself from all the wait
	 * lists). */
	
wa: i = 0;
	
	for (Thread *th : ProcManager::WaitA) {
		if (th == this) {
			ProcManager::WaitA.Remove(i);
			goto wa;
		}
		
		i++;
	}
	
wt:	i = 0;
	
	for (Thread *th : ProcManager::WaitT) {
		if (th->WaitT == this) {
			th->Return = Return;
			ProcManager::WakeupThread(ProcManager::WaitT, th);
			goto wt;
		} else if (th == this) {
			ProcManager::WaitT.Remove(i);
			goto wt;
		}
		
		i++;
	}
	
wp:	i = 0;
	
	for (Thread *th : ProcManager::WaitP) {
		if (th == this) {
			ProcManager::WaitP.Remove(i);
			goto wp;
		}
		
		i++;
	}
	
ws:	i = 0;
	
	for (Thread *th : ProcManager::WaitS) {
		if (th == this) {
			ProcManager::WaitS.Remove(i);
			goto ws;
		}
		
		i++;
	}
	
	ProcManager::Lock.Unlock();
}

Process::~Process(Void) {
	/* Don't even try deleting anything if the process is the parent of the currently running thread. */
	
	if (ProcManager::CurrentThread != Null && this == ProcManager::GetCurrentProcess()) {
		Debug->Write("PANIC! Tried deleting the currently running process\n");
		Arch->Halt();
	}
	
	while (Threads.GetLength() != 0) {
		delete Threads[0];
	}
	
	/* We also need to remove ourselves from the process list. */
	
	ProcManager::Lock.Lock();
	
	UIntPtr i = 0;
	
	for (Process *proc : ProcManager::Processes) {
		if (proc == this) {
			ProcManager::Processes.Remove(i);
			break;
		}
		
		i++;
	}
	
	/* Ah, we also have to wakeup any thread on the WaitP list (the thread deleting above already removed
	 * us from it, and from the other lists). */
	
w:	for (Thread *th : ProcManager::WaitP) {
		if (th->WaitP == this) {
			th->Return = Return;
			ProcManager::WakeupThread(ProcManager::WaitP, th);
			goto w;
		}
	}
	
	ProcManager::Lock.Unlock();
	Arch->FreeDirectory(Directory);
}

Status Process::Get(UIntPtr ID, Thread *&Out) {
	/* Yes, we do have a ProcManager::GetThread function, but that function only works for the current
	 * process (and, of course, it calls this function), while this function works for any process. */
	
	Lock.Lock();
	
	for (Thread *th : ProcManager::Queue) {
		if (th->ID == ID) {
			Lock.Unlock();
			Out = th;
			return Status::Success;
		}
	}
	
	Lock.Unlock();
	
	return Status::NotFound;
}

Status ProcManager::CreateThread(UIntPtr Entry, Thread *&Out) {
	/* Just make sure that no one is calling us before creating the first process. */
	
	return CurrentThread != Null ? CreateThread(GetCurrentProcess(), Entry, Out)
								 : Status::InvalidProcess;
}

Status ProcManager::CreateThread(Process *Parent, UIntPtr Entry, Thread *&Out, Boolean ShouldLock) {
	/* Creating the thread: We need to create the Context (using the entry point), create the thread
	 * struct itself, add it to the process thread list, add it to the queue, and return. */
	
	Void *ctx = Arch->CreateContext(Entry);
	
	if (ctx == Null) {
		return Status::OutOfMemory;
	}
	
	if (ShouldLock) {
		Lock.Lock();
	}
	
	if ((Out = new Thread(Parent->LastTID, Parent, ctx)) == Null) {
		if (ShouldLock) {
			Lock.Unlock();
		}
		
		Arch->FreeContext(ctx);
		
		return Status::OutOfMemory;
	}
	
	Status status = Parent->Threads.Add(Out);
	
	if (status != Status::Success) {
		if (ShouldLock) {
			Lock.Unlock();
		}
		
		delete Out;
		return Status::OutOfMemory;
	} else if ((status = Queue.Add(Out)) != Status::Success) {
		Parent->Threads.Remove(Parent->Threads.GetLength() - 1);
		
		if (ShouldLock) {
			Lock.Unlock();
		}
		
		delete Out;
		return Status::OutOfMemory;
	}
	
	Parent->LastTID++;
	
	if (ShouldLock) {
		Lock.Unlock();
	}
	
	return Status::Success;
}

Status ProcManager::CreateProcess(const String &Name, UIntPtr Entry, Process *&Out) {
	UIntPtr dir;
	Status status = Arch->CreateDirectory(dir);
	
	if (status != Status::Success) {
		return status;
	} else if ((status = CreateProcess(Name, Entry, dir, Out)) != Status::Success) {
		Arch->FreeDirectory(dir);
	}
	
	return status;
}

Status ProcManager::CreateProcess(const String &Name, UIntPtr Entry, UIntPtr Directory, Process *&Out) {
	if (Name.GetLength() == 0) {
		return Status::InvalidArg;
	}
	
	/* Here we do both the process creating, and adding the process to the queue (that is, there is no
	 * way to create the process, but only add it later). */
	
	Lock.Lock();
	
	if ((Out = new Process(Name, LastPID, Directory)) == Null) {
		Lock.Unlock();
		return Status::OutOfMemory;
	}
	
	Thread *th;
	Status status = CreateThread(Out, Entry, th, False);
	
	if (status != Status::Success) {
		Lock.Unlock();
		delete Out;
		return status;
	} else if ((status = Processes.Add(Out)) != Status::Success) {
		Lock.Unlock();
		delete Out;
		return status;
	}
	
	LastPID++;
	Lock.Unlock();
	
	return status;
}

Status ProcManager::GetThread(UIntPtr ID, Thread *&Out) {
	return CurrentThread != Null ? GetCurrentProcess()->Get(ID, Out) : Status::NotFound;
}

Status ProcManager::GetProcess(UIntPtr ID, Process *&Out) {
	/* Same as ProcManager::GetThread/Process::Get, but using our process list. */
	
	Lock.Lock();
	
	for (Process *proc : Processes) {
		if (proc->ID == ID) {
			Lock.Unlock();
			Out = proc;
			return Status::Success;
		}
	}
	
	Lock.Unlock();
	
	return Status::NotFound;
}

Void ProcManager::Wait(UIntPtr MilliSeconds) {
	/* Sleep (process version): Set the WaitS field, add the thread to the WaitS list, and Switch
	 * (while remembering to pass False as the first arg, as we shouldn't be rescheduled). */
	
	if (MilliSeconds == 0) {
		return;
	} else if (CurrentThread == Null || Queue.GetLength() == 0) {
		Arch->Sleep(MilliSeconds);
		return;
	}
	
	Lock.Lock();
	CurrentThread->Lock.Lock();
	
	Status status = WaitS.Add(CurrentThread);
	
	if (status != Status::Success) {
		CurrentThread->Lock.Unlock();
		Lock.Unlock();
		Arch->Sleep(MilliSeconds);
		return;
	}
	
	CurrentThread->WaitS = MilliSeconds;
	
	CurrentThread->Lock.Unlock();
	Lock.Unlock();
	Switch(False);
}

IntPtr ProcManager::Wait(Thread *Wait) {
	/* Waiting for another thread is simillar to the Sleep function, but we should set the WaitT
	 * field, and remeber to set the Out field after the Switch function "returns". */
	
	if (Wait == Null || CurrentThread == Null || Wait == CurrentThread) {
		return -1;
	}
	
	Lock.Lock();
	CurrentThread->Lock.Lock();
	
	Status status = WaitT.Add(CurrentThread);
	
	if (status != Status::Success) {
		CurrentThread->Lock.Unlock();
		Lock.Unlock();
		return -1;
	}
	
	CurrentThread->WaitT = Wait;
	
	CurrentThread->Lock.Unlock();
	Lock.Unlock();
	Switch(False);
	
	return CurrentThread->Return;
}

IntPtr ProcManager::Wait(Process *Wait) {
	/* And waiting for another process is pretty much the same as waiting for another thread, but we
	 * set the WaitP variable. */
	
	if (Wait == Null || CurrentThread == Null || Wait == GetCurrentProcess()) {
		return -1;
	}
	
	Lock.Lock();
	CurrentThread->Lock.Lock();
	
	Status status = WaitP.Add(CurrentThread);
	
	if (status != Status::Success) {
		CurrentThread->Lock.Unlock();
		Lock.Unlock();
		return -1;
	}
	
	CurrentThread->WaitP = Wait;
	
	CurrentThread->Lock.Unlock();
	Lock.Unlock();
	Switch(False);
	
	return CurrentThread->Return;
}

static Boolean CompareAndExchange(UIntPtr *Pointer, UIntPtr Expected, UIntPtr Value) {
	return __atomic_compare_exchange_n(Pointer, &Expected, Value, 0,
									   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

Status ProcManager::Wait(UIntPtr *Address, UIntPtr Expected, UIntPtr Value) {
	/* Waiting for the value at an address to change: First, in case multitasking still hasn't been
	 * initialized (or this is the only thread running), we can just do the same thing that we do on
	 * the SpinLock functions. */
	
	if (Address == Null) {
		return Status::InvalidArg;
	} else if (CurrentThread == Null || Queue.GetLength() == 0) {
		while (!CompareAndExchange(Address, Expected, Value)) {
#if defined(__i386__) || defined(__amd64__)
			asm volatile("pause");
#endif
		}
		
		return Status::Success;
	} else if (CompareAndExchange(Address, Expected, Value)) {
		return Status::Success;
	}
	
	/* In case we actually need to add this to the wait list (and unqueue), well, it's the same as
	 * the other wait functions (but this time we need to fill the WaitAddress struct). */
	
	Lock.Lock();
	CurrentThread->Lock.Lock();
	
	Status status = WaitA.Add(CurrentThread);
	
	if (status != Status::Success) {
		CurrentThread->Lock.Unlock();
		Lock.Unlock();
		return status;
	}
	
	CurrentThread->WaitA.Address = Address;
	CurrentThread->WaitA.Expected = Expected;
	CurrentThread->WaitA.Value = Value;
	
	CurrentThread->Lock.Unlock();
	Lock.Unlock();
	Switch(False);
	
	return Status::Success;
}

Void ProcManager::ExitThread(IntPtr Return) {
	/* We can't exit if there is no thread running (that is, we're not in a multitasking environment),
	 * or if this is the PID 0 TID 0 thread. */
	
	if (CurrentThread == Null || (CurrentThread->ID == 0 && GetCurrentProcess()->ID == 0)) {
		Debug->Write("PANIC! Tried Exit()ing the main kernel thread\n");
		Arch->Halt();
	}
	
	Lock.Lock();
	CurrentThread->Lock.Lock();
	GetCurrentProcess()->Lock.Lock();
	
	/* Now, we can add the process to the kill list, and remove it from the queue (and the proc's
	 * thread list). */
	
	Status status = KillList.Add(CurrentThread);
	
	if (status != Status::Success) {
		Debug->Write("PANIC! The system is out of memory to even add threads to the kill list\n");
		Arch->Halt();
	}
	
w:	for (Thread *th : WaitT) {
		if (th->WaitT == CurrentThread) {
			th->Return = CurrentThread->Return;
			WakeupThread(WaitT, th);
			goto w;
		}
	}
	
	UIntPtr i = 0;
	
	for (Thread *th : GetCurrentProcess()->Threads) {
		if (th == CurrentThread) {
			GetCurrentProcess()->Threads.Remove(i);
			break;
		}
		
		i++;
	}
	
	i = 0;
	
	for (Thread *th : Queue) {
		if (th == CurrentThread) {
			Queue.Remove(i);
			break;
		}
		
		i++;
	}
	
	CurrentThread->Return = Return;
	
	GetCurrentProcess()->Lock.Unlock();
	CurrentThread->Lock.Unlock();
	Lock.Unlock();
	Switch(False);
	Arch->Halt();
}

Void ProcManager::ExitProcess(IntPtr Return) {
	/* To Exit() an entire process, we can just delete all the threads (except for the running one), and
	 * call ExitThread (well, actually, and do its job). */
	
	if (CurrentThread == Null || GetCurrentProcess()->ID == 0) {
		Debug->Write("PANIC! Tried Exit()ing the main kernel process\n");
		Arch->Halt();
	} else if (GetCurrentProcess()->Threads.GetLength() == 1) {
		GetCurrentProcess()->Return = Return;
		ExitThread(Return);
		Arch->Halt();
	}
	
	Lock.Lock();
	CurrentThread->Lock.Lock();
	GetCurrentProcess()->Lock.Lock();
	
	Status status = KillList.Add(CurrentThread);
	
	if (status != Status::Success) {
		Debug->Write("PANIC! The system is out of memory to even add threads to the kill list\n");
		Arch->Halt();
	}
	
w:	for (Thread *th : WaitP) {
		if (th->WaitP == GetCurrentProcess()) {
			th->Return = GetCurrentProcess()->Return;
			WakeupThread(WaitP, th);
			goto w;
		}
	}
	
	while (GetCurrentProcess()->Threads.GetLength() != 0) {
		if (GetCurrentProcess()->Threads[0] != CurrentThread) {
			delete GetCurrentProcess()->Threads[0];
		}
		
		GetCurrentProcess()->Threads.Remove(0);
	}
	
	UIntPtr i = 0;
	
	for (Thread *th : Queue) {
		if (th == CurrentThread) {
			Queue.Remove(i);
			break;
		}
		
		i++;
	}
	
	CurrentThread->Return = Return;
	GetCurrentProcess()->Return = Return;
	
	GetCurrentProcess()->Lock.Unlock();
	CurrentThread->Lock.Unlock();
	Lock.Unlock();
	Switch(False);
	Arch->Halt();
}

Thread *ProcManager::GetNext(Void) {
	if (Queue.GetLength() != 0) {
		Thread *ret = Queue[0];
		Queue.Remove(0);
		return ret;
	}
	
	return CurrentThread;
}

Void ProcManager::WakeupThread(List<Thread*> &WaitList, Thread *WakeThread) {
	/* We could probably remove this from here, but, this function just removes the thread from the wait
	 * list and readd it to the queue. */
	
	UIntPtr i = 0;
	
	for (Thread *th : WaitList) {
		if (th == WakeThread) {
			WaitList.Remove(i);
			Queue.Add(th);
			return;
		}
		
		i++;
	}
}

Void KillerThread(Void) {
	/* The killer thread will always be running (it clean up other dead threads/processes), so everything
	 * will be inside a while(1) loop. */
	
	while (True) {
		while (ProcManager::KillList.GetLength() == 0) {
			ProcManager::Switch();
		}
		
		/* Clean up for single threads: delete the thread (yup, just that).
		 * And clean up for threads where the parent's thread list is empty: delete the process (no need to
		 * delete the process). */
		
		while (ProcManager::KillList.GetLength() != 0) {
			Thread *th = ProcManager::KillList[0];
			
			ProcManager::KillList.Remove(0);
			
			if (th->Parent->Threads.GetLength() == 0) {
				delete th->Parent;
			} else {
				delete th;
			}
		}
	}
}

Void ProcManager::Initialize(UIntPtr Entry) {
	/* The initial kernel process also acts as an idle process (when there are no other processes running),
	 * and, of course, we need to use Switch() instead of just waiting for the timer, as the timer only
	 * starts scheduling after we initialize the first process. */
	
	Process *proc;
	Status status = ProcManager::CreateProcess("System Process", Entry, Arch->GetCurrentDirectory(), proc);
	
	if (status != Status::Success) {
		Debug->Write("PANIC! Couldn't init the process manager\n");
		Arch->Halt();
	}
	
	/* We also need to already create the killer thread (which cleans up dead threads/processes). */
	
	Thread *th;
	
	if ((status = ProcManager::CreateThread(proc, (UIntPtr)KillerThread, th)) != Status::Success) {
		Debug->Write("PANIC! Couldn't init the process manager\n");
		Arch->Halt();
	}
	
	ProcManager::SetStatus(True);
	ProcManager::Switch();
}
