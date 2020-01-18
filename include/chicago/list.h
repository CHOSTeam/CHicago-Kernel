// File author is Ítalo Lima Marconato Matias
//
// Created on July 14 of 2018, at 22:10 BRT
// Last edited on December 07 of 2020, at 11:45 BRT

#ifndef __CHICAGO_LIST_H__
#define __CHICAGO_LIST_H__

#include <chicago/types.h>

#define ListForeach(l, i) for (PListNode i = (l)->head; i != Null; i = i->next)

typedef struct ListNodeStruct {
	PVoid data;
	struct ListNodeStruct *next;
	struct ListNodeStruct *prev;
} ListNode, *PListNode;

typedef struct {
	PListNode head;
	PListNode tail;
	UIntPtr length;
	Boolean free;
} List, *PList;

PList ListNew(Boolean free);
Void ListFree(PList list);
Boolean ListAdd(PList list, PVoid data);
Boolean ListAddStart(PList list, PVoid data);
PVoid ListRemove(PList list, UIntPtr idx);
PVoid ListGet(PList list, UIntPtr idx);

#endif		// __CHICAGO_LIST_H__
