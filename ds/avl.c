// File author is Ítalo Lima Marconato Matias
//
// Created on January 21 of 2020, at 12:45 BRT
// Last edited on February 02 of 2020, at 11:02 BRT

#include <chicago/alloc.h>
#include <chicago/avl.h>

PAvlTree AvlCreate(Int (*compare)(PVoid, PVoid), Void (*free_key)(PVoid), Void (*free_value)(PVoid)) {
	if (compare == Null) {																						// Make sure that the essential functions aren't just a Null pointer
		return Null;
	}
	
	PAvlTree avl = (PAvlTree)MemAllocate(sizeof(AvlTree));														// Alloc space for the tree struct
	
	if (avl == Null) {
		return Null;																							// Failed...
	}
	
	avl->root = Null;																							// The first call to AvlInsert will create the root node
	avl->compare = compare;																						// And let's set the callbacks...
	avl->free_key = free_key;
	avl->free_value = free_value;
	
	return avl;
}

static PAvlNode AvlCleanInt(PAvlNode node, Void (*free_key)(PVoid), Void (*free_value)(PVoid)) {
	if (node == Null) {																							// Check if the last node wasn't, uh, the last node (on a side of the tree)
		return Null;
	}
	
	node->left = AvlCleanInt(node->left, free_key, free_value);													// Clean our children
	node->right = AvlCleanInt(node->right, free_key, free_value);
	
	if (free_key != Null) {																						// And free everything else
		free_key(node->key);
	}
	
	if (free_value != Null) {
		free_value(node->value);
	}
	
	MemFree((UIntPtr)node);
	
	return Null;
}

Void AvlClean(PAvlTree avl) {
	if (avl == Null) {																							// Sanity check
		return;
	}
	
	avl->root = AvlCleanInt(avl->root, avl->free_key, avl->free_value);											// And clean the tree
}

static PAvlNode AvlSearchInt(PAvlNode node, PVoid key, Int (*compare)(PVoid, PVoid)) {
	Int res = 0;
	
	if (node == Null || (res = compare(node->key, key)) == 0) {													// Check if the requested key isn't the current one (and if the last node wasn't the last one)
		return node;
	}
	
	return AvlSearchInt(res < 0 ? node->right : node->left, key, compare);										// Determine if we should go to the left or to the right node, and keep on searching...
}

PAvlNode AvlSearch(PAvlTree avl, PVoid key) {
	if (avl == Null || avl->compare == Null || key == Null) {													// Sanity checks
		return Null;
	}
	
	return AvlSearchInt(avl->root, key, avl->compare);															// Redirect to the internal function
}

static PAvlNode AvlCreateNode(PVoid key, PVoid value) {
	PAvlNode node = (PAvlNode)MemAllocate(sizeof(AvlNode));														// Alloc the node struct
	
	if (node == Null) {
		return Null;																							// ... out of memory...
	}
	
	node->key = key;																							// Fill everything
	node->value = value;
	node->height = 1;
	node->left = Null;
	node->right = Null;
	
	return node;
}

static IntPtr AvlGetBiggestHeight(PAvlNode a, PAvlNode b) {
	IntPtr ah = a == Null ? 0 : a->height;																		// Get the height of A
	IntPtr bh = b == Null ? 0 : b->height;																		// And the height of B
	return (ah > bh ? ah : bh) + 1;																				// Return the biggest one...
}

static IntPtr AvlGetBalance(PAvlNode a, PAvlNode b) {
	return (a == Null ? 0 : a->height) - (b == Null ? 0 : b->height);											// Return the height of the A node minus the height of the B node...
}

static PAvlNode AvlRotate(PAvlNode node, Boolean left) {
	PAvlNode a = left ? node->right : node->left;																// Get some stuff that we need
	PAvlNode b = left ? a->left : a->right;
	
	if (left) {																									// Perform the rotation
		a->left = node;
		node->right = b;
	} else {
		a->right = node;
		node->left = b;
	}
	
	node->height = AvlGetBiggestHeight(node->left, node->right);												// And update the heights
	a->height = AvlGetBiggestHeight(a->left, a->right);
	
	return a;
}

