// File author is Ítalo Lima Marconato Matias
//
// Created on July 27 of 2018, at 14:42 BRT
// Last edited on January 18 of 2020, at 12:13 BRT

#ifndef __CHICAGO_PROCESS_H__
#define __CHICAGO_PROCESS_H__

#include <chicago/arch/process.h>
#include <chicago/file.h>
#include <chicago/queue.h>

#define PS_PRIORITY_REALTIME 0
#define PS_PRIORITY_VERYHIGH 1
#define PS_PRIORITY_HIGH 2
#define PS_PRIORITY_NORMAL 3
#define PS_PRIORITY_LOW 4
#define PS_PRIORITY_VERYLOW 5

#define PsLockTaskSwitch(i) Boolean i ## e = PsTaskSwitchEnabled; if (i ## e) PsTaskSwitchEnabled = False;
#define PsUnlockTaskSwitch(i) if (i ## e) PsTaskSwitchEnabled = True;
#define PsCurrentProcess (PsCurrentThread->parent)
#define PsDontRequeue ((PVoid)1)

struct ThreadStruct;

typedef struct {
	UIntPtr count;
	Volatile Boolean locked;
	struct ThreadStruct *owner;
} Lock, *PLock;

typedef struct {
	UIntPtr id;
	PWChar name;
	UIntPtr dir;
	PList threads;
	UIntPtr last_tid;
	UIntPtr mem_usage;
	PList shm_mapped_sections;
	PList handles;
	IntPtr last_handle_id;
} Process, *PProcess;

typedef struct ThreadStruct {
	UIntPtr id;
	UInt8 prio;
	UInt8 cprio;
	PContext ctx;
	UIntPtr retv;
	UIntPtr time;
	UIntPtr wtime;
	PProcess parent;
	PLock waitl;
	PProcess waitp;
	Boolean killp;
	UIntPtr tls;
	struct ThreadStruct *waitt;
} Thread, *PThread;

typedef struct {
	PFsNode file;
	IntPtr num;
} ProcessFile, *PProcessFile;

#ifndef __CHICAGO_PROCESS__
extern Boolean PsTaskSwitchEnabled;
extern PThread PsCurrentThread;
extern Queue PsThreadQueue[6];
extern List PsProcessList;
extern List PsSleepList;
extern List PsWaittList;
extern List PsWaitpList;
extern List PsWaitlList;
#endif

PThread PsCreateThreadInt(UInt8 prio, UIntPtr entry, UIntPtr userstack, Boolean user);
PProcess PsCreateProcessInt(PWChar name, UInt8 prio, UIntPtr entry, UIntPtr dir);
PThread PsCreateThread(UInt8 prio, UIntPtr entry, UIntPtr userstack, Boolean user);
PProcess PsCreateProcess(PWChar name, UInt8 prio, UIntPtr entry);
Void PsAddToQueue(PThread th, UInt8 priority);
PThread PsGetNext(Void);
Void PsAddThread(PThread th);
Void PsAddProcess(PProcess proc);
PThread PsGetThread(UIntPtr id);
PProcess PsGetProcess(UIntPtr id);
Void PsExitThread(UIntPtr ret);
Void PsExitProcess(UIntPtr ret);
Void PsSleep(UIntPtr ms);
UIntPtr PsWaitThread(UIntPtr id);
UIntPtr PsWaitProcess(UIntPtr id);
Void PsLock(PLock lock);
Boolean PsTryLock(PLock lock);
Void PsUnlock(PLock lock);
Void PsWakeup(PList list, PThread th);
Void PsWakeup2(PList list, PThread th);
PContext PsCreateContext(UIntPtr entry, UIntPtr userstack, Boolean user);
Void PsFreeContext(PContext context);
Void PsSwitchTask(PVoid priv);
Void PsInitIdleProcess(Void);
Void PsInitKillerThread(Void);
Void PsInit(Void);

#endif		// __CHICAGO_PROCESS_H__
