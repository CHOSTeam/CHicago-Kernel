/* File author is Ítalo Lima Marconato Matias
 *
 * Created on December 01 of 2020, at 12:40 BRT
 * Last edited on December 01 of 2020, at 14:23 BRT */

#include <lock.hxx>
#include <process.hxx>

/* Some helper functions, as the GCC/Clang builtins are... uh, a bit complex? Well, not that much, but
 * it still helps. */

static Boolean CompareAndExchange(Boolean *Pointer, Boolean Expected, Boolean Value) {
	return __atomic_compare_exchange_n(Pointer, &Expected, Value, 0,
									   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

Void SpinLock::Lock(Void) {
	/* Maybe we should move the "yield"/pause to some arch file? Idk, for now, I'm gonna put it here.
	 * We need to use the CompareAndExchange function in the loop, else, we may have some problems with
	 * MP. */
	
	while (!CompareAndExchange(&Value, False, True)) {
#if defined(__i386__) || defined(__amd64__)
		asm volatile("pause");
#endif
	}
	
	Owner = ProcManager::GetCurrentThread();
}

Status SpinLock::Unlock(Void) {
	if (Owner != ProcManager::GetCurrentThread()) {
		return Status::NotOwner;
	}
	
	Owner = Null;
	Value = False;
	
	return Status::Success;
}

Status Mutex::Lock(Void) {
	/* This time we need to return error if tasking is not enabled/there is no current thread. */
	
	if (ProcManager::GetCurrentThread() == Null) {
		return Status::InvalidThread;
	}
	
	return ProcManager::Wait((UIntPtr*)&Owner, 0, (UIntPtr)ProcManager::GetCurrentThread());
}

Status Mutex::Unlock(Void) {
	if (ProcManager::GetCurrentThread() == Null || Owner != ProcManager::GetCurrentThread()) {
		return Status::NotOwner;
	}
	
	Owner = Null;
	
	return Status::Success;
}
