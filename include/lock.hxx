/* File author is Ítalo Lima Marconato Matias
 *
 * Created on December 01 of 2020, at 12:31 BRT
 * Last edited on December 01 of 2020, at 14:20 BRT */

#ifndef __LOCK_HXX__
#define __LOCK_HXX__

#include <status.hxx>

class Thread;

class SpinLock {
public:
	/* Basic lock, used when we can't use the WaitAddress function.
	 * Instead of adding the thread to a wait list, we just use a small loop (while calling Switch). */
	
	Void Lock(Void);
	Status Unlock(Void);
private:
	Thread *Owner = Null;
	Boolean Value = False;
};

class Mutex {
public:
	/* Simillar to the SpinLock, but we only store the owner, and we NEED multitasking to be enabled, as
	 * a value of Null (like the one you would have if tasking is not enabled) means that the mutex is
	 * NOT locked (not that there is no owner but it is locked).
	 * Also, we use ProcManager::WaitAddress instead of directly using CompareAndExchange. */
	
	Status Lock(Void);
	Status Unlock(Void);
private:
	Thread *Owner = Null;
};

#endif
