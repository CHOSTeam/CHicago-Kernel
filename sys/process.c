// File author is Ítalo Lima Marconato Matias
//
// Created on July 27 of 2018, at 14:59 BRT
// Last edited on December 25 of 2019, at 17:28 BRT

#define __CHICAGO_PROCESS__

#include <chicago/arch/process.h>

#include <chicago/alloc.h>
#include <chicago/arch.h>
#include <chicago/debug.h>
#include <chicago/panic.h>
#include <chicago/process.h>
#include <chicago/sc.h>
#include <chicago/shm.h>
#include <chicago/string.h>
#include <chicago/timer.h>
#include <chicago/list.h>

extern Void KernelMainLate(Void);

Boolean PsTaskSwitchEnabled = False;
PThread PsCurrentThread = Null;
PQueue PsThreadQueue = Null;
PList PsProcessList = Null;
PList PsSleepList = Null;
PList PsWaittList = Null;
PList PsWaitpList = Null;
PList PsWaitlList = Null;
PList PsKillList = Null;
UIntPtr PsNextID = 0;

PThread PsCreateThreadInt(UIntPtr entry, UIntPtr userstack, Boolean user) {
	if (user && (userstack == 0)) {																												// Valid user stack?
		return Null;																															// Nope
	}
	
	PThread th = (PThread)MemAllocate(sizeof(Thread));																							// Let's try to allocate the process struct!
	
	if (th == Null) {
		return Null;																															// Failed
	} else if ((th->ctx = PsCreateContext(entry, userstack, user)) == Null) {																	// Create the thread context
		MemFree((UIntPtr)th);																													// Failed
		return Null;
	}
	
	th->id = 0;																																	// We're going to set the id and the parent process later
	th->retv = 0;
	th->time = PS_DEFAULT_QUANTUM - 1;																											// Set the default quantum
	th->wtime = 0;																																// We're not waiting anything (for now)
	th->parent = Null;																															// No parent (for now)
	th->waitl = Null;																															// We're not waiting anything (for now)
	th->waitp = Null;
	th->waitt = Null;
	th->killp = False;
	
	return th;
}

PProcess PsCreateProcessInt(PWChar name, UIntPtr entry, UIntPtr dir) {
	PProcess proc = (PProcess)MemAllocate(sizeof(Process));																						// Let's try to allocate the process struct!
	
	if (proc == Null) {
		return Null;																															// Failed
	}
	
	proc->id = PsNextID++;																														// Use the next PID
	proc->name = StrDuplicate(name);																											// Duplicate the name
	
	if ((proc->dir = dir) == 0) {																												// Create an new page dir?
		proc->dir = MmCreateDirectory();																										// Yes
		
		if (proc->dir == 0) {
			MemFree((UIntPtr)proc->name);																										// Failed...
			MemFree((UIntPtr)proc);
			
			return Null;
		}
	}
	
	if ((proc->threads = ListNew(False, False)) == Null) {																						// Create the thread list
		MmFreeDirectory(proc->dir);																												// Failed...
		MemFree((UIntPtr)proc->name);
		MemFree((UIntPtr)proc);
		return Null;
	}
	
	proc->last_tid = 0;
	proc->alloc_base = Null;
	proc->mem_usage = 0;
	proc->exec_handles = Null;
	proc->global_exec_handles = Null;
	proc->shm_mapped_sections = ListNew(False, False);																							// Init the mapped shm sections list for the process
	
	if (proc->shm_mapped_sections == Null) {
		ListFree(proc->threads);																												// Failed...
		MmFreeDirectory(proc->dir);
		MemFree((UIntPtr)proc->name);
		MemFree((UIntPtr)proc);
		return Null;
	}
	
	proc->handles = ListNew(False, False);																										// Init the handle list for the process
	proc->last_handle_id = 0;
	proc->exec_path = Null;
	
	if (proc->handles == Null) {
		ListFree(proc->shm_mapped_sections);																									// Failed...
		ListFree(proc->threads);
		MmFreeDirectory(proc->dir);
		MemFree((UIntPtr)proc->name);
		MemFree((UIntPtr)proc);
		return Null;
	}
	
	PThread th = PsCreateThreadInt(entry, 0, False);																							// Create the first thread
	
	if (th == Null) {
		ListFree(proc->handles);																												// Failed...
		ListFree(proc->shm_mapped_sections);
		ListFree(proc->threads);
		MmFreeDirectory(proc->dir);
		MemFree((UIntPtr)proc->name);
		MemFree((UIntPtr)proc);
		return Null;
	}
	
	th->id = proc->last_tid++;																													// Set the thread id
	th->parent = proc;																															// And the parent process
	
	if (!ListAdd(proc->threads, th)) {																											// Add it!
		PsFreeContext(th->ctx);																													// Failed...
		MemFree((UIntPtr)th);
		ListFree(proc->handles);
		ListFree(proc->shm_mapped_sections);
		ListFree(proc->threads);
		MmFreeDirectory(proc->dir);
		MemFree((UIntPtr)proc->name);
		MemFree((UIntPtr)proc);
		return Null;
	}
	
	return proc;
}

