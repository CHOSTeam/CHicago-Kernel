// File author is Ítalo Lima Marconato Matias
//
// Created on July 27 of 2018, at 14:59 BRT
// Last edited on January 20 of 2020, at 11:05 BRT

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
PThread PsKillerThread = Null;

Queue PsThreadQueue[6] = {
	{ Null, Null, 0, False },
	{ Null, Null, 0, False },
	{ Null, Null, 0, False },
	{ Null, Null, 0, False },
	{ Null, Null, 0, False },
	{ Null, Null, 0, False }
};

List PsProcessList = { Null, Null, 0, False };
List PsSleepList = { Null, Null, 0, False };
List PsWaittList = { Null, Null, 0, False };
List PsWaitpList = { Null, Null, 0, False };
List PsWaitlList = { Null, Null, 0, False };
List PsKillList = { Null, Null, 0, False };

UIntPtr PsNextID = 0;

Status PsCreateThreadInt(UInt8 prio, UIntPtr entry, UIntPtr userstack, Boolean user, PThread *ret) {
	if (ret == Null || (user && (userstack == 0))) {																							// Sanity checks
		return STATUS_INVALID_ARG;																												// Nope
	} else if (prio >= 6) {																														// If the priority is too high fall back to the normal priority
		prio = PS_PRIORITY_NORMAL;
	}
	
	PThread th = (PThread)MemAllocate(sizeof(Thread));																							// Let's try to allocate the process struct!
	
	if (th == Null) {
		return STATUS_OUT_OF_MEMORY;																											// Failed
	} else if ((th->ctx = PsCreateContext(entry, userstack, user)) == Null) {																	// Create the thread context
		MemFree((UIntPtr)th);																													// Failed
		return STATUS_OUT_OF_MEMORY;
	}
	
	th->id = 0;																																	// We're going to set the id and the parent process later
	th->prio = prio;																															// Set the priority to the default one
	th->cprio = prio;
	th->retv = 0;
	th->time = (prio + 1) * 5 - 1;																												// Set the quantum
	th->wtime = 0;																																// We're not waiting anything (for now)
	th->parent = Null;																															// No parent (for now)
	th->waitl = Null;																															// We're not waiting anything (for now)
	th->waitp = Null;
	th->waitt = Null;
	th->killp = False;
	
	Status status = user ? VirtAllocAddress(0, MM_PAGE_SIZE, VIRT_PROT_READ | VIRT_PROT_WRITE, &th->tls) : STATUS_SUCCESS;						// Init the TLS (if we need to)
	
	if (status != STATUS_SUCCESS) {
		PsFreeContext(th->ctx);																													// Failed
		MemFree((UIntPtr)th);
		return status;
	} else if (user) {
		StrSetMemory((PUInt8)th->tls, 0, MM_PAGE_SIZE);																							// Clean the TLS
	}
	
	*ret = th;
	
	return STATUS_SUCCESS;
}

Status PsCreateProcessInt(PWChar name, UInt8 prio, UIntPtr entry, UIntPtr dir, PProcess *ret) {
	if (ret == Null) {																															// Sanity check
		return STATUS_INVALID_ARG;																												// Nope
	} else if (prio >= 6) {																														// If the priority is too high fall back to the normal priority
		prio = PS_PRIORITY_NORMAL;
	}
	
	PProcess proc = (PProcess)MemAllocate(sizeof(Process));																						// Let's try to allocate the process struct!
	
	if (proc == Null) {
		return STATUS_OUT_OF_MEMORY;																											// Failed
	}
	
	proc->id = PsNextID++;																														// Use the next PID
	proc->name = StrDuplicate(name);																											// Duplicate the name
	
	if ((proc->dir = dir) == 0) {																												// Create an new page dir?
		proc->dir = MmCreateDirectory();																										// Yes
		
		if (proc->dir == 0) {
			MemFree((UIntPtr)proc->name);																										// Failed...
			MemFree((UIntPtr)proc);
			
			return STATUS_OUT_OF_MEMORY;
		}
	}
	
	if ((proc->threads = ListNew(False)) == Null) {																								// Create the thread list
		MmFreeDirectory(proc->dir);																												// Failed...
		MemFree((UIntPtr)proc->name);
		MemFree((UIntPtr)proc);
		return STATUS_OUT_OF_MEMORY;
	}
	
	proc->last_tid = 0;
	proc->mem_usage = 0;
	proc->shm_mapped_sections = ListNew(False);																									// Init the mapped shm sections list for the process
	
	if (proc->shm_mapped_sections == Null) {
		ListFree(proc->threads);																												// Failed...
		MmFreeDirectory(proc->dir);
		MemFree((UIntPtr)proc->name);
		MemFree((UIntPtr)proc);
		return STATUS_OUT_OF_MEMORY;
	}
	
	proc->handles = ListNew(False);																												// Init the handle list for the process
	proc->last_handle_id = 0;
	
	if (proc->handles == Null) {
		ListFree(proc->shm_mapped_sections);																									// Failed...
		ListFree(proc->threads);
		MmFreeDirectory(proc->dir);
		MemFree((UIntPtr)proc->name);
		MemFree((UIntPtr)proc);
		return STATUS_OUT_OF_MEMORY;
	}
	
	PThread th;
	Status status = PsCreateThreadInt(prio, entry, 0, False, &th);																				// Create the first thread
	
	if (status != STATUS_SUCCESS) {
		ListFree(proc->handles);																												// Failed...
		ListFree(proc->shm_mapped_sections);
		ListFree(proc->threads);
		MmFreeDirectory(proc->dir);
		MemFree((UIntPtr)proc->name);
		MemFree((UIntPtr)proc);
		return status;
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
		return STATUS_OUT_OF_MEMORY;
	}
	
	*ret = proc;
	
	return STATUS_SUCCESS;
}