static PAvlNode AvlInsertInt(PAvlNode node, PVoid key, PVoid value, Int (*compare)(PVoid, PVoid)) {
	if (node == Null) {																							// End of the search for where we are going to put it?
		return AvlCreateNode(key, value);																		// Yeah!
	}
	
	Int res = compare(node->key, key);																			// Compare the key of the current node and of the new node
	
	if (res > 0) {																								// Our key is bigger than the current node key?
		node->left = AvlInsertInt(node->left, key, value, compare);												// No...
	} else if (res < 0) {
		node->right = AvlInsertInt(node->right, key, value, compare);											// Yeah, it is!
	} else {
		return node;																							// Duplicate entry...
	}
	
	node->height = AvlGetBiggestHeight(node->left, node->right);												// Update the height...
	
	IntPtr balance = AvlGetBalance(node->left, node->right);													// Get the balance
	
	if (balance > 1) {																							// Unbalanced to the left?
		res = compare(node->left->key, key);																	// Yeah, let's see if it is left left or left right...
		
		if (res > 0) {
			return AvlRotate(node, False);																		// Left left...
		} else if (res < 0) {
			node->left = AvlRotate(node->left, True);															// Left right...
			return AvlRotate(node, False);
		}
	} else if (balance < -1) {																					// Or to the right?
		res = compare(node->right->key, key);																	// Yeah, let's see if it is right left or right right...
		
		if (res > 0) {
			node->right = AvlRotate(node->right, False);														// Right left...
			return AvlRotate(node, True);
		} else if (res < 0) {
			return AvlRotate(node, True);																		// Right right...
		}
	}
	
	return node;
}

Boolean AvlInsert(PAvlTree avl, PVoid key, PVoid value) {
	if (avl == Null) {																							// First, sanity check
		return False;
	}
	
	PAvlNode node = AvlInsertInt(avl->root, key, value, avl->compare);											// Now, redirect to the internal insert function
	
	if (node == Null) {
		return False;
	}
	
	avl->root = node;
	
	return True;
}

static PAvlNode AvlRemoveInt(PAvlNode node, PVoid key, Int (*compare)(PVoid, PVoid), Void (*free_key)(PVoid), Void (*free_value)(PVoid)) {
	if (node == Null) {																							// End of the search for the key?
		return Null;																							// Yeah, and we haven't found it...
	}
	
	Int res = compare(node->key, key);																			// Compare the key of the current node and of the new node
	
	if (res > 0) {																								// Our key is bigger than the current node key?
		node->left = AvlRemoveInt(node->left, key, compare, free_key, free_value);								// No...
		goto c;
	} else if (res < 0) {
		node->right = AvlRemoveInt(node->right, key, compare, free_key, free_value);							// Yeah, it is!
		goto c;
	} else if (node->left == Null && node->right == Null) {														// We don't have any children?
		if (free_key != Null) {																					// Yeah, we don't... just free everything and return..
			free_key(node->key);
		}
		
		if (free_value != Null) {
			free_value(node->value);
		}
		
		MemFree((UIntPtr)node);
		
		node = Null;
		
		goto c;
	} else if (node->left == Null || node->right == Null) {														// We only have one children?
		PAvlNode ret = node->left == Null ? node->right : node->left;											// Yeah, so we are going to free ourself and return that children
		
		if (free_key != Null) {
			free_key(node->key);
		}
		
		if (free_value != Null) {
			free_value(node->value);
		}
		
		MemFree((UIntPtr)node);
		
		node = ret;
		
		goto c;
	}
	
	PAvlNode min = node->right;																					// Ok, we have both right and left childrens... let's find the inorder successor...
	
	while (min->left != Null) {
		min = min->left;
	}
	
	node->key = min->key;																						// Copy the key and the data from the successor to us
	node->value = min->value;
	node->right = AvlRemoveInt(node->right, min->key, compare, Null, Null);									// And remove that successor...
	
c:	if (node == Null) {																							// Ok... was this the only node that we had?
		return Null;																							// Yeah, s owe don't need to do anything else...
	}
	
	node->height = AvlGetBiggestHeight(node->left, node->right);												// Update the height...
	
	IntPtr balance = AvlGetBalance(node->left, node->right);													// Get the balance
	
	if (balance > 1) {																							// Unbalanced to the left?
		balance = AvlGetBalance(node->left->left, node->left->right);											// Yeah, let's see if it is left left or left right...
		
		if (balance >= 0) {
			return AvlRotate(node, False);																		// Left left...
		} else if (balance < 0) {
			node->left = AvlRotate(node->left, True);															// Left right...
			return AvlRotate(node, False);
		}
	} else if (balance < -1) {																					// Or to the right?
		balance = AvlGetBalance(node->right->left, node->right->right);											// Yeah, let's see if it is right left or right right...
		
		if (balance > 0) {
			node->right = AvlRotate(node->right, False);														// Right left...
			return AvlRotate(node, True);
		} else if (balance <= 0) {
			return AvlRotate(node, True);																		// Right right...
		}
	}
	
	return node;
}

Void AvlRemove(PAvlTree avl, PVoid key) {
	if (avl == Null) {																							// Sanity checks
		return;
	}
	
	avl->root = AvlRemoveInt(avl->root, key, avl->compare, avl->free_key, avl->free_value);					// And redirect to the internal function
}