PThread PsCreateThread(UIntPtr entry, UIntPtr userstack, Boolean user) {
	return PsCreateThreadInt(entry, userstack, user);																							// Use our PsCreateThreadInt function
}

PProcess PsCreateProcess(PWChar name, UIntPtr entry) {
	return PsCreateProcessInt(name, entry, 0);																									// Use our PsCreateProcessInt function
}

Void PsAddThread(PThread th) {
	if ((th == Null) || (PsCurrentThread == Null) || (PsCurrentProcess == Null) || (PsThreadQueue == Null) || (PsProcessList == Null)) {		// Sanity checks
		return;
	}
	
	PsLockTaskSwitch(old);																														// Lock
	
	th->id = PsCurrentProcess->last_tid++;																										// Set the ID
	th->parent = PsCurrentProcess;																												// Set the parent process
	
	QueueAdd(PsThreadQueue, th);																												// Try to add this thread to the queue
	ListAdd(PsCurrentProcess->threads, th);																										// Try to add this thread to the thread list of this process
	PsUnlockTaskSwitch(old);																													// Unlock
}

Void PsAddProcess(PProcess proc) {
	if ((proc == Null) || (proc->dir == 0) || (proc->threads == Null) || (PsThreadQueue == Null) || (PsProcessList == Null)) {					// Sanity checks
		return;
	}
	
	PsLockTaskSwitch(old);																														// Lock
	
	ListForeach(proc->threads, i) {																												// Add all the threads from this process to the thread queue
		QueueAdd(PsThreadQueue, (PThread)i->data);
	}
	
	ListAdd(PsProcessList, proc);																												// Try to add this process to the process list
	PsUnlockTaskSwitch(old);																													// Unlock
}

PThread PsGetThread(UIntPtr id) {
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null) || (PsProcessList == Null)) {													// Sanity checks
		return Null;
	}
	
	ListForeach(PsCurrentProcess->threads, i) {																									// Let's search!
		PThread th = (PThread)i->data;
		
		if (th->id == id) {																														// Match?
			return th;																															// Yes :)
		}
	}
	
	return Null;
}

PProcess PsGetProcess(UIntPtr id) {
	if (PsProcessList == Null) {																												// Sanity checks
		return Null;
	}
	
	ListForeach(PsProcessList, i) {																												// Let's search!
		PProcess proc = (PProcess)i->data;
		
		if (proc->id == id) {																													// Match?
			return proc;																														// Yes :)
		}
	}
	
	return Null;
}

Void PsSleep(UIntPtr ms) {
	if (ms == 0) {																																// ms = 0?
		return;																																	// Yes, we don't need to do anything
	} else if ((PsSleepList == Null) || (PsThreadQueue == Null) || (PsThreadQueue->length == 0) || (PsCurrentThread == Null)) {					// Sleep list is initialized?
		TimerSleep(ms);																															// Nope
		return;
	}
	
	PsLockTaskSwitch(old);																														// Lock
	
	PsCurrentThread->wtime = ms;
	
	if (!ListAdd(PsSleepList, PsCurrentThread)) {																								// Try to add it to the sleep list
		PsUnlockTaskSwitch(old);																												// Failed, but let's keep on trying!
		PsSleep(ms);
		return;
	}
	
	PsUnlockTaskSwitch(old);																													// Unlock
	PsSwitchTask(PsDontRequeue);																												// Remove it from the queue and go to the next thread
}

