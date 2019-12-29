// File author is Ítalo Lima Marconato Matias
//
// Created on December 24 of 2019, at 14:28 BRT
// Last edited on December 25 of 2019, at 22:18 BRT

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

Boolean IpcCreatePort(PWChar name);
IntPtr IpcCreateResponsePort(Void);
Void IpcRemovePort(PWChar name);
Boolean IpcCheckPort(PWChar name);
Void IpcFreeResponsePort(PIpcResponsePort port);
PIpcMessage IpcSendMessage(PWChar port, UInt32 msg, UIntPtr size, PUInt8 buf, PIpcResponsePort rport);
Void IpcSendResponse(PIpcResponsePort port, UInt32 msg, UIntPtr size, PUInt8 buf);
PIpcMessage IpcReceiveMessage(PWChar name);
PIpcMessage IpcReceiveResponse(PIpcResponsePort port);
Void IpcInit(Void);

#endif		// __CHICAGO_IPC_H__