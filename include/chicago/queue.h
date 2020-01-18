// File author is Ítalo Lima Marconato Matias
//
// Created on October 27 of 2018, at 18:30 BRT
// Last edited on December 07 of 2020, at 11:45 BRT

#ifndef __CHICAGO_QUEUE_H__
#define __CHICAGO_QUEUE_H__

#include <chicago/list.h>

#define Queue List
#define PQueue PList

PQueue QueueNew(Void);
Void QueueFree(PQueue queue);
Boolean QueueAdd(PQueue queue, PVoid data);
PVoid QueueRemove(PQueue queue);

#endif		// __CHICAGO_QUEUE_H__