UIntPtr PsWaitThread(UIntPtr id) {
	if ((PsWaittList == Null) || (PsThreadQueue == Null) || (PsCurrentThread == Null)) {														// Waitt list is initialized?
		return 1;																																// Nope
	}
	
	while (PsThreadQueue->length == 0) {																										// Let's wait for having other threads in the queue
		PsSwitchTask(Null);
	}
	
	PsLockTaskSwitch(old);																														// Lock
	
	if (!ListAdd(PsWaittList, PsCurrentThread)) {																								// Try to add it to the waitt list
		PsUnlockTaskSwitch(old);																												// Failed, but let's keep on trying!
		return PsWaitThread(id);
	}
	
	PsCurrentThread->waitt = PsGetThread(id);
	
	PsUnlockTaskSwitch(old);																													// Unlock
	PsSwitchTask(PsDontRequeue);																												// Remove it from the queue and go to the next thread
	
	return PsCurrentThread->retv;
}

UIntPtr PsWaitProcess(UIntPtr id) {
	if ((PsWaitpList == Null) || (PsThreadQueue == Null) || (PsCurrentThread == Null)) {														// Waitp list is initialized?
		return 1;																																// Nope
	}
	
	while (PsThreadQueue->length == 0) {																										// Let's wait for having other threads in the queue
		PsSwitchTask(Null);
	}
	
	PsLockTaskSwitch(old);																														// Lock
	
	if (!ListAdd(PsWaitpList, PsCurrentThread)) {																								// Try to add it to the waitp list
		PsUnlockTaskSwitch(old);																												// Failed, but let's keep on trying!
		return PsWaitThread(id);
	}
	
	PsCurrentThread->waitp = PsGetProcess(id);
	
	PsUnlockTaskSwitch(old);																													// Unlock
	PsSwitchTask(PsDontRequeue);																												// Remove it from the queue and go to the next thread
	
	return PsCurrentThread->retv;
}

Void PsLock(PLock lock) {
	if ((lock == Null) || (PsWaitlList == Null) || (PsCurrentThread == Null)) {																	// Sanity checks
		return;
	}
	
	while (lock->locked && (PsThreadQueue->length == 0)) {																						// Let's wait for having other threads in the queue
		PsSwitchTask(Null);
	}
	
	PsLockTaskSwitch(old);																														// Lock (the task switching)
	
	if ((PsThreadQueue->length == 0) || (!lock->locked)) {																						// We need to add it to the waitl list?
		lock->locked = True;																													// Nope, set it as locked and return!
		lock->owner = PsCurrentThread;
		PsUnlockTaskSwitch(old);
		return;
	}
	
	if (!ListAdd(PsWaitlList, PsCurrentThread)) {																								// Try to add it to the waitl list
		PsUnlockTaskSwitch(old);																												// Failed, but let's keep on trying!
		PsLock(lock);
		return;
	}
	
	PsCurrentThread->waitl = lock;
	
	PsUnlockTaskSwitch(old);																													// Unlock
	PsSwitchTask(PsDontRequeue);																												// Remove it from the queue and go to the next thread
}

Void PsUnlock(PLock lock) {
	if ((lock == Null) || (PsWaitlList == Null) || (PsCurrentThread == Null)) {																	// Sanity checks
		return;
	} else if (!lock->locked || (lock->owner != PsCurrentThread)) {																				// We're the owner of this lock? We really need to unlock it?
		return;																																	// Nope >:(
	}
	
	PsLockTaskSwitch(old);																														// Lock (the task switching)
	
	lock->locked = False;																														// Unlock it and remove the owner (set it to Null)
	lock->owner = Null;
	
	ListForeach(PsWaitlList, i) {																												// Let's try to wakeup any thread that is waiting for this lock
		PThread th = (PThread)i->data;
		
		if (th->waitl == lock) {
			PsWakeup(PsWaitlList, th);																											// Found one!
			
			lock->locked = True;																												// Let's lock, set the lock owner, and force switch to it!
			lock->owner = th;
			
			PsUnlockTaskSwitch(old);
			PsSwitchTask(Null);
			
			return;
		}
	}
	
	PsUnlockTaskSwitch(old);																													// Unlock (the task switching)
}

