// File author is Ítalo Lima Marconato Matias
//
// Created on October 27 of 2018, at 18:28 BRT
// Last edited on December 07 of 2020, at 11:46 BRT

#include <chicago/queue.h>

PQueue QueueNew(Void) {
	return ListNew(False);								// Redirect to ListNew
}

Void QueueFree(PQueue queue) {
	ListFree(queue);									// Redirect to ListFree
}

Boolean QueueAdd(PQueue queue, PVoid data) {	
	return ListAddStart(queue, data);					// Redirect to ListAddStart
}

PVoid QueueRemove(PQueue queue) {
	if (queue->length > 0) {
		return ListRemove(queue, queue->length - 1);	// Redirect to ListRemove
	} else {
		return Null;
	}
}
