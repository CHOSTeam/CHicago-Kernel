// File author is Ítalo Lima Marconato Matias
//
// Created on September 04 of 2019, at 18:13 BRT
// Last edited on September 04 of 2019, at 20:12 BRT

#include <chicago/gui/window.h>
#include <chicago/mm.h>
#include <chicago/string.h>

PGuiWindow GuiCreateWindow(PWChar title, UIntPtr x, UIntPtr y, UIntPtr w, UIntPtr h) {
	PGuiWindow window = (PGuiWindow)MmAllocUserMemory(sizeof(GuiWindow));								// Try to alloc memory for the window struct
	
	if (window == Null) {
		return Null;																					// Failed...
	} else if (title == Null) {																			// Do we have a title?
		title = L"Untitled";																			// Nope, let's set a default one
	}
	
	window->title = (PWChar)MmAllocUserMemory((StrGetLength(title) + 1) * sizeof(WChar));				// Alloc space for cloning the title
	
	if (window->title == Null) {
		MmFreeUserMemory((UIntPtr)window);																// Failed :(
		return Null;
	}
	
	StrCopy(window->title, title);																		// Copy the title
	
	window->x = x;																						// And setup the rest of the struct
	window->y = y;
	window->w = w + 8;
	window->h = h + 27;
	
	return window;
}

Void GuiFreeWindow(PGuiWindow window) {
	if (window == Null) {																				// Sanity check
		return;
	}
	
	MmFreeUserMemory((UIntPtr)window->title);															// Free everything
	MmFreeUserMemory((UIntPtr)window);
}