Void PsExitThread(UIntPtr ret) {
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null) || (PsProcessList == Null)) {													// Sanity checks
		return;
	} else if ((PsCurrentThread->id == 0) && (PsCurrentProcess->id == 0)) {																		// Kernel main thread?
		PsLockTaskSwitch(old);																													// Yes, so PANIC!
		DbgWriteFormated("PANIC! The main kernel thread was closed\r\n");
		Panic(PANIC_KERNEL_MAIN_THREAD_CLOSED);
	} else if (PsCurrentProcess->threads->length == 1) {																						// We're the only thread?
		PsExitProcess(ret);																														// Yes, so call PsExitProcess	
	}
	
	PsLockTaskSwitch(old);																														// Lock
	
	UIntPtr idx = 0;
	Boolean found = False;
	
	ListForeach(PsCurrentProcess->threads, i) {																									// Let's try to remove ourself from the thread list of this process
		if (i->data == PsCurrentThread) {																										// Found?
			found = True;																														// YES!
			break;
		} else {
			idx++;																																// nope
		}
	}
	
	if (found) {																																// Found?
		ListRemove(PsCurrentProcess->threads, idx);																								// Yes, remove it!
	}
	
	if (PsWaittList != Null) {																													// Let's wake up any thread that is waiting for us
		for (PListNode i = PsWaittList->tail; i != Null; i = i->prev) {
			PThread th = (PThread)i->data;
			
			if (th->waitt == PsCurrentThread) {																									// Found?
				th->retv = ret;																													// Yes, wakeup!
				PsWakeup(PsWaittList, th);
			}
			
			if (i == PsWaittList->head) {
				break;
			}
		}
	}
	
	ListAdd(PsKillList, PsCurrentThread);																										// Add this thread to the kill list
	PsUnlockTaskSwitch(old);																													// Unlock
	PsSwitchTask(PsDontRequeue);																												// Switch to the next thread
	ArchHalt();																																	// Halt
}