Status PsCreateThread(UInt8 prio, UIntPtr entry, UIntPtr userstack, Boolean user, PThread *ret) {
	return PsCreateThreadInt(prio, entry, userstack, user, ret);																				// Use our PsCreateThreadInt function
}

Status PsCreateProcess(PWChar name, UInt8 prio, UIntPtr entry, PProcess *ret) {
	return PsCreateProcessInt(name, prio, entry, 0, ret);																						// Use our PsCreateProcessInt function
}

Void PsAddToQueue(PThread th, UInt8 priority) {
	if ((th == Null) || (PsCurrentThread == Null) || (PsCurrentProcess == Null)) {																// Sanity checks
		return;
	}
	
	PsLockTaskSwitch(old);																														// Lock
	QueueAdd(&PsThreadQueue[priority], th);																										// Add it to the queue for the right priority
	PsUnlockTaskSwitch(old);																													// And unlock
}

PThread PsGetNext(Void) {
	for (UInt8 i = 0; i < 6; i++) {																												// No need to lock, as we are only supposed to be called in the scheduler, let'a go!
		if (PsThreadQueue[i].length != 0) {																										// Anything on this queue?
			return QueueRemove(&PsThreadQueue[i]);																								// Yeah!
		}
	}
	
	return Null;																																// No threads avaliable... WTF?
}

Void PsAddThread(PThread th) {
	if ((th == Null) || (PsCurrentThread == Null) || (PsCurrentProcess == Null)) {																// Sanity checks
		return;
	}
	
	PsLockTaskSwitch(old);																														// Lock
	
	th->id = PsCurrentProcess->last_tid++;																										// Set the ID
	th->parent = PsCurrentProcess;																												// Set the parent process
	
	PsAddToQueue(th, th->cprio);																												// Try to add this thread to the right queue
	ListAdd(PsCurrentProcess->threads, th);																										// Try to add this thread to the thread list of this process
	PsUnlockTaskSwitch(old);																													// Unlock
}

Void PsAddProcess(PProcess proc) {
	if ((proc == Null) || (proc->dir == 0) || (proc->threads == Null)) {																		// Sanity checks
		return;
	}
	
	PsLockTaskSwitch(old);																														// Lock
	
	ListForeach(proc->threads, i) {																												// Add all the threads from this process to the thread queue
		QueueAdd(&PsThreadQueue[((PThread)i->data)->cprio], i->data);
	}
	
	ListAdd(&PsProcessList, proc);																												// Try to add this process to the process list
	PsUnlockTaskSwitch(old);																													// Unlock
}

PThread PsGetThread(UIntPtr id) {
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null)) {																				// Sanity checks
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
	ListForeach(&PsProcessList, i) {																											// Let's search!
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
	} else if (PsCurrentThread == Null) {																										// Is threading initialized?
		TimerSleep(ms);																															// Nope
		return;
	}
	
	PsLockTaskSwitch(old);																														// Lock
	
	PsCurrentThread->wtime = ms;
	
	if (!ListAdd(&PsSleepList, PsCurrentThread)) {																								// Try to add it to the sleep list
		PsUnlockTaskSwitch(old);																												// Failed, but let's keep on trying!
		PsSleep(ms);
		return;
	}
	
	PsUnlockTaskSwitch(old);																													// Unlock
	PsSwitchTask(PsDontRequeue);																												// Remove it from the queue and go to the next thread
}

