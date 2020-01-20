// File author is Ítalo Lima Marconato Matias
//
// Created on December 24 of 2019, at 14:28 BRT
// Last edited on January 18 of 2020, at 12:58 BRT

#ifndef __CHICAGO_IPC_H__
#define __CHICAGO_IPC_H__

#include <chicago/process.h>
#include <chicago/queue.h>

typedef struct {
	UInt32 msg;
	IntPtr src;
	IntPtr rport;
	UIntPtr size;
	PUInt8 buffer;
} IpcMessage, *PIpcMessage;

typedef struct {
	PWChar name;
	Queue queue;
	PProcess proc;
} IpcPort, *PIpcPort;

typedef struct {
	Queue queue;
	PProcess proc;
} IpcResponsePort, *PIpcResponsePort;

#ifndef __CHICAGO_IPC__
extern PList IpcPortList;
#endif

Status IpcCreatePort(PWChar name);
Status IpcCreateResponsePort(PIntPtr ret);
Status IpcRemovePort(PWChar name);
Status IpcCheckPort(PWChar name);
Void IpcFreeResponsePort(PIpcResponsePort port);
Status IpcSendMessage(PWChar port, UInt32 msg, UIntPtr size, PUInt8 buf, PIpcResponsePort rport);
Status IpcSendResponse(PIpcResponsePort port, UInt32 msg, UIntPtr size, PUInt8 buf);
Status IpcReceiveMessage(PWChar name, PUInt32 msg, UIntPtr size, PUInt8 buf);
Status IpcReceiveResponse(PIpcResponsePort port, PUInt32 msg, UIntPtr size, PUInt8 buf);
Void IpcInit(Void);

#endif		// __CHICAGO_IPC_H__