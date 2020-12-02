/* File author is Ítalo Lima Marconato Matias
 *
 * Created on November 30 of 2020, at 13:21 BRT
 * Last edited on December 01 of 2020, at 15:57 BRT */

#ifndef __PROCESS_HXX__
#define __PROCESS_HXX__

#include <lock.hxx>
#include <string.hxx>

class Process;
class ProcManager;

class Thread {
public:
	struct WaitAddress {
		UIntPtr *Address;
		UIntPtr Expected;
		UIntPtr Value;
	};
	
	friend class Process;
	friend class ProcManager;
	
	friend Void KillerThread(Void);
	
	Thread(UIntPtr, Process*, Void*);
	~Thread(Void);
	
	UIntPtr GetID(Void) const { return ID; }
private:
	SpinLock Lock;
	Void *Context;
	IntPtr Return = 0;
	WaitAddress WaitA;
	Thread *WaitT = Null;
	Process *Parent, *WaitP = Null;
	UIntPtr ID, WaitS = 0, Time = 19;
	Boolean Requeue = True, Finished = False;
};

class Process {
public:
	friend class Thread;
	friend class ProcManager;
	
	friend Void KillerThread(Void);
	
	Process(const String&, UIntPtr, UIntPtr);
	~Process(Void);
	
	Status Get(UIntPtr, Thread*&);
	
	UIntPtr GetID(Void) const { return ID; }
	const String &GetName(Void) const { return Name; }
private:
	SpinLock Lock;
	IntPtr Return = 0;
	const String Name;
	List<Thread*> Threads;
	Boolean Finished = False;
	UIntPtr ID, Directory, LastTID = 0;
};

class ProcManager {
public:
	friend class Thread;
	friend class Process;
	friend Void KillerThread(Void);
	
	static Void Initialize(UIntPtr);
	
	static Status CreateThread(UIntPtr, Thread*&);
	static Status CreateProcess(const String&, UIntPtr, Process*&);
	static Status CreateProcess(const String&, UIntPtr, UIntPtr, Process*&);
	
	static Status GetThread(UIntPtr, Thread*&);
	static Status GetProcess(UIntPtr, Process*&);
	static Status GetProcess(const String&, Process*&);
	
	static Void Wait(UIntPtr);
	static IntPtr Wait(Thread*);
	static IntPtr Wait(Process*);
	static Status Wait(UIntPtr*, UIntPtr, UIntPtr);
	
	static Void ExitThread(IntPtr);
	static Void ExitProcess(IntPtr);
	
	static Void Switch(Boolean = True);
	static Void SwitchManual(Void*);
	static Void SwitchTimer(Void*);
	
	static Boolean GetStatus(Void) { return Enabled; }
	static Void SetStatus(Boolean New) { Enabled = New; }
	
	static Thread *GetCurrentThread(Void) { return CurrentThread; }
	static Process *GetCurrentProcess(Void) { return CurrentThread->Parent; }
private:
	static Thread *GetNext(Void);
	static Void WakeupThread(List<Thread*>&, Thread*);
	static Status CreateThread(Process*, UIntPtr, Thread*&, Boolean = True);
	
	static SpinLock Lock;
	static Boolean Enabled;
	static UIntPtr LastPID;
	static Thread *CurrentThread;
	static List<Process*> Processes;
	static List<Thread*> Queue, WaitA, WaitT, WaitP, WaitS, KillList;
};

#endif