Status PsWaitThread(UIntPtr id, PUIntPtr ret) {
	if (PsCurrentThread == Null || ret == Null) {																								// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	PsLockTaskSwitch(old);																														// Lock
	
	if (!ListAdd(&PsWaittList, PsCurrentThread)) {																								// Try to add it to the waitt list
		PsUnlockTaskSwitch(old);																												// Failed, but let's keep on trying!
		return PsWaitThread(id, ret);
	}
	
	PsCurrentThread->waitt = PsGetThread(id);
	
	if (PsCurrentThread->waitt == Null) {
		PsUnlockTaskSwitch(old);
		return STATUS_ALREADY_KILLED;
	}
	
	PsUnlockTaskSwitch(old);																													// Unlock
	PsSwitchTask(PsDontRequeue);																												// Remove it from the queue and go to the next thread
	
	*ret = PsCurrentThread->retv;
	
	return STATUS_SUCCESS;
}

Status PsWaitProcess(UIntPtr id, PUIntPtr ret) {
	if (PsCurrentThread == Null || ret == Null) {																								// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	PsLockTaskSwitch(old);																														// Lock
	
	if (!ListAdd(&PsWaitpList, PsCurrentThread)) {																								// Try to add it to the waitp list
		PsUnlockTaskSwitch(old);																												// Failed, but let's keep on trying!
		return PsWaitThread(id, ret);
	}
	
	PsCurrentThread->waitp = PsGetProcess(id);
	
	if (PsCurrentThread->waitp == Null) {
		PsUnlockTaskSwitch(old);
		return STATUS_ALREADY_KILLED;
	}
	
	PsUnlockTaskSwitch(old);																													// Unlock
	PsSwitchTask(PsDontRequeue);																												// Remove it from the queue and go to the next thread
	
	*ret = PsCurrentThread->retv;
	
	return STATUS_SUCCESS;
}

Status PsLock(PLock lock) {
	if ((lock == Null) || (PsCurrentThread == Null)) {																							// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	PsLockTaskSwitch(old);																														// Lock (the task switching)
	
	if (!lock->locked || lock->owner == PsCurrentThread) {																						// We need to add it to the waitl list?
		lock->count++;																															// Nope, increase the counter, set it as locked and return!
		lock->locked = True;
		lock->owner = PsCurrentThread;
		PsUnlockTaskSwitch(old);
		return STATUS_SUCCESS;
	}
	
	if (!ListAdd(&PsWaitlList, PsCurrentThread)) {																								// Try to add it to the waitl list
		PsUnlockTaskSwitch(old);																												// Failed, but let's keep on trying!
		return PsLock(lock);
	}
	
	PsCurrentThread->waitl = lock;
	
	PsUnlockTaskSwitch(old);																													// Unlock
	PsSwitchTask(PsDontRequeue);																												// Remove it from the queue and go to the next thread
	
	return STATUS_SUCCESS;
}

Status PsTryLock(PLock lock) {
	if ((lock == Null) || (PsCurrentThread == Null)) {																							// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	Status ret = STATUS_TRYLOCK_FAILED;
	
	PsLockTaskSwitch(old);																														// Lock (the task switching)
	
	if (!lock->locked || lock->owner == PsCurrentThread) {																						// We can lock?
		lock->count++;																															// Yes!
		lock->locked = True;
		lock->owner = PsCurrentThread;
		ret = True;
	}
	
	PsUnlockTaskSwitch(old);																													// Unlock (the task switch)
	
	return ret;
}

Status PsUnlock(PLock lock) {
	if ((lock == Null) || (PsCurrentThread == Null)) {																							// Sanity checks
		return STATUS_INVALID_ARG;
	} else if (!lock->locked || (lock->owner != PsCurrentThread)) {																				// We're the owner of this lock? We really need to unlock it?
		return STATUS_NOT_OWNER;																												// Nope >:(
	}
	
	PsLockTaskSwitch(old);																														// Lock (the task switching)
	
	lock->count--;																																// Decrease the counter
	
	if (lock->count > 0) {																														// We can fully unlock it for other threads?
		return STATUS_SUCCESS;																													// Nope...
	}
	
	lock->locked = False;																														// Unlock it and remove the owner (set it to Null)
	lock->owner = Null;
	
	ListForeach(&PsWaitlList, i) {																												// Let's try to wakeup any thread that is waiting for this lock
		PThread th = (PThread)i->data;
		
		if (th->waitl == lock) {
			PsWakeup(&PsWaitlList, th);																											// Found one!
			
			lock->count++;																														// Let's lock, set the counter, set the lock owner, and force switch to it!
			lock->locked = True;
			lock->owner = th;
			
			PsUnlockTaskSwitch(old);
			PsSwitchTask(Null);
			
			return STATUS_SUCCESS;
		}
	}
	
	PsUnlockTaskSwitch(old);																													// Unlock (the task switching)
	
	return STATUS_SUCCESS;
}

static Void PsAddToKillList(PThread th) {
	PsKillerThread->cprio = PsKillerThread->prio;																								// Config the killer thread for rescheduling, reset the current priority
	PsKillerThread->time = (PsKillerThread->cprio + 1) * 5;																						// Set the quantum
	ListAdd(&PsKillList, th);																													// Add the thread to be killed to the kill list
	PsAddToQueue(PsKillerThread, PsKillerThread->cprio);																						// And add the killer thread to the scheduler queue again
}

Void PsExitThread(UIntPtr ret) {
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null)) {																				// Sanity checks
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
	
	for (PListNode i = PsWaittList.tail; i != Null; i = i->prev) {																				// Let's wake up any thread waiting for us
		PThread th = (PThread)i->data;
		
		if (th->waitt == PsCurrentThread) {																										// Found?
			th->retv = ret;																														// Yes, wakeup!
			PsWakeup(&PsWaittList, th);
		}
		
		if (i == PsWaittList.head) {
			break;
		}
	}
	
	ListAdd(&PsKillList, PsCurrentThread);																										// Add this thread to the kill list
	PsUnlockTaskSwitch(old);																													// Unlock
	PsSwitchTask(PsDontRequeue);																												// Switch to the next thread
	ArchHalt();																																	// Halt
}

Void PsExitProcess(UIntPtr ret) {
	if ((PsCurrentThread == Null) || (PsCurrentProcess == Null))  {																				// Sanity checks
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
			for (UInt8 j = 0; j < 6; j++) {																										// Let's try to remove this thread from the thread queue
				UIntPtr idx = 0;
				Boolean found = False;
				
				ListForeach(&PsThreadQueue[j], k) {
					if (k->data == th) {
						found = True;																											// We found it!
						break;
					}
					
					idx++;
				}
				
				if (found) {
					ListRemove(&PsThreadQueue[j], idx);																							// Remove it using the index
					break;
				}
			}
			
			ListForeach(&PsSleepList, j) {																										// Let's remove any of our threads from the sleep list
				if (j->data == th) {																											// Found?
					PsWakeup2(&PsSleepList, th);																								// Yes :)
				}
			}
			
			for (PListNode j = PsWaittList.tail; j != Null; j = j->prev) {																		// Let's remove any of our threads from the waitt list
				PThread th2 = (PThread)j->data;
				
				if (th2->waitt == PsCurrentThread) {																							// Found any thread waiting THIS THREAD?
					th2->retv = ret;																											// Yes, wakeup!
					PsWakeup(&PsWaittList, th2);
				} else if (th2 == th) {																											// Found THIS THREAD?
					PsWakeup2(&PsWaittList, th);																								// Yes
				}
				
				if (j == PsWaittList.head) {
					break;
				}
			}
			
			for (PListNode j = PsWaitpList.tail; j != Null; j = j->prev) {																		// Remove any of our threads from the waitp list
				PThread th2 = (PThread)j->data;
				
				if (th2 == th) {																												// Found THIS THREAD?
					PsWakeup2(&PsWaitpList, th);																								// Yes
				}
				
				if (j == PsWaitpList.head) {
					break;
				}
			}
			
			ListForeach(&PsWaitlList, j) {																										// Let's remove any of our threads from the waitl list
				if (j->data == th) {																											// Found?
					PsWakeup2(&PsWaitlList, th);																								// Yes :)
				}
			}
			
			PsFreeContext(th->ctx);																												// Free the context
			MemFree((UIntPtr)th);																												// And the thread struct itself
		} else {
			for (PListNode j = PsWaittList.tail; j != Null; j = j->prev) {																		// Let's wakeup any thread waiting for THIS THREAD
				PThread th2 = (PThread)j->data;
				
				if (th2->waitt == PsCurrentThread) {																							// Found any thread waiting THIS THREAD?
					th2->retv = ret;																											// Yes, wakeup!
					PsWakeup(&PsWaittList, th2);
				}
				
				if (j == PsWaittList.head) {
					break;
				}
			}
		}
	}
	
	for (PListNode j = PsWaitpList.tail; j != Null; j = j->prev) {																				// Let's wakeup any thread waiting for THIS PROCESS
		PThread th = (PThread)j->data;
		
		if (th->waitp == PsCurrentProcess) {																									// Found any thread waiting THIS PROCESS?
			th->retv = ret;																														// Yes, wakeup!
			PsWakeup(&PsWaitpList, th);
		}
		
		if (j == PsWaitpList.head) {
			break;
		}
	}
	
	UIntPtr idx = 0;
	Boolean found = False;
	
	ListForeach(&PsProcessList, i) {																											// Let's try to remove ourself from the process list
		if (i->data == PsCurrentProcess) {																										// Found?
			found = True;																														// YES!
			break;
		} else {
			idx++;																																// nope
		}
	}
	
	if (found) {																																// Found?
		ListRemove(&PsProcessList, idx);																										// Yes, remove it!
	}
	
	PsCurrentThread->killp = True;																												// We should also kill the process
	
	PsAddToKillList(PsCurrentThread);																											// Add this thread to the kill list
	PsUnlockTaskSwitch(old);																													// Unlock
	PsSwitchTask(PsDontRequeue);																												// Switch to the next process
	ArchHalt();																																	// Halt
}

Void PsWakeup(PList list, PThread th) {
	if ((list == Null) || (th == Null)) {																										// Sanity checks
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
	
	th->cprio = th->prio;																														// Let's queue it back!
	th->time = (th->cprio + 1) * 5;
	th->wtime = 0;
	th->waitl = Null;
	th->waitt = Null;
	th->waitp = Null;
	
	PsAddToQueue(th, th->cprio);
	PsUnlockTaskSwitch(old);
}

Void PsWakeup2(PList list, PThread th) {
	if ((list == Null) || (th == Null)) {																										// Sanity checks
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

static Void PsIdleThread(Void) {
	ArchHalt();																																	// Just halt...
}

static Void PsKillerThreadFunc(Void) {
	while (True) {																																// In this loop. we're going to kill all the process and threads that we need to kill!
		if (PsKillList.length == 0) {																											// We have anything to kill?
			PsSwitchTask(PsDontRequeue);																										// Nope, only reschedule us if we have something to killl
		}
		
		PsLockTaskSwitch(old);																													// Lock task switching
		
		for (PListNode i = PsKillList.tail; i != Null; i = PsKillList.tail) {																	// Let's kill everything that we need to kill
			PThread th = (PThread)i->data;																										// Get the thread
			
			if (th->killp) {																													// Kill the process too?
				MmFreeDirectory(th->parent->dir);																								// Yeah! Free the directory
				MemFree((UIntPtr)th->parent->name);																								// Free the name
				MemFree((UIntPtr)th->parent);																									// And the process struct itself
			}
			
			PsFreeContext(th->ctx);																												// Free the thread context
			MemFree((UIntPtr)th);																												// And free the thread struct :)
			ListRemove(&PsKillList, PsKillList.length - 1);																						// Finally, remove the thread from the kill list
		}
		
		PsUnlockTaskSwitch(old);																												// Unlock task switching
	}
}

Void PsInitIdleProcess(Void) {
	PProcess proc;
	Status status = PsCreateProcess(L"System Idle Process", PS_PRIORITY_VERYLOW, (UIntPtr)PsIdleThread, &proc);									// Now, let's try to create the idle process
	
	if (status != STATUS_SUCCESS) {
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");																					// ...
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PsAddProcess(proc);																															// And, add it!
}

Void PsInitKillerThread(Void) {
	Status status = PsCreateThread(PS_PRIORITY_VERYLOW, (UIntPtr)PsKillerThreadFunc, 0, False, &PsKillerThread);								// Now, let's try to create the killer thread
	
	if (status != STATUS_SUCCESS) {
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");																					// ...
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PsAddThread(PsKillerThread);																												// And, add it!
}

Void PsInit(Void) {
	PProcess proc;
	Status status = PsCreateProcessInt(L"System Process", PS_PRIORITY_NORMAL, (UIntPtr)KernelMainLate, MmKernelDirectory, &proc);				// Try to create the system process
	
	if (status != STATUS_SUCCESS) {
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	if (!QueueAdd(&PsThreadQueue[PS_PRIORITY_NORMAL], ListGet(proc->threads, 0))) {																// Try to add it to the thread queue
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	if (!ListAdd(&PsProcessList, proc)) {																										// And to the process list
		DbgWriteFormated("PANIC! Failed to init tasking\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	PsTaskSwitchEnabled = True;																													// Enable task switching
	PsSwitchTask(Null);																															// And switch to the system process!
}
