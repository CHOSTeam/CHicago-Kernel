// File author is Ítalo Lima Marconato Matias
//
// Created on July 27 of 2018, at 14:42 BRT
// Last edited on February 02 of 2020, at 12:34 BRT

#ifndef __CHICAGO_PROCESS_H__
#define __CHICAGO_PROCESS_H__

#include <chicago/arch/process.h>

#include <chicago/avl.h>
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
	UIntPtr addr;
	UIntPtr value;
	UIntPtr new;
} WaitingForAddress, *PWaitingForAddress;

typedef struct {
	UIntPtr id;
	PWChar name;
	UIntPtr dir;
	List threads;
	UIntPtr last_tid;
	AvlTree mappings;
	PVoid vaddresses_head;
	PVoid vaddresses_tail;
	UIntPtr mem_usage;
	List shm_mapped_sections;
	List handles;
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
	PWaitingForAddress waita;
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
extern Queue PsThreadQueue[6];
extern List PsProcessList;
extern List PsSleepList;
extern List PsWaittList;
extern List PsWaitpList;
extern List PsWaitaList;
#endif

Status PsCreateThreadInt(UInt8 prio, UIntPtr entry, UIntPtr userstack, Boolean user, PThread *ret);
Status PsCreateProcessInt(PWChar name, UInt8 prio, UIntPtr entry, UIntPtr dir, PProcess *ret);
Status PsCreateThread(UInt8 prio, UIntPtr entry, UIntPtr userstack, Boolean user, PThread *ret);
Status PsCreateProcess(PWChar name, UInt8 prio, UIntPtr entry, PProcess *ret);
Void PsAddToQueue(PThread th, UInt8 priority);
PThread PsGetNext(Void);
Void PsAddThread(PThread th);
Void PsAddProcess(PProcess proc);
PThread PsGetThread(UIntPtr id);
PProcess PsGetProcess(UIntPtr id);
Void PsExitThread(UIntPtr ret);
Void PsExitProcess(UIntPtr ret);
Void PsSleep(UIntPtr ms);
Status PsWaitThread(UIntPtr id, PUIntPtr ret);
Status PsWaitProcess(UIntPtr id, PUIntPtr ret);
Status PsWaitForAddress(UIntPtr addr, UIntPtr value, UIntPtr new, UIntPtr ms);
Status PsWakeAddress(UIntPtr addr);
Void PsWakeup(PList list, PThread th);
Void PsWakeup2(PList list, PThread th);
PContext PsCreateContext(UIntPtr entry, UIntPtr userstack, Boolean user);
Void PsFreeContext(PContext context);
Void PsSwitchTask(PVoid priv);
Void PsInitIdleProcess(Void);
Void PsInitKillerThread(Void);
Void PsInit(Void);

#endif		// __CHICAGO_PROCESS_H__