Void PsExitProcess(UIntPtr ret) {
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null) || (PsProcessList == Null))  {													// Sanity checks
		return;
	} else if (PsCurrentProcess->id == 0) {																										// Kernel process?
		PsLockTaskSwitch(old);																													// Yes, so PANIC!
		DbgWriteFormated("PANIC! The kernel process was closed\r\n");
		Panic(PANIC_KERNEL_PROCESS_CLOSED);
	}
	
	PsLockTaskSwitch(old);																														// Lock
	
	ListForeach(PsCurrentProcess->shm_mapped_sections, i) {																						// Unmap and dereference all the mapped shm sections
		ShmUnmapSection(((PShmMappedSection)i->data)->shm->key);
	}
	
	ListForeach(PsCurrentProcess->handles, i) {																									// Close all the handles that this process used
		PHandle handle = (PHandle)i->data;
		
		if (handle->used != False) {
			ScFreeHandle(handle->type, handle->data);
		}
		
		MemFree((UIntPtr)handle);
	}
	
	ListFree(PsCurrentProcess->shm_mapped_sections);																							// Free the mapped shm sections list
	ListFree(PsCurrentProcess->handles);																										// And free the handle list
	
	ListForeach(PsCurrentProcess->threads, i) {																									// Let's free and remove all the threads
		PThread th = (PThread)i->data;
		
		if (th != PsCurrentThread) {																											// Remove from the queue?
			UIntPtr idx = 0;
			Boolean found = False;
			
			ListForeach(PsThreadQueue, j) {																										// Let's try to remove this thread from the thread queue
				if (j->data == th) {																											// Found?
					found = True;																												// YES!
					break;
				} else {
					idx++;																														// nope
				}
			}
			
			if (found) {																														// Found?
				ListRemove(PsThreadQueue, idx);																									// Yes, remove it!
			}
			
			if (PsSleepList != Null) {																											// Let's remove any of our threads from the sleep list
				ListForeach(PsSleepList, j) {
					if (j->data == th) {																										// Found?
						PsWakeup2(PsSleepList, th);																								// Yes :)
					}
				}
			}
			
			if (PsWaittList != Null) {																											// Let's remove any of our threads from the waitt list
				for (PListNode j = PsWaittList->tail; j != Null; j = j->prev) {
					PThread th2 = (PThread)j->data;

					if (th2->waitt == PsCurrentThread) {																						// Found any thread waiting THIS THREAD?
						th2->retv = ret;																										// Yes, wakeup!
						PsWakeup(PsWaittList, th2);
					} else if (th2 == th) {																										// Found THIS THREAD?
						PsWakeup2(PsWaittList, th);																								// Yes
					}

					if (j == PsWaittList->head) {
						break;
					}
				}
			}
			
			if (PsWaitpList != Null) {																											// Remove any of our threads from the waitp list
				for (PListNode j = PsWaitpList->tail; j != Null; j = j->prev) {
					PThread th2 = (PThread)j->data;

					if (th2 == th) {																											// Found THIS THREAD?
						PsWakeup2(PsWaitpList, th);																								// Yes
					}

					if (j == PsWaitpList->head) {
						break;
					}
				}
			}
			
			if (PsWaitlList != Null) {																											// Let's remove any of our threads from the waitl list
				ListForeach(PsWaitlList, j) {
					if (j->data == th) {																										// Found?
						PsWakeup2(PsWaitlList, th);																								// Yes :)
					}
				}
			}
			
			PsFreeContext(th->ctx);																												// Free the context
			MemFree((UIntPtr)th);																												// And the thread struct itself
		} else {
			if (PsWaittList != Null) {																											// Let's wakeup any thread waiting for THIS THREAD
				for (PListNode j = PsWaittList->tail; j != Null; j = j->prev) {
					PThread th2 = (PThread)j->data;

					if (th2->waitt == PsCurrentThread) {																						// Found any thread waiting THIS THREAD?
						th2->retv = ret;																										// Yes, wakeup!
						PsWakeup(PsWaittList, th2);
					}

					if (j == PsWaittList->head) {
						break;
					}
				}
			}
		}
	}
	
	if (PsWaitpList != Null) {																													// Let's wakeup any thread waiting for THIS PROCESS
		for (PListNode j = PsWaitpList->tail; j != Null; j = j->prev) {
			PThread th = (PThread)j->data;
			
			if (th->waitp == PsCurrentProcess) {																								// Found any thread waiting THIS PROCESS?
				th->retv = ret;																													// Yes, wakeup!
				PsWakeup(PsWaitpList, th);
			}
	
			if (j == PsWaitpList->head) {
				break;
			}
		}
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	
	ListForeach(PsProcessList, i) {																												// Let's try to remove ourself from the process list
		if (i->data == PsCurrentProcess) {																										// Found?
			found = True;																														// YES!
			break;
		} else {
			idx++;																																// nope
		}
	}
	
	if (found) {																																// Found?
		ListRemove(PsProcessList, idx);																											// Yes, remove it!
	}
	
	PsCurrentThread->killp = True;																												// We should also kill the process
	
	ListAdd(PsKillList, PsCurrentThread);																										// Add this thread to the kill list
	PsUnlockTaskSwitch(old);																													// Unlock
	PsSwitchTask(PsDontRequeue);																												// Switch to the next process
	ArchHalt();																																	// Halt
}

