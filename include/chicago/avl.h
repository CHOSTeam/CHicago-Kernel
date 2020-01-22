// File author is Ítalo Lima Marconato Matias
//
// Created on January 21 of 2020, at 12:45 BRT
// Last edited on January 21 of 2020, at 20:43 BRT

#ifndef __CHICAGO_AVL_H__
#define __CHICAGO_AVL_H__

#include <chicago/types.h>

typedef struct AvlNodeStruct {
	PVoid key;
	PVoid value;
	IntPtr height;
	struct AvlNodeStruct *left;
	struct AvlNodeStruct *right;
} AvlNode, *PAvlNode;

typedef struct {
	PAvlNode root;
	Boolean free;
	Int (*compare)(PVoid, PVoid);
} AvlTree, *PAvlTree;

PAvlTree AvlCreate(Int (*compare)(PVoid, PVoid), Boolean free);
PAvlNode AvlSearch(PAvlTree avl, PVoid key);
Boolean AvlInsert(PAvlTree avl, PVoid key, PVoid value);
Void AvlRemove(PAvlTree avl, PVoid key);

#endif		// __CHICAGO_AVL_H__
