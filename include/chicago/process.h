// File author is Ítalo Lima Marconato Matias
//
// Created on July 27 of 2018, at 14:42 BRT
// Last edited on November 02 of 2019, at 12:10 BRT

#ifndef __CHICAGO_PROCESS_H__
#define __CHICAGO_PROCESS_H__

#include <chicago/arch/process.h>
#include <chicago/alloc-int.h>
#include <chicago/file.h>
#include <chicago/queue.h>

#define PsLockTaskSwitch(i) Boolean i ## e = PsTaskSwitchEnabled; if (i ## e) PsTaskSwitchEnabled = False;
#define PsUnlockTaskSwitch(i) if (i ## e) PsTaskSwitchEnabled = True;
#define PsCurrentProcess (PsCurrentThread->parent)
#define PsDontRequeue ((PVoid)1)
#define PS_DEFAULT_QUANTUM 20

struct ThreadStruct;

typedef struct {
	Volatile Boolean locked;
	struct ThreadStruct *owner;
} Lock, *PLock;

typedef struct {
	UIntPtr id;
	PWChar name;
	UIntPtr dir;
	PList threads;
	UIntPtr last_tid;
	PAllocBlock alloc_base;
	UIntPtr mem_usage;
	PList handle_list;
	PList global_handle_list;
	PList files;
	IntPtr last_fid;
	PWChar exec_path;
} Process, *PProcess;

typedef struct ThreadStruct {
	UIntPtr id;
	PContext ctx;
	UIntPtr retv;
	UIntPtr time;
	UIntPtr wtime;
	PProcess parent;
	PLock waitl;
	PProcess waitp;
	Boolean killp;
	struct ThreadStruct *waitt;
} Thread, *PThread;

typedef struct {
	PFsNode file;
	IntPtr num;
} ProcessFile, *PProcessFile;

#ifndef __CHICAGO_PROCESS__
extern Boolean PsTaskSwitchEnabled;
extern PThread PsCurrentThread;
extern PQueue PsThreadQueue;
extern PList PsProcessList;
extern PList PsSleepList;
extern PList PsWaittList;
extern PList PsWaitpList;
extern PList PsWaitlList;
#endif

PThread PsCreateThreadInt(UIntPtr entry, UIntPtr userstack, Boolean user);
PProcess PsCreateProcessInt(PWChar name, UIntPtr entry, UIntPtr dir);
PThread PsCreateThread(UIntPtr entry, UIntPtr userstack, Boolean user);
PProcess PsCreateProcess(PWChar name, UIntPtr entry);
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
Void PsUnlock(PLock lock);
Void PsWakeup(PList list, PThread th);
Void PsWakeup2(PList list, PThread th);
PContext PsCreateContext(UIntPtr entry, UIntPtr userstack, Boolean user);
Void PsFreeContext(PContext context);
Void PsSwitchTask(PVoid priv);
Void PsInitKillerThread(Void);
Void PsInit(Void);

#endif		// __CHICAGO_PROCESS_H__