Void PsWakeup(PList list, PThread th) {
	if ((list == Null) || (th == Null) || (PsThreadQueue == Null)) {																			// Sanity checks
		return;
	}
	
	PsLockTaskSwitch(old);																														// Lock
	
	UIntPtr idx = 0;																															// Let's try to find th in list
	Boolean found = False;
	
	ListForeach(list, i) {
		if (i->data == th) {																													// Found?
			found = True;																														// Yes!
			break;
		} else {
			idx++;																																// Nope
		}
	}
	
	if (!found) {																																// Found?
		PsUnlockTaskSwitch(old);																												// Nope
		return;
	}
	
	ListRemove(list, idx);																														// Remove th from list
	
	th->time = PS_DEFAULT_QUANTUM - 1;																											// Let's queue it back!
	th->wtime = 0;
	th->waitl = Null;
	th->waitt = Null;
	th->waitp = Null;
	
	QueueAdd(PsThreadQueue, th);
	PsUnlockTaskSwitch(old);
}

Void PsWakeup2(PList list, PThread th) {
	if ((list == Null) || (th == Null) || (PsThreadQueue == Null)) {																			// Sanity checks
		return;
	}
	
	PsLockTaskSwitch(old);																														// Lock
	
	UIntPtr idx = 0;																															// Let's try to find th in list
	Boolean found = False;
	
	ListForeach(list, i) {
		if (i->data == th) {																													// Found?
			found = True;																														// Yes!
			break;
		} else {
			idx++;																																// Nope
		}
	}
	
	if (!found) {																																// Found?
		PsUnlockTaskSwitch(old);																												// Nope
		return;
	}
	
	ListRemove(list, idx);																														// Remove th from list
	PsUnlockTaskSwitch(old);
}

static Void PsKillerThread(Void) {
	while (True) {																																// In this loop. we're going to kill all the process and threads that we need to kill!
		if (PsKillList->length == 0) {																											// We have anything to kill?
			PsSwitchTask(Null);																													// Nope, switch into the next thread
			continue;
		}
		
		PsLockTaskSwitch(old);																													// Lock task switching
		
		for (PListNode i = PsKillList->tail; i != Null; i = PsKillList->tail) {																	// Let's kill everything that we need to kill
			PThread th = (PThread)i->data;																										// Get the thread
			
			if (th->killp) {																													// Kill the process too?
				MmFreeDirectory(th->parent->dir);																								// Yeah! Free the directory
				MemFree((UIntPtr)th->parent->name);																								// Free the name
				MemFree((UIntPtr)th->parent);																									// And the process struct itself
			}
			
			PsFreeContext(th->ctx);																												// Free the thread context
			MemFree((UIntPtr)th);																												// And free the thread struct :)
			ListRemove(PsKillList, PsKillList->length - 1);																						// Finally, remove the thread from the kill list
		}
		
		PsUnlockTaskSwitch(old);																												// Unlock task switching
	}
}

Void PsInitKillerThread(Void) {
	PThread th = PsCreateThread((UIntPtr)PsKillerThread, 0, False);																				// Now, let's try to create the killer thread
	
	if (th == Null) {
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");																					// ...
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PsAddThread(th);																															// And, add it!
}

Void PsInit(Void) {
	if ((PsThreadQueue = QueueNew(False)) == Null) {																							// Try to init the process queue
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	if ((PsProcessList = ListNew(False, False)) == Null) {																						// Try to init the process list
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	if ((PsSleepList = ListNew(False, False)) == Null) {																						// Try to init the sleep list
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	if ((PsWaittList = ListNew(False, False)) == Null) {																						// Try to init the waitt list
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	if ((PsWaitpList = ListNew(False, False)) == Null) {																						// Try to init the waitp list
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	if ((PsWaitlList = ListNew(False, False)) == Null) {																						// Try to init the waitl list
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	if ((PsKillList = ListNew(False, False)) == Null) {																							// Try to init the kill list
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PProcess proc = PsCreateProcessInt(L"System", (UIntPtr)KernelMainLate, MmKernelDirectory);													// Try to create the system process
	
	if (proc == Null) {
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	if (!QueueAdd(PsThreadQueue, ListGet(proc->threads, 0))) {																					// Try to add it to the thread queue
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	if (!ListAdd(PsProcessList, proc)) {																										// And to the process list
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PsTaskSwitchEnabled = True;																													// Enable task switching
	PsSwitchTask(Null);																															// And switch to the system process!
}
