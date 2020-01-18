// File author is Ítalo Lima Marconato Matias
//
// Created on October 12 of 2018, at 15:35 BRT
// Last edited on December 07 of 2020, at 11:46 BRT

#include <chicago/stack.h>

PStack StackNew(Void) {
	return ListNew(False);										// Redirect to ListNew
}

Void StackFree(PStack stack) {
	ListFree(stack);											// Redirect to ListFree
}

Boolean StackPush(PStack stack, PVoid data) {
	if (stack != Null && stack->length == 1024) {				// Stack limit = 1024
		StackPopStart(stack);
	}
	
	return ListAdd(stack, data);								// Redirect to ListAdd
}

Boolean StackPushStart(PStack stack, PVoid data) {
	if (stack != Null && stack->length == 1024) {				// Stack limit = 1024
		StackPopStart(stack);
	}
	
	return ListAddStart(stack, data);							// Redirect to ListAddStart
}

PVoid StackPop(PStack stack) {
	return ListRemove(stack, stack->length - 1);				// Redirect to ListRemove
}

PVoid StackPopStart(PStack stack) {
	return ListRemove(stack, 0);								// Redirect to ListRemove
}
